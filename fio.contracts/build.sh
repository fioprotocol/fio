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
cp contracts/fio.name/fio.name.abi build/contracts/fio.name/
cp contracts/fio.fee/fio.fee.abi build/contracts/fio.fee/
cp contracts/fio.request.obt/fio.request.obt.abi build/contracts/fio.request.obt/
cp contracts/fio.finance/fio.finance.abi build/contracts/fio.finance/
