#!/usr/bin/env bash

echo $'Creating Accounts...done using the bios node\n'
./cleos -u http://localhost:8879 create account eosio fio.token FIO7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS FIO7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS
./cleos -u http://localhost:8879 create account eosio fio.address FIO7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS FIO7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS
./cleos -u http://localhost:8879 create account eosio fio.fee FIO7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS FIO7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS
./cleos -u http://localhost:8879 create account eosio fio.reqobt FIO7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS FIO7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS
./cleos -u http://localhost:8879 create account eosio fio.tpid FIO7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS FIO7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS
./cleos -u http://localhost:8879 create account eosio fio.treasury FIO7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS FIO7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS
./cleos -u http://localhost:8879 create account eosio fio.whitelst FIO7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS FIO7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS
./cleos -u http://localhost:8879 create account eosio fio.foundatn FIO7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS FIO7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS
./cleos -u http://localhost:8879 create account eosio eosio.msig FIO7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS FIO7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS
./cleos -u http://localhost:8879 create account eosio eosio.wrap FIO7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS FIO7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS

#Set Contracts..done using the bios node
./cleos -u http://localhost:8879 set contract fio.token $fio_token_contract_name_path fio.token.wasm fio.token.abi
sleep 2s
./cleos -u http://localhost:8879 set contract fio.tpid $fio_tpid_name_path fio.tpid.wasm fio.tpid.abi
./cleos -u http://localhost:8879 set contract eosio.msig $eosio_msig_contract_name_path eosio.msig.wasm eosio.msig.abi
./cleos -u http://localhost:8879 set contract -j fio.address $fio_contract_name_path fio.address.wasm fio.address.abi --permission fio.address@active
sleep 5s