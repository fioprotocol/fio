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

echo 'Welcome to the Basic Environment'

restartneeded=0

if [ -d /node2 ]; then
    echo 'Restart Detected'
    restartneeded=1
fi

read -p $'1. Local Blockchain ( No SDK Support ) 2. AWS Launch\n3. MacOS (Test) Install 4. Nuke All\nChoose(#):' mChoice

#Fio Name Directory Check
if [ -f /build/contracts/fio.name/fio.name.wasm ]; then
    echo 'No wasm file found at $PWD/build/contracts/fio.name'
    read -p 'Path to Fio Name Contract Folder: ' fio_contract_name_path
else
    fio_contract_name_path="$PWD/build/contracts/fio.name"
fi

#Fio Finance Directory Check
if [ -f /build/contracts/fio.finance/fio.finance.wasm ]; then
    echo 'No wasm file found at $PWD/build/contracts/fio.finance'
    read -p 'Path to Fio Finance Contract Folder: ' fio_finance_contract_name_path
else
    fio_finance_contract_name_path="$PWD/build/contracts/fio.finance"
fi

#EOSIO Directory Check
if [ -f /build/contracts/eosio.bios/eosio.bios.wasm ]; then
    echo 'No wasm file found at $PWD/build/contracts/eosio.bios'
    read -p 'Path to EOSIO.Token Contract Folder: ' eosio.bios_contract_name_path
else
    eosio_bios_contract_name_path="$PWD/build/contracts/eosio.bios"
fi

if [ -f /build/contracts/eosio.token/eosio.token.wasm ]; then
    echo 'No wasm file found at $PWD/build/contracts/eosio.token'
    read -p 'Path to EOSIO.Token Contract Folder: ' eosio.token_contract_name_path
else
    eosio_token_contract_name_path="$PWD/build/contracts/eosio.token"
fi

if [ $mChoice == 1 ] && [ $restartNeeded == 0 ]; then
    echo 'Welcome to the Basic Environment'

    walletkey=$(head -n 1 walletkey.ini)

    cleos wallet unlock --password $walletkey

    #Import Wallet Keys
    cleos wallet import --private-key 5JxUfAkXoCQdeZKNMhXEqRkFcZMYa3KR3vbie7SKsPv6rS3pCHg
    cleos wallet import --private-key 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
    cleos wallet import --private-key 5KDQzVMaD1iUdYDrA2PNK3qEP7zNbUf8D41ZVKqGzZ117PdM5Ap
    cleos wallet import --private-key 5Jr2SxVH6bh6QcJerJrGKvNAp7zfemN98rp4BfzFonkJQmcumvP
    cleos wallet import --private-key 5KBX1dwHME4VyuUss2sYM25D5ZTDvyYrbEz37UJqwAVAsR4tGuY
    cleos wallet import --private-key 5JLxoeRoMDGBbkLdXJjxuh3zHsSS7Lg6Ak9Ft8v8sSdYPkFuABF
    cleos wallet import --private-key 5HvaoRV9QrbbxhLh6zZHqTzesFEG5vusVJGbUazFi5xQvKMMt6U
    cleos wallet import --private-key 5JCpqkvsrCzrAC3YWhx7pnLodr3Wr9dNMULYU8yoUrPRzu269Xz
    cleos wallet import --private-key 5K3s1yePVjrsCJbeyoXGAfJn9yCKKZi6E9oyw59mt5LaJG3Bm1N
    cleos wallet import --private-key 5JAs6QTmEyVjJGbQf1e5MjWLoNqcUAMyZLbLwvDhaG3gYBawDwK
    cleos wallet import --private-key 5JvWX1RkiaRS7jgMTb9oZsFKy77jJHkzEv78JRFkjxdDQR3d6wN
    cleos wallet import --private-key 5JKQTYUjxWQbWt53z6Prrb3PUNtEiG5GcyXv3yHTK1nJaxzABrk
    cleos wallet import --private-key 5K7Dah3CB17j6hpQ5vnBebN2yzGkt5K6A2xV7KshKyAfAUJyY8J
    cleos wallet import --private-key 5HvvvAF32bADDzyqcc3kRN4GVKGAZdVFXQexDScJEqTnpk29Kdv
    cleos wallet import --private-key 5JUgV5evcQGJkniK7rH1HckBAvRKsJP1Zgbv39UWnyTBEkMyWMj
    cleos wallet import --private-key 5K2HBexbraViJLQUJVJqZc42A8dxkouCmzMamdrZsLHhUHv77jF
    cleos wallet import --private-key 5JA5zQkg1S59swPzY6d29gsfNhNPVCb7XhiGJAakGFa7tEKSMjT
    cleos wallet import --private-key 5KQPk78a4Srm11NnuxGnWGNJuZtGaMErQ31XFDvXwzyS3Ru7AJQ
    cleos wallet import --private-key 5JqMPkY5AZMYe4LSu1byGPbyjwin9uE9uB3a14WC8ULtYvjaJzE
    cleos wallet import --private-key 5KcNHxGqm9csmFq8VPa5dGHZ99H8qDGj5F5yWgg6u3D4PUkUoEp
    cleos wallet import --private-key 5KDDv35PHWGkReThhNsj2j23DCJzvWTLvZbSTUj8BFNicrJXPqG
    cleos wallet import --private-key 5JHsotwiGMAjrTu3oCiXYjzJ3dEz9mU7HFc4UigvWgEksbpaVkc
    cleos wallet import --private-key 5K8dPJgVYWDxfEURh1cvrRzqc7uvDLqNHjxs83Rf2n8Hu7rqhmp
    cleos wallet import --private-key 5JDGpznJSJw8cKQYtEuctobAu2zVmD3Rw3fWwPzDuF4XbriseLy
    cleos wallet import --private-key 5JiSAASb4PZTajemTr4KTgeNBWUxrz6SraHGUvNihSskDA9aY5b
    cleos wallet import --private-key 5JwttkBMpCijBWiLx75hHTkYGgDm5gmny7nvnss4s1FoZWPxNui

    #Start Both Nodes
    if pgrep -x "nodeos" > /dev/null
    then
        sh scripts/nodeos_kill.sh
    fi

    sh scripts/node1_start.sh &
    sleep 3s
    sh scripts/node2_start.sh &
    sleep 6s

    #Create Accounts
    echo $'Creating Accounts...\n'
    cleos -u http://localhost:8889 create account eosio fioname11111 EOS5oBUYbtGTxMS66pPkjC2p8pbA3zCtc8XD4dq9fMut867GRdh82 EOS5GpUwQtFrfvwqxAv24VvMJFeMHutpQJseTz8JYUBfZXP2zR8VY
    cleos -u http://localhost:8889 create account eosio fioname22222 EOS7uRvrLVrZCbCM2DtCgUMospqUMnP3JUC1sKHA8zNoF835kJBvN EOS8ApHc48DpXehLznVqMJgMGPAaJoyMbFJbfDLyGQ5QjF7nDPuvJ
    cleos -u http://localhost:8889 create account eosio fioname33333 EOS8NToQB65dZHv28RXSBBiyMCp55M7FRFw6wf4G3GeRt1VsiknrB EOS8JzoVTmdFCnjs7x2qEq7A4cKgfRatvnohgngUPnZs8XfeFknjL
    cleos -u http://localhost:8889 create account eosio fio.token EOS7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS EOS7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS
    cleos -u http://localhost:8889 create account eosio fio.system EOS7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS EOS7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS
    cleos -u http://localhost:8889 create account eosio fio.fee EOS7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS EOS7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS
    cleos -u http://localhost:8889 create account eosio fio.finance EOS7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS EOS7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS

    #Bind FIO.NAME Contract to Chain
    cleos -u http://localhost:8889 set contract -j fio.system $fio_contract_name_path fio.name.wasm fio.name.abi --permission fio.system@active

    #Bind fio.finance Contract to Chain
    cleos -u http://localhost:8889 set contract -j fio.finance $fio_finance_contract_name_path fio.finance.wasm fio.finance.abi --permission fio.finance@active

    #Bind EOSIO.Token Contract to Chain
    cleos -u http://localhost:8889 set contract eosio $eosio_bios_contract_name_path eosio.bios.wasm eosio.bios.abi
    cleos -u http://localhost:8889 set contract eosio $eosio_token_contract_name_path eosio.token.wasm eosio.token.abi

    #Create Domain
    cleos -u http://localhost:8889 push action -j fio.system registername '{"name":"brd","requestor":"fio.system"}' --permission fio.system@active

    #Create Account Name
    cleos -u http://localhost:8889 push action -j fio.system registername '{"name":"casey.brd","requestor":"fioname11111"}' --permission fioname11111@active
    cleos -u http://localhost:8889 push action -j fio.system registername '{"name":"adam.brd","requestor":"fioname11111"}' --permission fioname11111@active

