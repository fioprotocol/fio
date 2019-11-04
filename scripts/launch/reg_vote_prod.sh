#!/usr/bin/env bash

./cleos -u http://localhost:8879 push action eosio setprods "{ \"schedule\": [{\"producer_name\": \"3ddowwxs11ss\",\"block_signing_key\": \"FIO6ruJ5qLeaa6VtYVpkcU4AeWVaL2QvViyQqjxjpAWYRFsYaSbBN\"}]}" -p eosio@active

echo loading system contract, the bios node is going to go dormant so load the system contract now.
sleep 1
./cleos -u http://localhost:8879 set contract -j eosio $fio_system_contract_name_path fio.system.wasm fio.system.abi --permission eosio@active
sleep 5
#init the bios node, this must be done using the action in the bios contract.
./cleos -u http://localhost:8879  push action eosio init '{"version": 0, "core":"9,FIO"}' -p eosio@active

echo did eosio init
sleep 5

#register names for the producers.
echo bp1
  ./cleos -u http://localhost:8889 push action -j fio.address regaddress '{"fio_address":"bp1:dapix","owner_fio_public_key":"","max_fee":"40000000000","actor":"qbxn5zhw2ypw","tpid":""}' --permission qbxn5zhw2ypw@active
echo bp2
  ./cleos -u http://localhost:8889 push action -j fio.address regaddress '{"fio_address":"bp2:dapix","owner_fio_public_key":"","max_fee":"40000000000","actor":"hfdg2qumuvlc","tpid":""}' --permission hfdg2qumuvlc@active
echo bp3
  ./cleos -u http://localhost:8889 push action -j fio.address regaddress '{"fio_address":"bp3:dapix","owner_fio_public_key":"","max_fee":"40000000000","actor":"wttywsmdmfew","tpid":""}' --permission wttywsmdmfew@active
echo bp4
  ./cleos -u http://localhost:8889 push action -j fio.address regaddress '{"fio_address":"bp4:dapix","owner_fio_public_key":"","max_fee":"40000000000","actor":"3ddowwxs11ss","tpid":""}' --permission 3ddowwxs11ss@active

#register names for the voters.
echo vote1
  ./cleos -u http://localhost:8889 push action -j fio.address regaddress '{"fio_address":"vote1:dapix","owner_fio_public_key":"","max_fee":"40000000000","actor":"o2ouxipw2rt4","tpid":""}' --permission o2ouxipw2rt4@active
echo vote2
  ./cleos -u http://localhost:8889 push action -j fio.address regaddress '{"fio_address":"vote2:dapix","owner_fio_public_key":"","max_fee":"40000000000","actor":"extjnqh3j3gt","tpid":""}' --permission extjnqh3j3gt@active
echo vote3
   ./cleos -u http://localhost:8889 push action -j fio.address regaddress '{"fio_address":"vote3:dapix","owner_fio_public_key":"","max_fee":"40000000000","actor":"npe3obkgoteh","tpid":""}' --permission npe3obkgoteh@active

echo calling regproducer
#register the desired producers, note this does not yet turn on block production because teh necessary amount of
#fio is not yet in circulation, and there are no votes.
#it just registers these BP.
./cleos -u http://localhost:8879 push action eosio regproducer '{"fio_address":"bp1:dapix","url":"www.yahoo.com","location":10,"actor":"qbxn5zhw2ypw", "max_fee":"40000000000" }' --permission qbxn5zhw2ypw@active
./cleos -u http://localhost:8879 push action eosio regproducer '{"fio_address":"bp2:dapix","url":"www.yahoo.com","location":20,"actor":"hfdg2qumuvlc", "max_fee":"40000000000" }' --permission hfdg2qumuvlc@active
./cleos -u http://localhost:8879 push action eosio regproducer '{"fio_address":"bp3:dapix","url":"www.yahoo.com","location":30,"actor":"wttywsmdmfew", "max_fee":"40000000000" }' --permission wttywsmdmfew@active

sleep 5
./cleos -u http://localhost:8879 system listproducers

echo setting up voters
#ensure a record is in the voters table
echo setting up vote1:dapix
./cleos -u http://localhost:8889 push action -j eosio regproxy '{"fio_address":"vote1:dapix","actor":"o2ouxipw2rt4",,"max_fee":"40000000000"}' --permission o2ouxipw2rt4@active
./cleos -u http://localhost:8889 push action -j eosio unregproxy '{"fio_address":"vote1:dapix","actor":"o2ouxipw2rt4",,"max_fee":"40000000000"}' --permission o2ouxipw2rt4@active

echo setting up vote2:dapix
./cleos -u http://localhost:8889 push action -j eosio regproxy '{"fio_address":"vote2:dapix","actor":"extjnqh3j3gt",,"max_fee":"40000000000"}' --permission extjnqh3j3gt@active
./cleos -u http://localhost:8889 push action -j eosio unregproxy '{"fio_address":"vote2:dapix","actor":"extjnqh3j3gt",,"max_fee":"40000000000"}' --permission extjnqh3j3gt@active

echo setting up vote3:dapix
./cleos -u http://localhost:8889 push action -j eosio regproxy '{"fio_address":"vote3:dapix","actor":"npe3obkgoteh",,"max_fee":"40000000000"}' --permission npe3obkgoteh@active
./cleos -u http://localhost:8889 push action -j eosio unregproxy '{"fio_address":"vote3:dapix","actor":"npe3obkgoteh",,"max_fee":"40000000000"}' --permission npe3obkgoteh@active

echo calling vproducer
#vote for the producers, now block production may occur.
./cleos -u http://localhost:8889 system vproducer prods o2ouxipw2rt4 qbxn5zhw2ypw -p o2ouxipw2rt4@active
./cleos -u http://localhost:8889 system vproducer prods extjnqh3j3gt hfdg2qumuvlc -p extjnqh3j3gt@active
./cleos -u http://localhost:8889 system vproducer prods npe3obkgoteh wttywsmdmfew -p npe3obkgoteh@active