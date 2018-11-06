#!/bin/bash
#set -x

MY_INTERFACE=$1

if [ "$MY_INTERFACE" == "-?" ] || [ "$MY_INTERFACE" == "-h" ] || [ "$MY_INTERFACE" == "--help" ]; then
    echo "USAGE: $0 <local interface name>"
    exit 0
fi

if [ "$MY_INTERFACE" == "" ]; then
    MY_INTERFACE=localhost
fi

# TBD prod conditional
# PORT=8889
# read -p "Press any key to setup EOS node to listen on host \"$MY_INTERFACE\" port \"$PORT\""
PORT=8888
echo Starting up EOS node to listen on host \"$MY_INTERFACE\" port \"$PORT\"
#read -p "Press any key to setup EOS node to listen on host \"$MY_INTERFACE\" port \"$PORT\""

SCRIPT_PATH=`dirname $0`
LAUNCHER_SCRIPT=bios_boot.sh
STAGING_DIR=staging
CONFIG_BIOS=$STAGING_DIR/etc/eosio/node_bios/config.ini
CONFIG_00=$STAGING_DIR/etc/eosio/node_00/config.ini

echo Killing and deleting artifacts from previous runs...
pkill nodeos
rm -rf bios_boot.sh
rm -rf staging

echo Generating host specific configuration files...
cp ${SCRIPT_PATH}/bios_boot.sh-template bios_boot.sh
cp -ar ${SCRIPT_PATH}/staging-template staging

sed -i "s/HTTP-SERVER-ADDRESS/$MY_INTERFACE/g" $LAUNCHER_SCRIPT
if [ $? -ne 0 ]; then
    echo "sed 1 failed"
    exit 1
fi
sed -i "s/HTTP-SERVER-ADDRESS/$MY_INTERFACE/g" $CONFIG_BIOS
if [ $? -ne 0 ]; then
    echo "sed 2 failed"
    exit 1
fi
sed -i "s/HTTP-SERVER-ADDRESS/$MY_INTERFACE/g" $CONFIG_00
if [ $? -ne 0 ]; then
    echo "sed 3 failed"
    exit 1
fi

# run launcher
echo Launching EOS nodes
programs/eosio-launcher/eosio-launcher --nogen -p 1 -n 1 -s mesh -d 1 -i 2018-09-12T16:21:19.132 -f --p2p-plugin net --nodeos '--max-transaction-time 50000 --abi-serializer-max-time-ms 990000 --filter-on * --p2p-max-nodes-per-host 1 --contracts-console'
if [ $? -ne 0 ]; then
    echo "Launcher failed"
    exit 1
fi

echo Configuring EOS nodes
bash bios_boot.sh
if [ $? -ne 0 ]; then
    echo "EOS configuration failed"
    exit 1
fi

exit 0
