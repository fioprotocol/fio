#!/usr/bin/env bash
set -eo pipefail
VERSION=2.1
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
  -a      Build and Install FIO
  -t      Build and Install the FIO Contract Development Toolkit (CDT), to support FIO contract development.
   \\n" "$0" 1>&2
   exit 1
}

BUILD_FIO=${BUILD_FIO:-false}
INSTALL_FIO_CDT=${INSTALL_FIO_CDT:-false}
if [ $# -ne 0 ]; then
   while getopts "at" opt; do
      case "${opt}" in
      a)
         BUILD_FIO=true
         ;;
      t)
         INSTALL_FIO_CDT=true
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

# Get directory this script is in
MY_SCRIPT_DIR=$(cd -P -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)

# Build fio first if -a argument was given
if $BUILD_FIO; then
  echo "Building FIO..."
  # Pass all args to fio_build.sh
  $MY_SCRIPT_DIR/fio_build.sh "$@"
  echo
fi

# Ensure we're in the repo root and not inside of scripts
cd $(dirname "${BASH_SOURCE[0]}")/..

# Load fio specific helper functions
. ./scripts/helpers/fio_helper.sh

# Load build env (generated during build)
. ./scripts/.build_env

echo "Capturing env" && env > fio_build_env.out

[[ ! $NAME == "Ubuntu" ]] && set -i # Ubuntu doesn't support interactive mode since it uses dash

[[ ! -f ${BUILD_DIR}/CMakeCache.txt ]] && printf "${COLOR_RED}Please run ./fio_build.sh first!${COLOR_NC}" && echo && exit 1
echo "${COLOR_CYAN}====================================================================================="
echo "========================== ${COLOR_WHITE}Starting FIO Installation${COLOR_CYAN} ==============================${COLOR_NC}"
execute cd $BUILD_DIR
execute make install
execute cd ..

printf "${COLOR_RED}\n"
printf "      ___                       ___               \n"
printf "     /\\__\\                     /\\  \\          \n"
printf "    /:/ _/_      ___          /::\\  \\           \n"
printf "   /:/ /\\__\\    /\\__\\        /:/\\:\\  \\     \n"
printf "  /:/ /:/  /   /:/__/       /:/  \\:\\  \\        \n"
printf " /:/_/:/  /   /::\\  \\      /:/__/ \\:\\__\\     \n"
printf " \\:\\/:/  /    \\/\\:\\  \\__   \\:\\  \\ /:/  / \n"
printf "  \\::/__/        \\:\\/\\__\\   \\:\\  /:/  /    \n"
printf "   \\:\\  \\         \\::/  /    \\:\\/:/  /      \n"
printf "    \\:\\__\\        /:/  /      \\::/  /         \n"
printf "     \\/__/        \\/__/        \\/__/           \n"
printf "  FOUNDATION FOR INTERWALLET OPERABILITY          \n\n${COLOR_NC}"

printf "==============================================================================================\\n"
printf "${COLOR_GREEN}FIO has been installed to ${CACHED_INSTALL_PATH}${COLOR_NC}"
printf "\\n${COLOR_YELLOW}Uninstall with: ${REPO_ROOT}/scripts/fio_uninstall.sh${COLOR_NC}\\n"
printf "==============================================================================================\\n\\n"
resources
printf "==============================================================================================\\n\\n"
echo
if ! hash eosio-cpp 2>/dev/null; then
   if $INSTALL_FIO_CDT; then
      cd ../
      echo && echo "Cloning fio.cdt..." && echo
      [ -d fio.cdt ] || git clone https://www.github.com/fioprotocol/fio.cdt.git
      sleep 2s
      cd fio.cdt
      git submodule update --init --recursive
      git checkout --recurse-submodules -- .
      echo && echo "Checking out upgrade branch. Remove on merge to develop!" && echo
      git checkout feature/bd-4618-ubuntu-upgrade
      echo && echo "Cleaning fio.cdt build directory..." && echo
      rm -rf build
      ./build.sh
      sudo ./install.sh
   else
      echo
      echo "The FIO Contract Development Toolkit is not installed. If contract development will be performed, either"
      echo "re-execute this script with the '-t' option or perform the following steps to clone, build and install the"
      echo "fio.cdt suite;"
      echo "  git clone https://www.github.com/fioprotocol/fio.cdt.git"
      echo "  cd fio.cdt"
      echo "  git submodule update --init --recursive"
      echo "  ./build.sh"
      echo "  sudo ./install.sh"
      echo
   fi
fi
