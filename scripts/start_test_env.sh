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

echo 'Welcome to the Custom Environment'
echo $'\nEnter Default Wallet Passkey'
cleos wallet unlock

#Start Both Nodes
if pgrep -x "nodeos" > /dev/null
then
    sh nodeos_kill.sh
fi

sh node1_start.sh &
sleep 3s
sh node2_start.sh &
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
cd ..
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


