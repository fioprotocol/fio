# Checks for Arch and OS + Support for tests setting them manually
## Necessary for linux exclusion while running bats tests/bash-bats/*.sh
[[ -z "${ARCH}" ]] && export ARCH=$(uname)
if [[ -z "${NAME}" ]]; then
    if [[ $ARCH == "Linux" ]]; then
        [[ ! -e /etc/os-release ]] && echo "${COLOR_RED} - /etc/os-release not found! It seems you're attempting to use an unsupported Linux distribution.${COLOR_NC}" && exit 1
        # Obtain OS NAME, and VERSION
        . /etc/os-release
    elif [[ $ARCH == "Darwin" ]]; then
        export NAME=$(sw_vers -productName)
    else
        echo " ${COLOR_RED}- FIO is not supported for your Architecture!${COLOR_NC}" && exit 1
    fi
fi

# Setup yum and apt variables
if [[ $NAME =~ "Amazon Linux" ]] || [[ $NAME == "CentOS Linux" ]]; then
    if ! YUM=$(command -v yum 2>/dev/null); then echo "${COLOR_RED}YUM must be installed to compile FIO${COLOR_NC}" && exit 1; fi
elif [[ $NAME == "Ubuntu" ]]; then
    if ! APTGET=$(command -v apt-get 2>/dev/null); then echo "${COLOR_RED}APT-GET must be installed to compile FIO${COLOR_NC}" && exit 1; fi
fi

echo VERSION: $VERSION_ID

# Obtain dependency versions; Must come first in the script
. ./scripts/helpers/.environment

# Load general helpers
. ./scripts/helpers/general.sh

function setup() {
    if $VERBOSE; then
        echo "VERBOSE: ${VERBOSE}"
        echo "DRYRUN: ${DRYRUN}"
        echo "TEMP_DIR: ${TEMP_DIR}"
        echo "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}"
        echo "CORE_SYMBOL_NAME: ${CORE_SYMBOL_NAME}"
        echo "BOOST_LOCATION: ${BOOST_LOCATION}"
        echo "INSTALL_LOCATION: ${INSTALL_LOCATION}"
        echo "BUILD_DIR: ${BUILD_DIR}"
        echo "EOSIO_INSTALL_DIR: ${EOSIO_INSTALL_DIR}"
        echo "NONINTERACTIVE: ${NONINTERACTIVE}"
        echo "PROCEED: ${PROCEED}"
        echo "ENABLE_COVERAGE_TESTING: ${ENABLE_COVERAGE_TESTING}"
        echo "ENABLE_DOXYGEN: ${ENABLE_DOXYGEN}"
        echo "ENABLE_MONGO: ${ENABLE_MONGO}"
        echo "INSTALL_MONGO: ${INSTALL_MONGO}"
        echo "SUDO_LOCATION: ${SUDO_LOCATION}"
        echo "PIN_COMPILER: ${PIN_COMPILER}"
    fi
    ([[ -d $BUILD_DIR ]] && [[ -z $BUILD_DIR_CLEANUP_SKIP ]]) && execute rm -rf $BUILD_DIR # cleanup old build directory; support disabling it (Zach requested)

    execute-always mkdir -p $TEMP_DIR
    execute-always mkdir -p $FIO_APTS_DIR

    execute mkdir -p $BUILD_DIR
    execute mkdir -p $SRC_DIR
    execute mkdir -p $OPT_DIR
    execute mkdir -p $VAR_DIR
    execute mkdir -p $BIN_DIR
    execute mkdir -p $VAR_DIR/log
    execute mkdir -p $ETC_DIR
    execute mkdir -p $LIB_DIR

    execute mkdir -p $CLANG_ROOT
    execute mkdir -p $LLVM_ROOT

    if $ENABLE_MONGO; then
        execute mkdir -p $MONGODB_LOG_DIR
        execute mkdir -p $MONGODB_DATA_DIR
    fi

    # remove previous environment param capture
    if [[ -e ./scripts/.build_env ]]; then
        rm -f ./scripts/.build_env
    fi
    touch ./scripts/.build_env
}

