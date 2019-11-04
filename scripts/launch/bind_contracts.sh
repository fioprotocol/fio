#!/usr/bin/env bash

#create the token on the bios node
./cleos -u http://localhost:8879 push action -j fio.token create '["1000000000.000000000 FIO"]' -p fio.token@active
#load the bios contract on the bios node
./cleos -u http://localhost:8879 set contract eosio $eosio_bios_contract_name_path eosio.bios.wasm eosio.bios.abi

sleep 3s

# Bind more fio contracts
./cleos -u http://localhost:8879 set contract -j fio.reqobt $fio_reqobt_name_path fio.request.obt.wasm fio.request.obt.abi --permission fio.reqobt@active
sleep 3s
./cleos -u http://localhost:8879 set contract -j fio.fee $fio_fee_name_path fio.fee.wasm fio.fee.abi --permission fio.fee@active
sleep 3s
./cleos -u http://localhost:8889 set contract -j fio.treasury $fio_treasury_name_path fio.treasury.wasm fio.treasury.abi --permission fio.treasury@active
sleep 3s
./cleos -u http://localhost:8889 set contract -j fio.whitelst $fio_whitelist_name_path fio.whitelist.wasm fio.whitelist.abi --permission fio.whitelst@active
sleep 3s
./cleos -u http://localhost:8889 set contract -j eosio.wrap $eosio_wrap_name_path eosio.wrap.wasm eosio.wrap.abi --permission eosio.wrap@active
sleep 3s