elif [ $mChoice == 2 ]; then
    cd tests
    SOURCE="bootstrap"
    DESTINATION="../build/"

    cp -r "$SOURCE"* "$DESTINATION"
    cp launcher.py ../build/
    cd ..
    cd build/
    python3 ./launcher.py
    cd ..

    sleep 3s

    cleos -u http://localhost:8889 --wallet-url http://localhost:9899 set contract -j fio.system $fio_contract_name_path fio.name.wasm fio.name.abi --permission fio.system@active
    cleos -u http://localhost:8889 --wallet-url http://localhost:9899 set contract -j fio.finance $fio_finance_contract_name_path fio.finance.wasm fio.finance.abi --permission fio.finance@active

    cleos -u http://0.0.0.0:8889 --wallet-url http://0.0.0.0:9899 push action -j fio.system registername '{"name":"brd","requestor":"fioname11111"}' --permission fioname11111@active

elif [ $mChoice == 3 ] || [ ]; then
    cd build
    python3 ./tests/startupNodeos.py -v

    sleep 3s

    cd ..

    #Bind FIO.NAME Contract to Chain
    cleos -u http://localhost:8889 --wallet-url http://localhost:9899 set contract -j fio.system $fio_contract_name_path fio.name.wasm fio.name.abi --permission fio.system@active

    #Bind fio.finance Contract to Chain
    cleos -u http://localhost:8889 --wallet-url http://localhost:9899 set contract -j fio.finance $fio_finance_contract_name_path fio.finance.wasm fio.finance.abi --permission fio.finance@active

    #Create Domain
    cleos -u http://localhost:8889 --wallet-url http://localhost:9899 push action -j fio.system registername '{"name":"brd","requestor":"fioname11111"}' --permission fioname11111@active

elif [ $mChoice == 4 ]; then
    read -p $'WARNING: ALL FILES ( WALLET & CHAIN ) WILL BE DELETED\n\nContinue? (1. No 2. Yes): ' bChoice

    if [ $bChoice == 2 ]; then
        cd scripts
        sh ./chain_nuke.sh
        find . -name eosio-wallet -type d -exec rm -r {} +

        nodeos --hard-replay

        echo $'\n\nNUKE COMPLETE - WELCOME TO YOUR NEW BUILD'
    fi

    exit 1
else
    exit 1
fi

printf "\n\n${bldgrn}  FFFFFFFFFFFFFFFFFFF IIIIIIIII     OOOOOOO     IIIIIIIII     OOOOOOO\n"
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
echo $'\nTest Environment Setup Completed\n'