function ensure-which() {
    if ! which ls &>/dev/null; then
        while true; do
            [[ $NONINTERACTIVE == false ]] && printf "${COLOR_YELLOW}FIO compiler checks require the 'which' package: Would you like for us to install it? (y/n)?${COLOR_NC}" && read -p " " PROCEED
            echo ""
            case $PROCEED in
            "") echo "What would you like to do?" ;;
            0 | true | [Yy]*)
                install-package which WETRUN
                break
                ;;
            1 | false | [Nn]*)
                echo "${COLOR_RED}Please install the 'which' command before proceeding!${COLOR_NC}"
                exit
                ;;
            *) echo "Please type 'y' for yes or 'n' for no." ;;
            esac
        done
    fi
}

# Prompt user for installation directory.
function install-directory-prompt() {
    if [[ -z $INSTALL_LOCATION ]]; then
        echo "No installation location was specified. Please provide the location where FIO is installed."
        while true; do
            [[ $NONINTERACTIVE == false ]] && printf "${COLOR_YELLOW}Do you wish to use the default location? ${EOSIO_INSTALL_DIR}? (y/n)${COLOR_NC}" && read -p " " PROCEED
            echo ""
            case $PROCEED in
            "") echo "What would you like to do?" ;;
            0 | true | [Yy]*)
                INSTALL_LOCATION=${EOSIO_INSTALL_DIR}
                break
                ;;
            1 | false | [Nn]*)
                printf "Enter the desired installation location." && read -p " " INSTALL_LOCATION
                break
                ;;
            *) echo "Please type 'y' for yes or 'n' for no." ;;
            esac
        done
    fi

    # Support relative paths : https://github.com/EOSIO/eos/issues/7560
    [[ ${INSTALL_LOCATION:0:1} != '/' && ${INSTALL_LOCATION:0:2} != ~[/a-z] ]] && INSTALL_LOCATION=${CURRENT_WORKING_DIR}/${INSTALL_LOCATION}
    [[ ${INSTALL_LOCATION:0:2} == ~[/a-z] ]] && INSTALL_LOCATION="${INSTALL_LOCATION/#\~/$HOME}"
    export INSTALL_LOCATION=$(realpath ${INSTALL_LOCATION})
}

function previous-install-prompt() {
    if [[ -d $INSTALL_LOCATION ]]; then
        echo "FIO has already been installed into ${INSTALL_LOCATION}... It's suggested that you fio_uninstall.sh before re-running this script."
        while true; do
            [[ $NONINTERACTIVE == false ]] && printf "${COLOR_YELLOW}Do you wish to proceed anyway? (y/n)${COLOR_NC}" && read -p " " PROCEED
            echo ""
            case $PROCEED in
            "") echo "What would you like to do?" ;;
            0 | true | [Yy]*)
                break
                ;;
            1 | false | [Nn]*)
                exit
                ;;
            *) echo "Please type 'y' for yes or 'n' for no." ;;
            esac
        done
    fi

    # Now export the eosio install dir and set up the build env
    export EOSIO_INSTALL_DIR=${INSTALL_LOCATION}
    . ./scripts/helpers/.build_vars

    echo "FIO will be installed to: ${EOSIO_INSTALL_DIR}"
}

function resources() {
    echo "${COLOR_CYAN}For more information:"
    echo "${COLOR_CYAN}FIO website: https://fio.net"
    echo "${COLOR_CYAN}FIO Developer Hub: https://dev.fio.net/"
    echo "${COLOR_CYAN}FIO Telegram channel:${COLOR_NC} https://t.me/joinFIO"
    echo "${COLOR_CYAN}FIO Discord:${COLOR_NC} https://discordapp.com/invite/pHBmJCc"
}

function print_supported_linux_distros_and_exit() {
    echo "On Linux the FIO build script only supports Amazon, Centos, and Ubuntu."
    echo "Please install on a supported version of one of these Linux distributions."
    echo "https://aws.amazon.com/amazon-linux-ami/"
    echo "https://www.centos.org/"
    echo "https://www.ubuntu.com/"
    echo "Exiting now."
    exit 1
}

