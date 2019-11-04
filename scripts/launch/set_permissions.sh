#!/usr/bin/env bash

# Reference: https://github.com/EOSIO/eos/issues/4348#issuecomment-400562839
# Reference: https://developers.eos.io/eosio-home/docs/inline-actions
./cleos -u http://localhost:8879 set account permission fio.token active '{"threshold": 1,"keys": [{"key": "FIO7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS","weight": 1}],"accounts": [{"permission":{"actor":"fio.token","permission":"eosio.code"},"weight":1}]}}' owner -p fio.token@owner
#make the fio.token into a privileged account
./cleos -u http://localhost:8879 push action eosio setpriv '["fio.token",1]' -p eosio@active
./cleos -u http://localhost:8879 set account permission fio.address active '{"threshold": 1,"keys": [{"key": "FIO7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS","weight": 1}],"accounts": [{"permission":{"actor":"fio.address","permission":"eosio.code"},"weight":1}]}}' owner -p fio.address@owner
#make the fio.system into a privileged account
./cleos -u http://localhost:8879 push action eosio setpriv '["fio.address",1]' -p eosio@active
./cleos -u http://localhost:8879 set account permission fio.reqobt active '{"threshold": 1,"keys": [{"key": "FIO7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS","weight": 1}],"accounts": [{"permission":{"actor":"fio.reqobt","permission":"eosio.code"},"weight":1}]}}' owner -p fio.reqobt@owner
#make the fio.reqobt into a privileged account
./cleos -u http://localhost:8879 push action eosio setpriv '["fio.reqobt",1]' -p eosio@active
./cleos -u http://localhost:8879 set account permission fio.whitelst active '{"threshold": 1,"keys": [{"key": "FIO7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS","weight": 1}],"accounts": [{"permission":{"actor":"fio.whitelst","permission":"eosio.code"},"weight":1}]}}' owner -p fio.whitelst@owner
#make the fio.reqobt into a privileged account
./cleos -u http://localhost:8879 push action eosio setpriv '["fio.whitelst",1]' -p eosio@active
#set permissions on tpid and make it a privileged system account, permit cross communication with fio.system
#and other contracts.
./cleos -u http://localhost:8879 set account permission fio.tpid active '{"threshold": 1,"keys": [{"key": "FIO7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS","weight": 1}],"accounts": [{"permission":{"actor":"fio.tpid","permission":"eosio.code"},"weight":1}]}}' owner -p fio.tpid@owner
#make the fio.system into a privileged account
./cleos -u http://localhost:8879 push action eosio setpriv '["fio.tpid",1]' -p eosio@active

./cleos -u http://localhost:8879 set account permission r41zuwovtn44 active '{"threshold":1,"keys":[{"key":"FIO5oBUYbtGTxMS66pPkjC2p8pbA3zCtc8XD4dq9fMut867GRdh82","weight":1}],"accounts":[{"permission":{"actor":"r41zuwovtn44","permission":"eosio.code"},"weight":1}]}' owner -p r41zuwovtn44@owner
./cleos -u http://localhost:8879 push action eosio setpriv '["r41zuwovtn44",1]' -p eosio@active

./cleos -u http://localhost:8889 set account permission fio.treasury active --add-code
sleep 1s
./cleos -u http://localhost:8889 push action fio.treasury startclock '{"":""}' -p fio.treasury@active
