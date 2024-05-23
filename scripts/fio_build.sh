#!/usr/bin/env bash
set -eo pipefail

##########################################################################
# This is the FIO automated install script for Linux and Mac OS.
# This file was downloaded from https://github.com/fioprotocol/fio
#
# Copyright (c) 2017, Respective Authors all rights reserved.
#
# After June 1, 2018 this software is available under the following terms:
#
# The MIT License
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
# https://github.com/fioprotocol/fio/blob/master/LICENSE
##########################################################################

function usage() {
   printf "Usage: $0 OPTION...
  -P          Build with pinned clang and libcxx
  -o TYPE     Build <Debug|Release|RelWithDebInfo|MinSizeRel>. Default: Release
  -s NAME     Core Symbol Name <1-7 characters>. Default: SYS
  -b DIR      Use pre-built boost in DIR
  -i DIR      Install Directory (FIO binary, dependencies). Default: $HOME/fio
  -y          Noninteractive mode (answers yes to every prompt)
  -c          Enable Code Coverage
  -d          Generate Doxygen
  -m          Build MongoDB dependencies
   \\n" "$0" 1>&2
   exit 1
}

TIME_BEGIN=$(date -u +%s)
DEBUG=${DEBUG:-false}
if [ $# -ne 0 ]; then
   while getopts "ado:s:b:i:ycdhmP" opt; do
      case "${opt}" in
      a) #NOOP: passed from fio_install.sh
         ;;
      d)
         DEBUG=true
         set -x
         ;;
      o)
         options=("Debug" "Release" "RelWithDebInfo" "MinSizeRel")
         if [[ "${options[*]}" =~ "${OPTARG}" ]]; then
            CMAKE_BUILD_TYPE=$OPTARG
         else
            echo "Invalid argument: ${OPTARG}" 1>&2
            usage
         fi
         ;;
      s)
         if [ "${#OPTARG}" -gt 7 ] || [ -z "${#OPTARG}" ]; then
            echo "Invalid argument: ${OPTARG}" 1>&2
            usage
         else
            CORE_SYMBOL_NAME=$OPTARG
         fi
         ;;
      b)
         BOOST_LOCATION=$OPTARG
         ;;
      i)
         INSTALL_LOCATION=$OPTARG
         ;;
      y)
         NONINTERACTIVE=true
         PROCEED=true
         ;;
      c)
         ENABLE_COVERAGE_TESTING=true
         ;;
      d)
         ENABLE_DOXYGEN=true
         ;;
      m)
         ENABLE_MONGO=true
         ;;
      P)
         PIN_COMPILER=true
         ;;
      h)
         usage
         ;;
      ?)
         echo "Invalid Option!" 1>&2
         usage
         ;;
      :)
         echo "Invalid Option: -${OPTARG} requires an argument." 1>&2
         usage
         ;;
      *)
         usage
         ;;
      esac
   done
fi

SCRIPT_VERSION=3.5 # Build script version (change this to re-build the CICD image)
export CURRENT_WORKING_DIR=$(pwd) # relative path support

# Ensure we're in the repo root and not inside of scripts
cd $(dirname "${BASH_SOURCE[0]}")/..

# Load FIO specific helper functions
. ./scripts/helpers/fio_helper.sh

$VERBOSE && echo "Build Script Version: ${SCRIPT_VERSION}"
echo "FIO Version: ${EOSIO_VERSION_FULL}"
echo "$(date -u)"
echo "User: ${CURRENT_USER}"
# echo "git head id: %s" "$( cat .git/refs/heads/master )"
echo "Current branch: $(execute git rev-parse --abbrev-ref HEAD 2>/dev/null)"

([[ ! $NAME == "Ubuntu" ]] && [[ ! $ARCH == "Darwin" ]] && [[ ! $NAME == "CentOS Linux" ]]) && set -i # Ubuntu doesn't support interactive mode since it uses dash + Some folks are having this issue on Darwin; colors aren't supported yet anyway

if [[ ! $NAME == "Ubuntu" ]] && [[ ! $ARCH == "Darwin" ]]; then
   echo "${COLOR_RED}Only Ubuntu(dev,test,prod) and MacOS Darwin(dev) are currently supported. Cannot proceed."
   exit 1
fi

# Ensure sudo is available (only if not using the root user)
ensure-sudo
# Test that which is on the system before proceeding
ensure-which
# Prevent a non-git clone from running
ensure-git-clone
# Prompt user for installation path.
install-directory-prompt
# If the same version has already been installed...
previous-install-prompt
# Prompt user for mongo install
#prompt-mongo-install
# Prompt user about pinned clang build
prompt-pinned-clang-build
# Prompt user about pinned llvm build
prompt-pinned-llvm-build
# Setup directories and envs we need (must come last)
setup
execute cd $REPO_ROOT

