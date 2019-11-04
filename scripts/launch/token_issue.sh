#!/usr/bin/env bash

./cleos -u http://localhost:8879 push action -j fio.token issue '["5spujqoyq4ie","1000.000000000 FIO","memo"]' -p eosio@active
./cleos -u http://localhost:8879 push action -j fio.token issue '["r41zuwovtn44","1000.000000000 FIO","memo"]' -p eosio@active
./cleos -u http://localhost:8879 push action -j fio.token issue '["htjonrkf1lgs","1000.000000000 FIO","memo"]' -p eosio@active
./cleos -u http://localhost:8879 push action -j fio.token issue '["euwdcp13zlrj","2000.000000000 FIO","memo"]' -p eosio@active
./cleos -u http://localhost:8879 push action -j fio.token issue '["mnvcf4v1flnn","4000.000000000 FIO","memo"]' -p eosio@active
./cleos -u http://localhost:8879 push action -j fio.token issue '["fio.treasury","1.000000000 FIO","memo"]' -p eosio@active
./cleos -u http://localhost:8879 push action -j fio.token issue '["fio.token","999.000000000 FIO","memo"]' -p eosio@active

#accounts for BP
./cleos -u http://localhost:8879 push action -j fio.token issue '["qbxn5zhw2ypw","100.000000000 FIO","memo"]' -p eosio@active
./cleos -u http://localhost:8879 push action -j fio.token issue '["hfdg2qumuvlc","100.000000000 FIO","memo"]' -p eosio@active
./cleos -u http://localhost:8879 push action -j fio.token issue '["wttywsmdmfew","100.000000000 FIO","memo"]' -p eosio@active
./cleos -u http://localhost:8879 push action -j fio.token issue '["3ddowwxs11ss","100.000000000 FIO","memo"]' -p eosio@active


#accounts for voters
./cleos -u http://localhost:8879 push action -j fio.token issue '["o2ouxipw2rt4","1000.000000000 FIO","memo"]' -p eosio@active
./cleos -u http://localhost:8879 push action -j fio.token issue '["extjnqh3j3gt","2000.000000000 FIO","memo"]' -p eosio@active
./cleos -u http://localhost:8879 push action -j fio.token issue '["npe3obkgoteh","3000.000000000 FIO","memo"]' -p eosio@active

./cleos -u http://localhost:8879 push action -j fio.token issue '["eosio",       "49980000.000000000 FIO","memo"]' -p eosio@active
./cleos -u http://localhost:8879 push action -j fio.token transfer '{"from": "eosio", "to": "r41zuwovtn44", "quantity": "999999.000000000 FIO", "memo": "launcher test transfer"}' -p eosio@active