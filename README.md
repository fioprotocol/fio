# FIO
The Foundation for Interwallet Operability (FIO) or, in short, the FIO Protocol, is an open-source project based on EOSIO 1.8+.

* For information on FIO Protocol, visit [FIO](https://fio.net).
* For information on the FIO Chain, API, and SDKs, including detailed clone, build and deploy instructions, visit [FIO Protocol Developer Hub](https://dev.fio.net).
* To get updates on the development roadmap, visit [FIO Improvement Proposals](https://github.com/fioprotocol/fips). Anyone is welcome and encouraged to contribute.
* To contribute, please review [Contributing to FIO](CONTRIBUTING.md)
* To join the community, visit [Discord](https://discord.com/invite/pHBmJCc)

## License
[FIO License](./LICENSE)

## Release Information
[Releases](https://github.com/fioprotocol/fio/releases)

## Build Information

### Compatibility
The FIO blockchain (3.5.1+) is compatible with Ubuntu releases 18, 20 and Ubuntu 22. For Ubuntu 20 and higher, a pinned build (using clang/llvm 8) is required and patches may be applied for build compatibility. The build scripts will detect the OS version automatically and perform the appropriate steps to build and install the FIO blockchain.

### Building
The build and install scripts are located in ./scripts directory.

While there are several options, there are two primary build targets; the default build with no arguments, `./fio_build.sh`, will build a dev-centric FIO blockchain, specifically for development/test, and a release-centric build with the '-P' argument, `./fio_build.sh -P`, which is a pinned build (clang 8) for formal testing and release to the BP community. Execute `./fio_build.sh -h` to output the complete usage.

### Dependencies:
Boost version 1.71.0, Clang (llvm) version 8, and cmake version 3.21.0, are all dependencies of the FIO blockchain. The FIO build script will automatically check for these versions locally and, if not found, will download, build and install them. This will take approximately 1 to 2 hrs, depending on the performance of your build system. All dependency artifacts will be downloaded and built in ~/tmp and used for any future fio blockchain build.

## Build
The build script first installs any dependencies and then builds FIO. The script has several options, including '-P' (pinned build), '-i' (install directory), and '-o' (build type, i.e. Release, Debug, etc.). Providing no options will use default options, i.e. a local build directory, $HOME/fio, as the install directory.

{% include alert.html type="danger" title="An operational FIO build requires clang 8" content="FIO chain requires clang v8 as part of the LLVM requirements. When executing the build, specify '-P' for a 'pinned' build to ensure the correct LLVM versions are used." %}

To build, first change to the `~/fioprotocol/fio` folder, then execute the script as follows:

```shell
cd ~/fioprotocol/fio/scripts
./fio_build.sh -P
```

The build process queries the user for configuration, installs any dependencies, and writes build artifacts to the `build` folder. After building, the program binaries can be found at `build/programs`.

## Install
For ease of contract development, FIO will be installed in the `~/fio` folder using the fio_install.sh script within the `fio/scripts` folder. Adequate permission is required to install in system folders, e.g., `/usr/local/bin`.

```shell
cd ~/fioprotocol/fio/scripts
./fio_install.sh
```