# Submodules need to be up to date
ensure-submodules-up-to-date

# Force install of cmake
# MacOS (darwin) - handle via brew? See https://cmake.org/download/
export CMAKE=
if [[ $ARCH == "Darwin" ]]; then
   ([[ -z "${CMAKE}" ]] && [[ ! -z $(command -v cmake 2>/dev/null) ]]) && export CMAKE=$(command -v cmake 2>/dev/null) && export CMAKE_CURRENT_VERSION=$($CMAKE --version | grep -E "cmake version[[:blank:]]*" | sed 's/.*cmake version //g')

   # If it exists, check that it's > required version +
   if [[ ! -z $CMAKE_CURRENT_VERSION ]] && [[ $((10#$(echo $CMAKE_CURRENT_VERSION | awk -F. '{ printf("%03d%03d%03d\n", $1,$2,$3); }'))) -lt $((10#$(echo $CMAKE_REQUIRED_VERSION | awk -F. '{ printf("%03d%03d%03d\n", $1,$2,$3); }'))) ]]; then
      echo "${COLOR_RED}The currently installed cmake version ($CMAKE_CURRENT_VERSION) is less than the required version ($CMAKE_REQUIRED_VERSION). Cannot proceed."
      exit 1
   fi
fi

# Use existing cmake on system (either global or specific to eosio)
# Setup based on architecture
if [[ $ARCH == "Linux" ]]; then
   export CMAKE=${CMAKE:-${EOSIO_INSTALL_DIR}/bin/cmake}
   [[ ! -e /etc/os-release ]] && print_supported_linux_distros_and_exit
   case $NAME in
   "Amazon Linux AMI" | "Amazon Linux")
      echo "${COLOR_CYAN}[Ensuring YUM installation]${COLOR_NC}"
      FILE="${REPO_ROOT}/scripts/helpers/build_amazonlinux.sh"
      ;;
   "CentOS Linux")
      FILE="${REPO_ROOT}/scripts/helpers/build_centos.sh"
      ;;
   "Ubuntu")
      FILE="${REPO_ROOT}/scripts/helpers/build_ubuntu.sh"
      ;;
   *) print_supported_linux_distros_and_exit ;;
   esac
   CMAKE_PREFIX_PATHS="${EOSIO_INSTALL_DIR}"
fi

if [ "$ARCH" == "Darwin" ]; then
   # opt/gettext: clio requires Intl, which requires gettext; it's keg only though and we don't want to force linking: https://github.com/EOSIO/eos/issues/2240#issuecomment-396309884
   # EOSIO_INSTALL_DIR/lib/cmake: mongo_db_plugin.cpp:25:10: fatal error: 'bsoncxx/builder/basic/kvp.hpp' file not found
   CMAKE_PREFIX_PATHS="/usr/local/opt/gettext;${EOSIO_INSTALL_DIR}"
   FILE="${SCRIPT_DIR}/build_darwin.sh"
   export CMAKE=${CMAKE}
fi

# Find and replace OPT_DIR in pinned_toolchain.cmake, then move it into build dir
execute bash -c "sed -e 's~@~$OPT_DIR~g' $SCRIPT_DIR/pinned_toolchain.cmake &> $BUILD_DIR/pinned_toolchain.cmake"

# Set OS and associated system vars
set_system_vars # JOBS, Memory, disk space available, etc

echo "${COLOR_CYAN}====================================================================================="
echo "======================= ${COLOR_WHITE}Starting FIO Dependency Install${COLOR_CYAN} ===========================${COLOR_NC}"
execute cd $SRC_DIR
echo "Architecture: ${ARCH}"
. $FILE # Execute OS specific build file

execute cd $REPO_ROOT

# Save off env vars for later use, i.e. for install
echo-to-envfile "EOSIO_INSTALL_DIR" ${EOSIO_INSTALL_DIR}
echo-to-envfile "CMAKE_REQUIRED_VERSION" ${CMAKE_REQUIRED_VERSION}
echo-to-envfile "CMAKE_INSTALL_DIR" ${CMAKE_INSTALL_DIR}
echo-to-envfile "CMAKE" ${CMAKE}

echo ""
echo "${COLOR_CYAN}========================================================================"
echo "======================= ${COLOR_WHITE}Starting FIO Build${COLOR_CYAN} ===========================${COLOR_NC}"
if $VERBOSE; then
   echo "CXX: $CXX"
   echo "CC: $CC"
