#!/usr/bin/env bash

#FIO Directory Check
if [ -f ../fio.contracts/build/contracts/eosio.bios/eosio.bios.wasm ]; then
    eosio_bios_contract_name_path="$PWD/../fio.contracts/build/contracts/eosio.bios"
else
    echo 'No wasm file found at $PWD/build/contracts/eosio.bios'
fi

if [ -f ../fio.contracts/build/contracts/fio.system/fio.system.wasm ]; then
    fio_system_contract_name_path="$PWD/../fio.contracts/build/contracts/fio.system"
else
    echo 'No wasm file found at $PWD/build/contracts/fio.system'
fi

if [ -f ../fio.contracts/build/contracts/eosio.msig/eosio.msig.wasm ]; then
    eosio_msig_contract_name_path="$PWD/../fio.contracts/build/contracts/eosio.msig"
else
    echo 'No wasm file found at $PWD/build/contracts/eosio.msig'
fi

if [ -f ../fio.contracts/build/contracts/fio.token/fio.token.wasm ]; then
    fio_token_contract_name_path="$PWD/../fio.contracts/build/contracts/fio.token"
else
    echo 'No wasm file found at $PWD/build/contracts/fio.token'
fi
#Fio Name Directory Check
if [ -f ../fio.contracts/build/contracts/fio.address/fio.address.wasm ]; then
    fio_contract_name_path="$PWD/../fio.contracts/build/contracts/fio.address"
else
    echo 'No wasm file found at $PWD/build/contracts/fio.address'
fi

if [ -f ../fio.contracts/build/contracts/fio.fee/fio.fee.wasm ]; then
    fio_fee_name_path="$PWD/../fio.contracts/build/contracts/fio.fee"
else
    echo 'No wasm file found at $PWD/build/contracts/fio.fee'
fi

if [ -f ../fio.contracts/build/contracts/fio.request.obt/fio.request.obt.wasm ]; then
        fio_reqobt_name_path="$PWD/../fio.contracts/build/contracts/fio.request.obt"
    else
        echo 'No wasm file found at $PWD/build/contracts/fio.request.obt'
fi

if [ -f ../fio.contracts/build/contracts/fio.tpid/fio.tpid.wasm ]; then
        fio_tpid_name_path="$PWD/../fio.contracts/build/contracts/fio.tpid"
    else
        echo 'No wasm file found at $PWD/build/contracts/fio.tpid'
fi

if [ -f ../fio.contracts/build/contracts/fio.treasury/fio.treasury.wasm ]; then
        fio_treasury_name_path="$PWD/../fio.contracts/build/contracts/fio.treasury"
    else
        echo 'No wasm file found at $PWD/build/contracts/fio.treasury'
fi

 if [ -f ../fio.contracts/build/contracts/fio.whitelist/fio.whitelist.wasm ]; then
            fio_whitelist_name_path="$PWD/../fio.contracts/build/contracts/fio.whitelist"
        else
            echo 'No wasm file found at $PWD/build/contracts/fio.whitelist'
fi

if [ -f ../fio.contracts/build/contracts/eosio.wrap/eosio.wrap.wasm ]; then
           eosio_wrap_name_path="$PWD/../fio.contracts/build/contracts/eosio.wrap"
        else
            echo 'No wasm file found at $PWD/build/contracts/eosio.wrap'
fi

export eosio_bios_contract_name_path
export fio_system_contract_name_path
export eosio_msig_contract_name_path
export fio_token_contract_name_path
export fio_contract_name_path
export fio_fee_name_path
export fio_reqobt_name_path
export fio_tpid_name_path
export fio_treasury_name_path
export fio_whitelist_name_path
export eosio_wrap_name_path