function prompt-mongo-install() {
    if $ENABLE_MONGO; then
        while true; do
            [[ $NONINTERACTIVE == false ]] && printf "${COLOR_YELLOW}You have chosen to include MongoDB support. Do you want for us to install MongoDB as well? (y/n)?${COLOR_NC}" && read -p " " PROCEED
            echo ""
            case $PROCEED in
            "") echo "What would you like to do?" ;;
            0 | true | [Yy]*)
                export INSTALL_MONGO=true
                break
                ;;
            1 | false | [Nn]*)
                echo "${COLOR_RED} - Existing MongoDB will be used.${COLOR_NC}"
                break
                ;;
            *) echo "Please type 'y' for yes or 'n' for no." ;;
            esac
        done
    fi
}

function ensure-compiler() {
    echo "${COLOR_CYAN}[Ensuring compiler support]${COLOR_NC}"
    # Support build-essentials on ubuntu
    if [[ $NAME == "CentOS Linux" ]] || ($PIN_COMPILER && ([[ $VERSION_ID == "18.04" ]] || [[ $VERSION_ID == "20.04" ]] || [[ $VERSION_ID == "22.04" ]])); then
        export CXX=${CXX:-'g++'}
        export CC=${CC:-'gcc'}
    fi
    export CXX=${CXX:-'clang++'}
    export CC=${CC:-'clang'}
    if $PIN_COMPILER || [[ -f $CLANG_ROOT/bin/clang++ ]]; then
        export PIN_COMPILER=true
        export BUILD_CLANG=true
        export CPP_COMP=$CLANG_ROOT/bin/clang++
        export CC_COMP=$CLANG_ROOT/bin/clang
        export PATH=$CLANG_ROOT/bin:$PATH
    elif [[ $PIN_COMPILER == false ]]; then
        which $CXX &>/dev/null || (
            echo "${COLOR_RED}Unable to find $CXX compiler: Pass in the -P option if you wish for us to install it or install a C++17 compiler and set \$CXX and \$CC to the proper binary locations. ${COLOR_NC}"
            exit 1
        )
        # readlink on mac differs from linux readlink (mac doesn't have -f)
        [[ $ARCH == "Linux" ]] && READLINK_COMMAND="readlink -f" || READLINK_COMMAND="readlink"
        COMPILER_TYPE=$(eval $READLINK_COMMAND $(which $CXX) || true)
        if [[ $CXX =~ "clang" ]] || [[ $COMPILER_TYPE =~ "clang" ]]; then
            if [[ $ARCH == "Darwin" ]]; then
                ### Check for apple clang version 10 or higher
                [[ $($(which $CXX) --version | cut -d ' ' -f 4 | cut -d '.' -f 1 | head -n 1) -lt 10 ]] && export NO_CPP17=true
            else
                if [[ $($(which $CXX) --version | cut -d ' ' -f 3 | head -n 1 | cut -d '.' -f1) =~ ^[0-9]+$ ]]; then # Check if the version message cut returns an integer
                    [[ $($(which $CXX) --version | cut -d ' ' -f 3 | head -n 1 | cut -d '.' -f1) < 6 ]] && export NO_CPP17=true
                elif [[ $(clang --version | cut -d ' ' -f 4 | head -n 1 | cut -d '.' -f1) =~ ^[0-9]+$ ]]; then # Check if the version message cut returns an integer
                    [[ $($(which $CXX) --version | cut -d ' ' -f 4 | cut -d '.' -f 1 | head -n 1) < 6 ]] && export NO_CPP17=true
                fi
            fi
        else
            ## Check for c++ version 7 or higher
            [[ $($(which $CXX) -dumpversion | cut -d '.' -f 1) -lt 7 ]] && export NO_CPP17=true
            if [[ $NO_CPP17 == false ]]; then # https://github.com/EOSIO/eos/issues/7402
                while true; do
                    echo "${COLOR_YELLOW}WARNING: Your GCC compiler ($CXX) is less performant than clang (https://github.com/EOSIO/eos/issues/7402). We suggest running the build script with -P or install your own clang and try again.${COLOR_NC}"
                    [[ $NONINTERACTIVE == false ]] && printf "${COLOR_YELLOW}Do you wish to proceed anyway? (y/n)?${COLOR_NC}" && read -p " " PROCEED
                    case $PROCEED in
                    "") echo "What would you like to do?" ;;
                    0 | true | [Yy]*) break ;;
                    1 | false | [Nn]*) exit 1 ;;
                    *) echo "Please type 'y' for yes or 'n' for no." ;;
                    esac
                done
            fi
        fi
    fi
    if $NO_CPP17; then
        while true; do
            echo "${COLOR_YELLOW}Unable to find C++17 support in ${CXX}!${COLOR_NC}"
            echo "If you already have a C++17 compiler installed or would like to install your own, export CXX to point to the compiler of your choosing."
            [[ $NONINTERACTIVE == false ]] && printf "${COLOR_YELLOW}Do you wish to download and build C++17? (y/n)?${COLOR_NC}" && read -p " " PROCEED
            case $PROCEED in
            "") echo "What would you like to do?" ;;
            0 | true | [Yy]*)
                export PIN_COMPILER=true
                export BUILD_CLANG=true
                export CPP_COMP=$CLANG_ROOT/bin/clang++
                export CC_COMP=$CLANG_ROOT/bin/clang
                export PATH=$CLANG_ROOT/bin:$PATH
                break
                ;;
            1 | false | [Nn]*)
                echo "${COLOR_RED} - User aborted C++17 installation!${COLOR_NC}"
                exit 1
                ;;
            *) echo "Please type 'y' for yes or 'n' for no." ;;
            esac
        done
    fi
    $BUILD_CLANG && export PINNED_TOOLCHAIN="-DCMAKE_TOOLCHAIN_FILE='${BUILD_DIR}/pinned_toolchain.cmake'"
    echo ""
}

