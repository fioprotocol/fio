#! /bin/bash

printf "\t=========== Building FIO system contracts ===========\n\n"

RED='\033[0;31m'
NC='\033[0m'

CORES=`getconf _NPROCESSORS_ONLN`
mkdir -p build
pushd build &> /dev/null
cmake ../
make -j${CORES}
popd &> /dev/null

printf "\t=========== Copying FIO source abi ===========\n\n"
cp contracts/fio.address/fio.address.abi build/contracts/fio.address/
cp contracts/fio.fee/fio.fee.abi build/contracts/fio.fee/
cp contracts/fio.request.obt/fio.request.obt.abi build/contracts/fio.request.obt/
cp contracts/fio.tpid/fio.tpid.abi build/contracts/fio.tpid/
cp contracts/fio.treasury/fio.treasury.abi build/contracts/fio.treasury/