fi
[[ -n $BUILD_DIR_CLEANUP_SKIP ]] \
  && printf "\n>>>>> Build cleanup skipped (BUILD_DIR_CLEANUP_SKIP is set)! Re-using previous build artifacts including configuration...\n\n"

execute cd $BUILD_DIR
# LOCAL_CMAKE_FLAGS
$ENABLE_MONGO && LOCAL_CMAKE_FLAGS="-DBUILD_MONGO_DB_PLUGIN=true ${LOCAL_CMAKE_FLAGS}" # Enable Mongo DB Plugin if user has enabled -m
if $PIN_COMPILER; then
   CMAKE_PREFIX_PATHS="${CMAKE_PREFIX_PATHS};${LLVM_ROOT}"
   LOCAL_CMAKE_FLAGS="${PINNED_TOOLCHAIN} -DCMAKE_PREFIX_PATH='${CMAKE_PREFIX_PATHS}' ${LOCAL_CMAKE_FLAGS}"
else
   LOCAL_CMAKE_FLAGS="-DCMAKE_CXX_COMPILER='${CXX}' -DCMAKE_C_COMPILER='${CC}' -DCMAKE_PREFIX_PATH='${CMAKE_PREFIX_PATHS}' ${LOCAL_CMAKE_FLAGS}"
fi
$ENABLE_DOXYGEN && LOCAL_CMAKE_FLAGS="-DBUILD_DOXYGEN='${DOXYGEN}' ${LOCAL_CMAKE_FLAGS}"
$ENABLE_COVERAGE_TESTING && LOCAL_CMAKE_FLAGS="-DENABLE_COVERAGE_TESTING='${ENABLE_COVERAGE_TESTING}' ${LOCAL_CMAKE_FLAGS}"

execute bash -c "$CMAKE -DCMAKE_BUILD_TYPE='${CMAKE_BUILD_TYPE}' -DCORE_SYMBOL_NAME='${CORE_SYMBOL_NAME}' -DCMAKE_INSTALL_PREFIX='${EOSIO_INSTALL_DIR}' ${LOCAL_CMAKE_FLAGS} '${REPO_ROOT}'"
execute make -j$JOBS
execute cd $REPO_ROOT 1>/dev/null

TIME_END=$(($(date -u +%s) - $TIME_BEGIN))

printf "${bldred}\n"
printf "      ___                        ___              \n"
printf "     /\\__\\                      /\\  \\         \n"
printf "    /:/ _/_       ___          /::\\  \\          \n"
printf "   /:/ /\\__\\     /\\__\\        /:/\\:\\  \\    \n"
printf "  /:/ /:/  /    /:/__/       /:/  \\:\\  \\       \n"
printf " /:/_/:/  /    /::\\  \\      /:/    \\:\\__\\    \n"
printf " \\:\\/:/  /     \\/\\:\\  \\__   \\:\\    /:/  / \n"
printf "  \\::/__/         \\:\\/\\__\\   \\:\\  /:/  /   \n"
printf "   \\:\\  \\          \\::/  /    \\:\\/:/  /     \n"
printf "    \\:\\__\\         /:/  /      \\::/  /        \n"
printf "     \\/__/         \\/__/        \\/__/          \n"
printf "  FOUNDATION FOR INTERWALLET OPERABILITY            \n\n${txtrst}"
printf "FIO has been successfully built. %02d:%02d:%02d\\n" $(($TIME_END / 3600)) $(($TIME_END % 3600 / 60)) $(($TIME_END % 60))
printf "==============================================================================================\\n${bldred}"
echo "${COLOR_GREEN}You can now install using: ${REPO_ROOT}/scripts/fio_install.sh${COLOR_NC}"
echo "${COLOR_YELLOW}Uninstall with: ${REPO_ROOT}/scripts/fio_uninstall.sh${COLOR_NC}"

echo ""
echo "${COLOR_CYAN}If you wish to perform tests to ensure functional code:${COLOR_NC}"
if $ENABLE_MONGO; then
   echo "${BIN_DIR}/mongod --dbpath ${MONGODB_DATA_DIR} -f ${MONGODB_CONF} --logpath ${MONGODB_LOG_DIR}/mongod.log &"
   PATH_TO_USE=" PATH=\$PATH:$OPT_DIR/mongodb/bin"
fi
echo "cd ${BUILD_DIR} && ${PATH_TO_USE} make test" # PATH is set as currently 'mongo' binary is required for the mongodb test

echo ""
resources
