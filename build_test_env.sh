#!/usr/bin/env bash

printf "\n\n${bldred}  FFFFFFFFFFFFFFFFFFF IIIIIIIII     OOOOOOO     IIIIIIIII     OOOOOOO\n"
printf "  F:::::::::::::::::F I:::::::I   OO::::::::OO  I:::::::I   OO:::::::OO\n"
printf "  FF:::::FFFFFFFF:::F II:::::II O:::::OOO:::::O II:::::II O:::::OOO:::::O\n"
printf "    F::::F      FFFFF   I:::I  O:::::O   O:::::O  I:::I  O:::::O   O:::::O\n"
printf "    F::::F              I:::I  O::::O     O::::O  I:::I  O::::O     O::::O\n"
printf "    F:::::FFFFFFFFF     I:::I  O::::O     O::::O  I:::I  O::::O     O::::O\n"
printf "    F:::::::::::::F     I:::I  O::::O     O::::O  I:::I  O::::O     O::::O\n"
printf "    F:::::FFFFFFFFF     I:::I  O::::O     O::::O  I:::I  O::::O     O::::O\n"
printf "    F::::F              I:::I  O::::O     O::::O  I:::I  O::::O     O::::O\n"
printf "    F::::F              I:::I  O:::::O   O:::::O  I:::I  O:::::O   O:::::O\n"
printf "  FF::::::FF          II:::::II O:::::OOO:::::O II:::::II O:::::OOO:::::O\n"
printf "  F:::::::FF          I:::::::I   OO:::::::OO   I:::::::I   OO:::::::OO\n"
printf "  FFFFFFFFFF          IIIIIIIII     OOOOOOO     IIIIIIIII     OOOOO0O \n${txtrst}"

echo 'Enter Default Wallet Passkey'
cleos wallet unlock

#Import Wallet Keys
cleos wallet import --private-key 5JxUfAkXoCQdeZKNMhXEqRkFcZMYa3KR3vbie7SKsPv6rS3pCHg
cleos wallet import --private-key 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
cleos wallet import --private-key 5KDQzVMaD1iUdYDrA2PNK3qEP7zNbUf8D41ZVKqGzZ117PdM5Ap
cleos wallet import --private-key 5Jr2SxVH6bh6QcJerJrGKvNAp7zfemN98rp4BfzFonkJQmcumvP
cleos wallet import --private-key 5KBX1dwHME4VyuUss2sYM25D5ZTDvyYrbEz37UJqwAVAsR4tGuY

#Start Both Nodes
if ! pgrep -x "nodeos" > /dev/null
then
    sh scripts/node1_start.sh &
    sleep 3s
    sh scripts/node2_start.sh &
fi

sleep 6s

#Create Accounts
pvt_key="EOS6D6gSipBmP1KW9SMB5r4ELjooaogFt77gEs25V9TU9FrxKVeFb"
echo $'Creating Accounts...\n'
echo 'Using Private Key:' $pvt_key
cleos -u http://localhost:8889 create account eosio fioname11111 $pvt_key
cleos -u http://localhost:8889 create account eosio fioname22222 $pvt_key
cleos -u http://localhost:8889 create account eosio fioname33333 $pvt_key
cleos -u http://localhost:8889 create account eosio fio.system $pvt_key
cleos -u http://localhost:8889 create account eosio fio.finance $pvt_key
cleos -u http://localhost:8889 create account eosio fio.fee $pvt_key
cleos -u http://localhost:8889 create account eosio fio.common $pvt_key

#Directory Check
if [ -f /build/contracts/fio.name/fio.name.wasm ]; then
    echo 'No wasm file found at $PWD/build/contracts/fio.name'
    read -p 'Path to Fio Name Contract Folder: ' fio_contract_name_path
else
    fio_contract_name_path="$PWD/build/contracts/fio.name"
fi

#Bind FIO.NAME Contract to Chain
cleos -u http://localhost:8889 set contract -j fio.system $fio_contract_name_path fio.name.wasm fio.name.abi --permission fio.system@active

#Create Domain
cleos -u http://localhost:8889 push action -j fio.system registername '{"name":"brd","requestor":"fioname11111"}' --permission fioname11111@active

#Create Account Name
cleos -u http://localhost:8889 push action -j fio.system registername '{"name":"casey.brd","requestor":"fioname11111"}' --permission fioname11111@active
cleos -u http://localhost:8889 push action -j fio.system registername '{"name":"adam.brd","requestor":"fioname11111"}' --permission fioname11111@active

printf "\n\n${bldred}  FFFFFFFFFFFFFFFFFFF IIIIIIIII     OOOOOOO     IIIIIIIII     OOOOOOO\n"
printf "  F:::::::::::::::::F I:::::::I   OO::::::::OO  I:::::::I   OO:::::::OO\n"
printf "  FF:::::FFFFFFFF:::F II:::::II O:::::OOO:::::O II:::::II O:::::OOO:::::O\n"
printf "    F::::F      FFFFF   I:::I  O:::::O   O:::::O  I:::I  O:::::O   O:::::O\n"
printf "    F::::F              I:::I  O::::O     O::::O  I:::I  O::::O     O::::O\n"
printf "    F:::::FFFFFFFFF     I:::I  O::::O     O::::O  I:::I  O::::O     O::::O\n"
printf "    F:::::::::::::F     I:::I  O::::O     O::::O  I:::I  O::::O     O::::O\n"
printf "    F:::::FFFFFFFFF     I:::I  O::::O     O::::O  I:::I  O::::O     O::::O\n"
printf "    F::::F              I:::I  O::::O     O::::O  I:::I  O::::O     O::::O\n"
printf "    F::::F              I:::I  O:::::O   O:::::O  I:::I  O:::::O   O:::::O\n"
printf "  FF::::::FF          II:::::II O:::::OOO:::::O II:::::II O:::::OOO:::::O\n"
printf "  F:::::::FF          I:::::::I   OO:::::::OO   I:::::::I   OO:::::::OO\n"
printf "  FFFFFFFFFF          IIIIIIIII     OOOOOOO     IIIIIIIII     OOOOO0O \n${txtrst}"
echo $'\nTest Environment Setup Complete\n'