function ensure-cmake() {
    echo "${COLOR_CYAN}[Ensuring CMAKE installation]${COLOR_NC}"
    if [[ ! -e "${CMAKE}" ]]; then
        if ! is-cmake-built; then
            build-cmake
        fi
        install-cmake
        publish-cmake
        [[ -z "${CMAKE}" ]] && export CMAKE="${CMAKE_ROOT}/bin/cmake"
        echo " - CMAKE successfully installed @ ${CMAKE}"
        echo ""
    else
        echo " - CMAKE found @ ${CMAKE}."
        echo ""
    fi
}

# CMake may be built but is it configured for the same install directory??? applies to other repos as well
function is-cmake-built() {
    if [[ -x ${TEMP_DIR}/cmake-${CMAKE_VERSION}/build/bin/cmake ]]; then
        cmake_version=$(${TEMP_DIR}/cmake-${CMAKE_VERSION}/build/bin/cmake --version | grep version | awk '{print $3}')
        if [[ $cmake_version =~ 3.2 ]]; then
            return
        fi
    fi
    false
}

function build-cmake() {
    execute bash -c "cd ${TEMP_DIR} \
        && rm -rf cmake-${CMAKE_VERSION} \
        && curl -LO https://cmake.org/files/v${CMAKE_VERSION_MAJOR}.${CMAKE_VERSION_MINOR}/cmake-${CMAKE_VERSION}.tar.gz \
        && tar -xzf cmake-${CMAKE_VERSION}.tar.gz \
        && rm -f cmake-${CMAKE_VERSION}.tar.gz \
        && cd cmake-${CMAKE_VERSION} \
        && mkdir build && cd build \
        && ../bootstrap --prefix=${FIO_APTS_DIR} \
        && make -j${JOBS}"
}

function install-cmake() {
    execute bash -c "cd ${TEMP_DIR}/cmake-${CMAKE_VERSION} \
        && cd build \
        && make install"
}

function publish-cmake() {
    execute bash -c "cd ${TEMP_DIR}/cmake-${CMAKE_VERSION} \
        && ./build/bin/cmake --install build --prefix ${CMAKE_ROOT}"
}

