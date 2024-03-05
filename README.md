## Welcome to the FIO Protocol Github.
FIO Protocol is an open-source project and anyone is welcomed and encouraged to contribute.

## Resources
Before contributing:
* Review [Contributing to FIO](CONTRIBUTING.md)
* For information on FIO Protocol, visit the [FIO website](https://fio.foundation).
* For information on the FIO Chain, API, and SDKs visit the [FIO Protocol Developer Hub](https://developers.fioprotocol.io).
* To join the community, visit [Discord](https://discord.com/invite/pHBmJCc)       

## Build

The build script first installs all dependencies and then builds FIO. The script has several options, including '-P' (pinned build), '-i' (install directory), and '-o' (build type, i.e. Release, Debug, etc.). Providing no options will use default options, i.e. a local build directory, $HOME/fio, as the install directory.

{% include alert.html type="danger" title="An operational FIO build requires clang 8" content="FIO chain requires clang v8 as part of the LLVM requirements. When executing the build, specify '-P' for a 'pinned' build to ensure the correct LLVM versions are used." %}

To build, first change to the `~/fioprotocol/fio` folder, then execute the script as follows:

```shell
cd ~/fioprotocol/fio/scripts
./fio_build.sh -P
```

The build process writes temporary content to the `fio/build` folder. After building, the program binaries can be found at `fio/build/programs`.

## Install

For ease of contract development, FIO will be installed in the `~/fio` folder using the fio_install.sh script within the `fio/scripts` folder. Adequate permission is required to install in system folders, e.g., `/usr/local/bin`.

```shell
cd ~/fioprotocol/fio/scripts
./fio_install.sh
```
