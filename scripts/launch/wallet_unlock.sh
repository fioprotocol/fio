#!/usr/bin/env bash

oldpath=$PWD
cd ~/opt/eosio/bin

walletkey=$(head -n 1 $oldpath/../walletkey.ini)
echo 'Using Password:' $walletkey
sleep 2s
#./keosd --config-dir ~/eosio-wallet --wallet-dir ~/eosio-wallet --http-server-address localhost:8879 --http-alias localhost:8900 > ~/eosio-wallet/stdout.log 2> ~/eosio-wallet/stderr.log &
./cleos wallet unlock -n fio --password $walletkey