function ensure-boost() {
    [[ $ARCH == "Darwin" ]] && export CPATH="$(python-config --includes | awk '{print $1}' | cut -dI -f2):$CPATH" # Boost has trouble finding pyconfig.h
    echo "${COLOR_CYAN}[Ensuring Boost $(echo $BOOST_VERSION | sed 's/_/./g') library installation]${COLOR_NC}"
    BOOSTVERSION=$(grep "#define BOOST_VERSION" "$BOOST_ROOT/include/boost/version.hpp" 2>/dev/null | tail -1 | tr -s ' ' | cut -d\  -f3 || true)
    if [[ "${BOOSTVERSION}" != "${BOOST_VERSION_MAJOR}0${BOOST_VERSION_MINOR}0${BOOST_VERSION_PATCH}" ]]; then
        B2_FLAGS="-q -j${JOBS} --with-iostreams --with-date_time --with-filesystem --with-system --with-program_options --with-chrono --with-test install"
        BOOTSTRAP_FLAGS=""
        if [[ $ARCH == "Linux" ]] && $PIN_COMPILER; then
            B2_FLAGS="toolset=clang cxxflags='-stdlib=libc++ -D__STRICT_ANSI__ -nostdinc++ -I${CLANG_ROOT}/include/c++/v1 -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fpie' linkflags='-stdlib=libc++ -pie' link=static threading=multi --with-iostreams --with-date_time --with-filesystem --with-system --with-program_options --with-chrono --with-test -q -j${JOBS} install"
            BOOTSTRAP_FLAGS="--with-toolset=clang"
        elif $PIN_COMPILER; then
            local SDKROOT="$(xcrun --sdk macosx --show-sdk-path)"
        fi
        if ! is-boost-built; then
            build-boost
        fi
        install-boost
        echo " - Boost library successfully installed @ ${BOOST_ROOT}"
        echo ""
    else
        echo " - Boost library found with correct version @ ${BOOST_ROOT}"
        echo ""
    fi
}

function is-boost-built() {
    if [[ -r ${TEMP_DIR}/boost_${BOOST_VERSION}/boost/version.hpp ]]; then
        BOOSTVERSION=$(grep "#define BOOST_VERSION" "${TEMP_DIR}/boost_${BOOST_VERSION}/boost/version.hpp" 2>/dev/null | tail -1 | tr -s ' ' | cut -d\  -f3 || true)
        if [[ "${BOOSTVERSION}" == "${BOOST_VERSION_MAJOR}0${BOOST_VERSION_MINOR}0${BOOST_VERSION_PATCH}" ]]; then
            cat ${TEMP_DIR}/boost_${BOOST_VERSION}/project-config.jam | grep prefix | grep ${EOSIO_INSTALL_DIR} >/dev/null
            if [[ $? -eq 0 ]]; then
                return
            fi
        fi
    fi
    false
}

function build-boost() {
    execute bash -c "cd ${TEMP_DIR} \
        && rm -rf boost_${BOOST_VERSION} \
        && curl -LO https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION_MAJOR}.${BOOST_VERSION_MINOR}.${BOOST_VERSION_PATCH}/source/boost_${BOOST_VERSION}.tar.bz2 \
        && tar -xjf boost_${BOOST_VERSION}.tar.bz2 \
        && rm -f boost_${BOOST_VERSION}.tar.bz2 \
        && cd boost_${BOOST_VERSION} \
        && SDKROOT="$SDKROOT" ./bootstrap.sh ${BOOTSTRAP_FLAGS} --prefix=${BOOST_ROOT}"
}

function install-boost() {
    execute bash -c "cd ${TEMP_DIR}/boost_${BOOST_VERSION} \
        && SDKROOT="$SDKROOT" ./b2 ${B2_FLAGS} \
        && rm -rf ${BOOST_LINK_LOCATION}"
}

