#!/usr/bin/env bash
dom=1
retries=3

./cleos -u http://localhost:8889 push action -j fio.address regdomain '{"fio_domain":"cryptowallet","owner_fio_public_key":"","max_fee":"40000000000","actor":"5spujqoyq4ie","tpid":""}' --permission 5spujqoyq4ie@active
sleep 5

#Create Account Name
echo casey
./cleos -u http://localhost:8889 push action -j fio.address regaddress '{"fio_address":"casey:dapix","owner_fio_public_key":"","max_fee":"40000000000","actor":"r41zuwovtn44","tpid":""}' --permission r41zuwovtn44@active
echo adam
./cleos -u http://localhost:8889 push action -j fio.address regaddress '{"fio_address":"adam:dapix","owner_fio_public_key":"","max_fee":"40000000000","actor":"htjonrkf1lgs","tpid":""}' --permission htjonrkf1lgs@active
echo ed
./cleos -u http://localhost:8889 push action -j fio.address regaddress '{"fio_address":"ed:dapix","owner_fio_public_key":"","max_fee":"40000000000","actor":"euwdcp13zlrj","tpid":"adam:dapix"}' --permission euwdcp13zlrj@active
echo ada
./cleos -u http://localhost:8889 push action -j fio.address regaddress '{"fio_address":"ada:dapix","owner_fio_public_key":"","max_fee":"40000000000","actor":"r41zuwovtn44","tpid":"adam:dapix"}' --permission r41zuwovtn44@active
echo 5spujqoyq4ie
./cleos -u http://localhost:8889 push action -j fio.address regaddress '{"fio_address":"producera:cryptowallet","owner_fio_public_key":"","max_fee":"40000000000","actor":"5spujqoyq4ie","tpid":"adam:dapix"}' --permission 5spujqoyq4ie@active

#we do these 3 lines to create a record in voter_info for adam:dapix, then we set that record to NOT proxy,
#then we give that record some votes...after doing this we can run the register_proxy signing script and this
#tests the logic when there is already a record in the voters table for this account....
./cleos -u http://localhost:8889 push action -j eosio regproxy '{"fio_address":"adam:dapix","actor":"htjonrkf1lgs",,"max_fee":"40000000000"}' --permission htjonrkf1lgs@active
./cleos -u http://localhost:8889 push action -j eosio unregproxy '{"fio_address":"adam:dapix","actor":"htjonrkf1lgs",,"max_fee":"40000000000"}' --permission htjonrkf1lgs@active
./cleos -u http://localhost:8889 push action -j eosio regproxy '{"fio_address":"casey:dapix","actor":"r41zuwovtn44",,"max_fee":"40000000000"}' --permission r41zuwovtn44@active
./cleos -u http://localhost:8889 push action -j eosio unregproxy '{"fio_address":"casey:dapix","actor":"r41zuwovtn44",,"max_fee":"40000000000"}' --permission r41zuwovtn44@active
