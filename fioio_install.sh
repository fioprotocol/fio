#!/bin/bash
##########################################################################
# This is the FIOIO automated install script for Linux and Mac OS.
#
# Copyright (c) 2018, Dapix INC.
#
# After ??? this software is available under the following terms:
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
# https://github.com/daixio/dapix_core/blob/master/LICENSE
##########################################################################

if [ "$(id -u)" -ne 0 ]; then
        printf "\n\tThis requires sudo. Please run with sudo.\n\n"
        exit -1
fi   

   CWD="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
   if [ "${CWD}" != "${PWD}" ]; then
      printf "\\n\\tPlease cd into directory %s to run this script.\\n \\tExiting now.\\n\\n" "${CWD}"
      exit 1
   fi

   BUILD_DIR="${PWD}/build"
   CMAKE_BUILD_TYPE=Release
   TIME_BEGIN=$( date -u +%s )
   INSTALL_PREFIX="/usr/local/eosio"
   VERSION=1.2

   txtbld=$(tput bold)
   bldred=${txtbld}$(tput setaf 1)
   txtrst=$(tput sgr0)

   create_symlink() {
      pushd /usr/local/bin &> /dev/null
      ln -sf ../eosio/bin/$1 $1
      popd &> /dev/null
   }

   create_cmake_symlink() {
      mkdir -p /usr/local/lib/cmake/eosio
      pushd /usr/local/lib/cmake/eosio &> /dev/null
      ln -sf ../../../eosio/lib/cmake/eosio/$1 $1
      popd &> /dev/null
   }

   install_symlinks() {
      printf "\\n\\tInstalling FIOIO Binary Symlinks\\n\\n"
      create_symlink "cleos"
      create_symlink "eosio-abigen"
      create_symlink "eosio-launcher"
      create_symlink "eosio-s2wasm"
      create_symlink "eosio-wast2wasm"
      create_symlink "eosiocpp"
      create_symlink "keosd"
      create_symlink "nodeos"
   }

   if [ ! -d "${BUILD_DIR}" ]; then
      printf "\\n\\tError, fioio_build.sh has not ran.  Please run ./fioio_build.sh first!\\n\\n"
      exit -1
   fi

   ${PWD}/scripts/clean_old_install.sh
   if [ $? -ne 0 ]; then
      printf "\\n\\tError occurred while trying to remove old installation!\\n\\n"
      exit -1
   fi

   if ! pushd "${BUILD_DIR}"
   then
      printf "Unable to enter build directory %s.\\n Exiting now.\\n" "${BUILD_DIR}"
      exit 1;
   fi
   
   if ! make install
   then
      printf "\\n\\t>>>>>>>>>>>>>>>>>>>> MAKE installing FIOIO has exited with the above error.\\n\\n"
      exit -1
   fi
   popd &> /dev/null 

   install_symlinks   
   create_cmake_symlink "eosio-config.cmake"

   printf "\n\n${white}  FFFFFFFFFFFFFFFFFFF IIIIIIIII     OOOOOOO     IIIIIIIII     OOOOOOO\n"
   printf "  F:::::::::::::::::F I:::::::I   OO::::::::OO  I:::::::I   OO:::::::OO\n"
   printf "  FF:::::FFFFFFFF:::F II:::::II O:::::OOO:::::O II:::::II O:::::OOO:::::O\n"
   printf "    F::::F      FFFFF   I:::I  O:::::O   O:::::O  I:::I  O:::::O   O:::::O\n"
   printf "    F::::F              I:::I  O::::O     O::::O  I:::I  O::::O     O::::O\n"
   printf "    F:::::FFFFFFFFF     I:::I  O::::O     O::::O  I:::I  O::::O     O::::O\n"
   printf "    F:::::::::::::F     I:::I  O::::O     O::::O  I:::I  O::::O     O::::O\n"
   printf "    F:::::FFFFFFFFF     I:::I  O::::O     O::::O  I:::I  O::::O     O::::O\n"
   printf "    F::::F              I:::I  O::::O     O::::O  I:::I  O::::O     O::::O\n"
   printf "    F::::F              I:::I  O:::::O   O:::::O  I:::I  O:::::O   O:::::O\n"
   printf "  FF::::::FF          II:::::II O:::::OOO:::::O II:::::II O:::::OOO:::::O\n"
   printf "  F:::::::FF          I:::::::I   OO:::::::OO   I:::::::I   OO:::::::OO\n"
   printf "  FFFFFFFFFF          IIIIIIIII     OOOOOOO     IIIIIIIII     OOOOO0O \n${txtrst}"

   printf "\\n\\tFIOIO has been successfully installed.\\n"

   printf "\\tFor more information:\\n"
   printf "\\tFIOIO wiki: https://github.com/dapixio/fio/wiki\\n\\n\\n"

   if [ -d node2/ ]; then
    exit -1
   else
    cleos wallet create --to-console

    read -p $"Enter Wallet Password Listed Above: " walletpassword

    echo "$walletpassword" > walletkey.ini
    echo 'Saved'

   fi