# Prompt user for installation directory.
function prompt-pinned-llvm-build() {
    # Use pinned compiler AND clang not found in install dir AND a previous pinned clang build was found
    if [[ ! -d $LLVM_ROOT && ($PIN_COMPILER == true || $BUILD_CLANG == true) ]]; then
        if is-llvm-built && ! $NONINTERACTIVE; then
            while true; do
                printf "\n${COLOR_YELLOW}A pinned llvm build was found in ${TEMP_DIR}/${LLVM_NAME}. Do you wish to use this as the FIO llvm? (y/n)${COLOR_NC}" && read -p " " PROCEED
                case $PROCEED in
                "") echo "What would you like to do?" ;;
                0 | true | [Yy]*)
                    break
                    ;;
                1 | false | [Nn]*)
                    echo "Removing previous llvm 4 build..."
                    rm -rf ${TEMP_DIR}/${LLVM_NAME}
                    break
                    ;;
                *) echo "Please type 'y' for yes or 'n' for no." ;;
                esac
            done
        fi
    fi
}

function ensure-llvm() {
    echo "${COLOR_CYAN}[Ensuring LLVM 4 support]${COLOR_NC}"
    if ! is-llvm-published; then
        if [[ $NAME == "Ubuntu" && $PIN_COMPILER == false && $BUILD_CLANG == false ]]; then
            execute ln -s /usr/lib/llvm-4.0 ${LLVM_ROOT}
            echo " - LLVM successfully linked from /usr/lib/llvm-4.0 to ${LLVM_ROOT}"
        else
            if ! is-llvm-built; then
                CMAKE_FLAGS="-DCMAKE_INSTALL_PREFIX=${FIO_APTS_DIR} -DLLVM_TARGETS_TO_BUILD=host -DLLVM_BUILD_TOOLS=false -DLLVM_ENABLE_RTTI=1 -DCMAKE_BUILD_TYPE=Release"
                if $PIN_COMPILER || $BUILD_CLANG; then
                    CMAKE_FLAGS="${CMAKE_FLAGS} -DCMAKE_TOOLCHAIN_FILE=${BUILD_DIR}/pinned_toolchain.cmake .."
                else
                    CMAKE_FLAGS="${CMAKE_FLAGS} -G 'Unix Makefiles' .."
                fi
                clean-llvm
                build-llvm
            fi
            install-llvm
            publish-llvm
            echo " - LLVM successfully installed @ ${LLVM_ROOT}"
            echo
        fi
    else
        echo " - LLVM found @ ${LLVM_ROOT}"
        echo
    fi
}

function is-llvm-built() {
    if [[ -x ${TEMP_DIR}/${LLVM_NAME}/build/bin/llvm-tblgen ]]; then
        llvm_version=$(${TEMP_DIR}/${LLVM_NAME}/build/bin/llvm-tblgen --version | grep version | awk '{print $3}')
        if [[ $llvm_version =~ 4 ]]; then
            return
        fi
    fi
    false
}

function is-llvm-published() {
    if [[ -x ${LLVM_ROOT}/bin/llvm-tblgen ]]; then
        llvm_version=$(${LLVM_ROOT}/bin/llvm-tblgen --version | grep version | awk '{print $3}')
        if [[ $llvm_version =~ 4 ]]; then
            return
        fi
    fi
    false
}

function clean-llvm() {
    execute bash -c "rm -rf ${TEMP_DIR}/${LLVM_NAME}"
}

function build-llvm() {
    execute bash -c "cd ${TEMP_DIR} \
        && git clone --depth 1 --single-branch --branch $PINNED_LLVM_VERSION https://github.com/llvm-mirror/llvm.git ${LLVM_NAME} \
        && cd ${LLVM_NAME} \
        && mkdir build && cd build \
        && ${CMAKE} ${CMAKE_FLAGS} \
        && make -j${JOBS}"
}

function install-llvm() {
    execute bash -c "cd ${TEMP_DIR}/${LLVM_NAME} \
        && cd build \
        && make install"
}

function publish-llvm() {
    execute bash -c "cd ${TEMP_DIR}/${LLVM_NAME} \
        && ${CMAKE} --install build --prefix ${LLVM_ROOT}"
}

