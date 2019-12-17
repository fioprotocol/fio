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

# Ensure we're in the repo root and not inside of scripts
cd $( dirname "${BASH_SOURCE[0]}" )/..

# Load fio specific helper functions
. ./scripts/helpers/fio_helper.sh

[[ ! $NAME == "Ubuntu" ]] && set -i # Ubuntu doesn't support interactive mode since it uses dash

[[ ! -f ${BUILD_DIR}/CMakeCache.txt ]] && printf "${COLOR_RED}Please run ${SCRIPT_DIR}/fio_build.sh first!${COLOR_NC}" && exit 1
echo "${COLOR_CYAN}====================================================================================="
echo "========================== ${COLOR_WHITE}Starting FIO Installation${COLOR_CYAN} ==============================${COLOR_NC}"
execute cd $BUILD_DIR
execute make install
execute cd ..

if hash eosio-cpp 2>/dev/null; then
    echo $'Restart Detected\n\n'
else
    cd ../../
    git clone https://github.com/dapixio/cdt
    sleep 2s
    cd cdt
    git submodule update --init --recursive
    ./build.sh
    sudo ./install.sh
fi

printf "\n${COLOR_RED}      ___           ___           ___                       ___\n"
printf "\n${bldred}\n"
printf "      ___                       ___                 \n"
printf "     /\\__\\                     /\\  \\            \n"
printf "    /:/ _/_       ___         /::\\  \\             \n"
printf "   /:/ /\\__\\     /\\__\\       /:/\\:\\  \\       \n"
printf "  /:/ /:/  /    /:/__/      /:/  \\:\\  \\          \n"
printf " /:/_/:/  /    /::\\  \\     /:/__/ \\:\\__\\       \n"
printf " \\:\\/:/  /     \\/\\:\\  \\__  \\:\\  \\ /:/  /   \n"
printf "  \\::/__/         \\:\\/\\__\\  \\:\\  /:/  /      \n"
printf "   \\:\\  \\          \\::/  /   \\:\\/:/  /        \n"
printf "    \\:\\__\\         /:/  /     \\::/  /           \n"
printf "     \\/__/         \\/__/       \\/__/             \n"
printf "  FOUNDATION FOR INTERWALLET OPERABILITY            \n\n${COLOR_NC}"

printf "==============================================================================================\\n"
printf "${COLOR_GREEN}FIO has been installed into ${CACHED_INSTALL_PATH}/bin${COLOR_NC}"
printf "\\n${COLOR_YELLOW}Uninstall with: ${SCRIPT_DIR}/fio_uninstall.sh${COLOR_NC}\\n"
printf "==============================================================================================\\n\\n"
resources