# Prompt user for installation directory.
function prompt-pinned-clang-build() {
    # Use pinned compiler AND clang not found in install dir AND a previous pinned clang build was found
    if ! ${PIN_COMPILER}; then
        return
    fi

    if ! is-clang-published; then
        if is-clang-built && ! $NONINTERACTIVE; then
            while true; do
                printf "\n${COLOR_YELLOW}A pinned clang build was found in $TEMP_DIR/clang8. Do you wish to use this as the FIO clang? (y/n)${COLOR_NC}" && read -p " " PROCEED
                case $PROCEED in
                "") echo "What would you like to do?" ;;
                0 | true | [Yy]*)
                    break
                    ;;
                1 | false | [Nn]*)
                    echo "Removing previous clang 8 build..."
                    rm -rf ${TEMP_DIR}/${CLANG_NAME}
                    break
                    ;;
                *) echo "Please type 'y' for yes or 'n' for no." ;;
                esac
            done
        fi
    fi
}

function ensure-clang() {
    if $BUILD_CLANG; then
        echo "${COLOR_CYAN}[Ensuring Clang support]${COLOR_NC}"
        if ! is-clang-published; then
            # Check tmp dir for previous clang build
            if ! is-clang-built; then
                clean-clang
                clone-clang
                if [[ $NAME == "Ubuntu" ]]; then
                    if [[ $VERSION_ID == "20.04" ]]; then
                        apply-clang-ubuntu20-patches
                    fi
                    if [[ $VERSION_ID == "22.04" ]]; then
                        apply-clang-ubuntu22-patches
                    fi
                fi
                build-clang
            fi
            install-clang
            publish-clang
            echo " - Clang 8 successfully installed @ ${CLANG_ROOT}"
            echo ""
        else
            echo " - Clang 8 found @ ${CLANG_ROOT}"
            echo ""
        fi
        export CXX=$CPP_COMP
        export CC=$CC_COMP
    fi
}

function is-clang-built() {
    if [[ -x ${TEMP_DIR}/${CLANG_NAME}/build/bin/clang ]]; then
        clang_version=$(${TEMP_DIR}/${CLANG_NAME}/build/bin/clang --version | grep version | awk '{print $3}')
        if [[ $clang_version =~ 8 ]]; then
            return
        fi
    fi
    false
}

function is-clang-published() {
    if [[ -x ${CLANG_ROOT}/bin/clang ]]; then
        clang_version=$(${CLANG_ROOT}/bin/clang --version | grep version | awk '{print $3}')
        if [[ $clang_version =~ 8 ]]; then
            return
        fi
    fi
    false
}

function clean-clang() {
    execute bash -c "rm -rf ${TEMP_DIR}/${CLANG_NAME}"
}

function clone-clang() {
    execute bash -c "cd ${TEMP_DIR} \
        && git clone --single-branch --branch $PINNED_COMPILER_BRANCH https://github.com/llvm-mirror/llvm.git ${CLANG_NAME} \
        && cd ${CLANG_NAME} && git checkout $PINNED_COMPILER_LLVM_COMMIT \
        && cd tools \
        && git clone --single-branch --branch $PINNED_COMPILER_BRANCH https://github.com/llvm-mirror/lld.git \
        && cd lld && git checkout $PINNED_COMPILER_LLD_COMMIT && cd ../ \
        && git clone --single-branch --branch $PINNED_COMPILER_BRANCH https://github.com/llvm-mirror/polly.git \
        && cd polly && git checkout $PINNED_COMPILER_POLLY_COMMIT && cd ../ \
        && git clone --single-branch --branch $PINNED_COMPILER_BRANCH https://github.com/llvm-mirror/clang.git clang && cd clang \
        && git checkout $PINNED_COMPILER_CLANG_COMMIT \
        && patch -p2 < \"$REPO_ROOT/patches/clang-devtoolset8-support.patch\" \
        && cd tools && mkdir extra && cd extra \
        && git clone --single-branch --branch $PINNED_COMPILER_BRANCH https://github.com/llvm-mirror/clang-tools-extra.git \
        && cd clang-tools-extra && git checkout $PINNED_COMPILER_CLANG_TOOLS_EXTRA_COMMIT && cd .. \
        && cd ../../../../projects \
        && git clone --single-branch --branch $PINNED_COMPILER_BRANCH https://github.com/llvm-mirror/libcxx.git \
        && cd libcxx && git checkout $PINNED_COMPILER_LIBCXX_COMMIT && cd ../ \
        && git clone --single-branch --branch $PINNED_COMPILER_BRANCH https://github.com/llvm-mirror/libcxxabi.git \
        && cd libcxxabi && git checkout $PINNED_COMPILER_LIBCXXABI_COMMIT && cd ../ \
        && git clone --single-branch --branch $PINNED_COMPILER_BRANCH https://github.com/llvm-mirror/libunwind.git \
        && cd libunwind && git checkout $PINNED_COMPILER_LIBUNWIND_COMMIT && cd ../ \
        && git clone --single-branch --branch $PINNED_COMPILER_BRANCH https://github.com/llvm-mirror/compiler-rt.git \
        && cd compiler-rt && git checkout $PINNED_COMPILER_COMPILER_RT_COMMIT"
}

function build-clang() {
    execute bash -c "cd ${TEMP_DIR}/${CLANG_NAME} \
        && mkdir build && cd build \
        && ${CMAKE} -G 'Unix Makefiles' -DCMAKE_INSTALL_PREFIX='${FIO_APTS_DIR}' -DLLVM_BUILD_EXTERNAL_COMPILER_RT=ON -DLLVM_BUILD_LLVM_DYLIB=ON -DLLVM_ENABLE_LIBCXX=ON -DLLVM_ENABLE_RTTI=ON -DLLVM_INCLUDE_DOCS=OFF -DLLVM_OPTIMIZED_TABLEGEN=ON -DLLVM_TARGETS_TO_BUILD=all -DCMAKE_BUILD_TYPE=Release .. \
        && make -j${JOBS}"
}

function install-clang() {
    execute bash -c "cd ${TEMP_DIR}/${CLANG_NAME} \
        && cd build \
        && make install"
}

function publish-clang() {
    execute bash -c "cd ${TEMP_DIR}/${CLANG_NAME} \
        && ${CMAKE} --install build --prefix ${CLANG_ROOT}"
}

function apply-clang-ubuntu20-patches() {
    echo && echo "Applying glibc 2.31 patch to clang for ubuntu 20+..."
    execute bash -c "cd ${TEMP_DIR}/${CLANG_NAME}/projects/compiler-rt \
        && git checkout -- . \
        && git apply \"$REPO_ROOT/patches/clang8-sanitizer_common-glibc2.31-947f969.patch\""
}

function apply-clang-ubuntu22-patches() {
    apply-clang-ubuntu20-patches

    echo && echo "Applying standard headers patch and limits patch to clang for ubuntu 22..."
    execute bash -c "cd ${TEMP_DIR}/${CLANG_NAME} \
        && git checkout -- . \
        && git apply \"$REPO_ROOT/patches/clang8-standardheaders.patch\" \
        && git apply \"$REPO_ROOT/patches/clang8-limits.patch\""
}

function apply-fio-ubuntu22-patches() {
    apply-fio-fc-ubuntu22-patches
    apply-fio-yubihsm-ubuntu22-patches
}

function apply-fio-fc-ubuntu22-patches() {
    echo && echo "Applying openssl compat patch to fio fc submodule for ubuntu 22..."
    execute bash -c "cd ${REPO_ROOT}/libraries/fc \
        && git checkout -- . \
        && git apply \"$REPO_ROOT/patches/fc-openssl-compat.patch\""
}

function apply-fio-yubihsm-ubuntu22-patches() {
    echo && echo "Applying openssl compat patch to fio yubihsm submodule for ubuntu 22..."
    execute bash -c "cd ${REPO_ROOT}/libraries/yubihsm \
        && git checkout -- . \
        && git apply \"$REPO_ROOT/patches/yubihsm-openssl-compat.patch\""
}

function echo-to-envfile() {
    local env_name="${1}"
    local env_value="${2}"

    echo "export ${env_name}=${env_value}" >>./scripts/.build_env
}
