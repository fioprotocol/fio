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
oldpath=$PWD

if [ -f ../walletkey.ini ]; then
    echo $'Restart Detected\n'
    restartneeded=1
fi

if [ -z "$1" ]; then
    read -p $'1. Local Blockchain 2. Build Contracts 3. Nuke All\nChoose(#):' mChoice
else
    mChoice=$1
fi

if [ $mChoice == 2 ]; then
    pwd
    cd ../fio.contracts
    ./build.sh
fi

#Fio Name Directory Check
if [ -f ../fio.contracts/build/contracts/fio.name/fio.name.wasm ]; then
    echo 'No wasm file found at $PWD/build/contracts/fio.name'
    read -p 'Path to Fio Name Contract Folder: ' fio_contract_name_path
else
    fio_contract_name_path="$PWD/build/contracts/fio.name"
fi

#Fio fee Directory Check
if [ -f ../fio.contracts/build/contracts/fio.fee/fio.fee.wasm ]; then
    echo 'No wasm file found at $PWD/build/contracts/fio.fee'
    read -p 'Path to Fio Name Contract Folder: ' fio_fee_name_path
else
    fio_fee_name_path="$PWD/build/contracts/fio.fee"
fi

#Fio request obt Directory Check
if [ -f ../fio.contracts/build/contracts/fio.request.obt/fio.request.obt.wasm ]; then
    echo 'No wasm file found at $PWD/build/contracts/fio.request.obt'
    read -p 'Path to Fio request obt Contract Folder: ' fio_request_obt_path
else
    fio_request_obt_path="$PWD/build/contracts/fio.request.obt"
fi

#Fio Finance Directory Check
if [ -f ../fio.contracts/build/contracts/fio.finance/fio.finance.wasm ]; then
    echo 'No wasm file found at $PWD/build/contracts/fio.finance'
    read -p 'Path to Fio Finance Contract Folder: ' fio_finance_contract_name_path
else
    fio_finance_contract_name_path="$PWD/build/contracts/fio.finance"
fi

#EOSIO Directory Check
if [ -f ../fio.contracts/build/contracts/eosio.bios/eosio.bios.wasm ]; then
    echo 'No wasm file found at $PWD/build/contracts/eosio.bios'
    read -p 'Path to EOSIO.Token Contract Folder: ' eosio.bios_contract_name_path
else
    eosio_bios_contract_name_path="$PWD/build/contracts/eosio.bios"
fi

if [ -f ../fio.contracts/build/contracts/eosio.token/eosio.token.wasm ]; then
    echo 'No wasm file found at $PWD/build/contracts/eosio.token'
    read -p 'Path to EOSIO.Token Contract Folder: ' eosio.token_contract_name_path
else
    eosio_token_contract_name_path="$PWD/build/contracts/eosio.token"
fi

cd ~/opt/eosio/bin

if [ $mChoice == 1 ]; then

    if [ ! -f $oldpath/../walletkey.ini ]; then
        ./keosd --config-dir ~/eosio-wallet --wallet-dir ~/eosio-wallet --http-server-address localhost:8879 --http-alias localhost:8900 > ~/eosio-wallet/stdout.log 2> ~/eosio-wallet/stderr.log &
        ./cleos wallet create -n fio -f $oldpath/../walletkey.ini
    else
        walletkey=$(head -n 1 $oldpath/../walletkey.ini)
        echo 'Using Password:' $walletkey
        sleep 2s
        ./keosd --config-dir ~/eosio-wallet --wallet-dir ~/eosio-wallet --http-server-address localhost:8879 --http-alias localhost:8900 > ~/eosio-wallet/stdout.log 2> ~/eosio-wallet/stderr.log &
        ./cleos wallet unlock -n fio --password $walletkey
    fi

    if [ $restartneeded == 0 ]; then
        #Import Wallet Keys
        ./cleos wallet import --private-key 5JxUfAkXoCQdeZKNMhXEqRkFcZMYa3KR3vbie7SKsPv6rS3pCHg -n fio
        ./cleos wallet import --private-key 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3 -n fio
        ./cleos wallet import --private-key 5KDQzVMaD1iUdYDrA2PNK3qEP7zNbUf8D41ZVKqGzZ117PdM5Ap -n fio
        ./cleos wallet import --private-key 5Jr2SxVH6bh6QcJerJrGKvNAp7zfemN98rp4BfzFonkJQmcumvP -n fio
        ./cleos wallet import --private-key 5KBX1dwHME4VyuUss2sYM25D5ZTDvyYrbEz37UJqwAVAsR4tGuY -n fio
        ./cleos wallet import --private-key 5JLxoeRoMDGBbkLdXJjxuh3zHsSS7Lg6Ak9Ft8v8sSdYPkFuABF -n fio
        ./cleos wallet import --private-key 5HvaoRV9QrbbxhLh6zZHqTzesFEG5vusVJGbUazFi5xQvKMMt6U -n fio
        ./cleos wallet import --private-key 5JCpqkvsrCzrAC3YWhx7pnLodr3Wr9dNMULYU8yoUrPRzu269Xz -n fio
        ./cleos wallet import --private-key 5K3s1yePVjrsCJbeyoXGAfJn9yCKKZi6E9oyw59mt5LaJG3Bm1N -n fio
        ./cleos wallet import --private-key 5JAs6QTmEyVjJGbQf1e5MjWLoNqcUAMyZLbLwvDhaG3gYBawDwK -n fio
        ./cleos wallet import --private-key 5JvWX1RkiaRS7jgMTb9oZsFKy77jJHkzEv78JRFkjxdDQR3d6wN -n fio
        ./cleos wallet import --private-key 5JKQTYUjxWQbWt53z6Prrb3PUNtEiG5GcyXv3yHTK1nJaxzABrk -n fio
        ./cleos wallet import --private-key 5K7Dah3CB17j6hpQ5vnBebN2yzGkt5K6A2xV7KshKyAfAUJyY8J -n fio
        ./cleos wallet import --private-key 5HvvvAF32bADDzyqcc3kRN4GVKGAZdVFXQexDScJEqTnpk29Kdv -n fio
        ./cleos wallet import --private-key 5JUgV5evcQGJkniK7rH1HckBAvRKsJP1Zgbv39UWnyTBEkMyWMj -n fio
        ./cleos wallet import --private-key 5K2HBexbraViJLQUJVJqZc42A8dxkouCmzMamdrZsLHhUHv77jF -n fio
        ./cleos wallet import --private-key 5JA5zQkg1S59swPzY6d29gsfNhNPVCb7XhiGJAakGFa7tEKSMjT -n fio
        ./cleos wallet import --private-key 5KQPk78a4Srm11NnuxGnWGNJuZtGaMErQ31XFDvXwzyS3Ru7AJQ -n fio
        ./cleos wallet import --private-key 5JqMPkY5AZMYe4LSu1byGPbyjwin9uE9uB3a14WC8ULtYvjaJzE -n fio
        ./cleos wallet import --private-key 5KcNHxGqm9csmFq8VPa5dGHZ99H8qDGj5F5yWgg6u3D4PUkUoEp -n fio
        ./cleos wallet import --private-key 5KDDv35PHWGkReThhNsj2j23DCJzvWTLvZbSTUj8BFNicrJXPqG -n fio
        ./cleos wallet import --private-key 5JHsotwiGMAjrTu3oCiXYjzJ3dEz9mU7HFc4UigvWgEksbpaVkc -n fio
        ./cleos wallet import --private-key 5K8dPJgVYWDxfEURh1cvrRzqc7uvDLqNHjxs83Rf2n8Hu7rqhmp -n fio
        ./cleos wallet import --private-key 5JDGpznJSJw8cKQYtEuctobAu2zVmD3Rw3fWwPzDuF4XbriseLy -n fio
        ./cleos wallet import --private-key 5JiSAASb4PZTajemTr4KTgeNBWUxrz6SraHGUvNihSskDA9aY5b -n fio
        ./cleos wallet import --private-key 5JwttkBMpCijBWiLx75hHTkYGgDm5gmny7nvnss4s1FoZWPxNui -n fio
    fi

    #Start Both Nodes
    if pgrep -x "nodeos" > /dev/null
    then
        kill nodeos
    fi

    ./nodeos --http-server-address localhost:8879 --http-validate-host=0 --enable-stale-production --producer-name eosio --plugin eosio::chain_api_plugin --plugin eosio::net_api_plugin 2> $oldpath/../node1.txt &
    sleep 3s
    ./nodeos --max-transaction-time=3000 --producer-name inita --plugin eosio::chain_api_plugin --plugin eosio::net_api_plugin --http-server-address 0.0.0.0:8889 --http-validate-host=0 --p2p-listen-endpoint :9877 --p2p-peer-address 0.0.0.0:9876 --config-dir $HOME/node2 --data-dir $HOME/node2 --private-key [\"EOS79vbwYtjhBVnBRYDjhCyxRFVr6JsFfVrLVhUKoqFTnceZtPvAU\",\"5JxUfAkXoCQdeZKNMhXEqRkFcZMYa3KR3vbie7SKsPv6rS3pCHg\"] --contracts-console 2> $oldpath/../node2.txt &
    sleep 6s

    if [ $restartneeded == 0 ]; then
        #Create Accounts
        echo $'Creating Accounts...\n'
        ./cleos -u http://localhost:8889 create account eosio fio.token EOS7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS EOS7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS
        ./cleos -u http://localhost:8889 create account eosio fio.system EOS7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS EOS7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS
        ./cleos -u http://localhost:8889 create account eosio fio.fee EOS7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS EOS7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS
        ./cleos -u http://localhost:8889 create account eosio fio.finance EOS7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS EOS7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS
        ./cleos -u http://localhost:8889 create account eosio fio.reqobt EOS7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS EOS7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS
    fi

    #Bind FIO.NAME Contract to Chain
    ./cleos -u http://localhost:8889 set contract -j fio.system $fio_contract_name_path fio.name.wasm fio.name.abi --permission fio.system@active
    #Bind fio.finance Contract to Chain
    ./cleos -u http://localhost:8889 set contract -j fio.finance $fio_finance_contract_name_path fio.finance.wasm fio.finance.abi --permission fio.finance@active
    #Bind fio.request.obt Contract to Chain
    ./cleos -u http://localhost:8889 set contract -j fio.reqobt $fio_request_obt_path fio.request.obt.wasm fio.request.obt.abi --permission fio.reqobt@active
    #Bind fio.fee Contract to Chain
    ./cleos -u http://localhost:8889 set contract -j fio.fee $fio_fee_name_path fio.fee.wasm fio.fee.abi --permission fio.fee@active
    #Bind EOSIO.Token Contract to Chain
    ./cleos -u http://localhost:8889 set contract eosio $eosio_bios_contract_name_path eosio.bios.wasm eosio.bios.abi
    ./cleos -u http://localhost:8889 set contract fio.token $eosio_token_contract_name_path eosio.token.wasm eosio.token.abi

    #Create the hashed accounts
    if [ $restartneeded == 0 ]; then

      ./cleos -u http://localhost:8889 create account eosio r41zuwovtn44 EOS5oBUYbtGTxMS66pPkjC2p8pbA3zCtc8XD4dq9fMut867GRdh82 EOS5oBUYbtGTxMS66pPkjC2p8pbA3zCtc8XD4dq9fMut867GRdh82
      ./cleos -u http://localhost:8889 create account eosio htjonrkf1lgs EOS7uRvrLVrZCbCM2DtCgUMospqUMnP3JUC1sKHA8zNoF835kJBvN EOS7uRvrLVrZCbCM2DtCgUMospqUMnP3JUC1sKHA8zNoF835kJBvN
      ./cleos -u http://localhost:8889 create account eosio euwdcp13zlrj EOS8NToQB65dZHv28RXSBBiyMCp55M7FRFw6wf4G3GeRt1VsiknrB EOS8NToQB65dZHv28RXSBBiyMCp55M7FRFw6wf4G3GeRt1VsiknrB
      ./cleos -u http://localhost:8889 create account eosio mnvcf4v1flnn EOS5GpUwQtFrfvwqxAv24VvMJFeMHutpQJseTz8JYUBfZXP2zR8VY EOS5GpUwQtFrfvwqxAv24VvMJFeMHutpQJseTz8JYUBfZXP2zR8VY
    fi

    ./cleos -u http://localhost:8889 push action -j fio.token create '["eosio","1000000000.000000000 FIO"]' -p fio.token@active
    ./cleos -u http://localhost:8889 push action -j fio.token issue '["r41zuwovtn44","1000000.000000000 FIO","memo"]' -p eosio@active
    ./cleos -u http://localhost:8889 push action -j fio.token issue '["htjonrkf1lgs","1000.000000000 FIO","memo"]' -p eosio@active
    ./cleos -u http://localhost:8889 push action -j fio.token issue '["euwdcp13zlrj","1000.000000000 FIO","memo"]' -p eosio@active
    ./cleos -u http://localhost:8889 push action -j fio.token issue '["mnvcf4v1flnn","1000.000000000 FIO","memo"]' -p eosio@active

    if [ $restartneeded == 0 ]; then
      # Setup permissions for inline actions in the
      echo Setting up inline permissions
      echo Adding eosio code to fio.token and fio.system.
      # Reference: https://github.com/EOSIO/eos/issues/4348#issuecomment-400562839
      # Reference: https://developers.eos.io/eosio-home/docs/inline-actions
      ./cleos -u http://localhost:8889 set account permission fio.token active '{"threshold": 1,"keys": [{"key": "EOS7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS","weight": 1}],"accounts": [{"permission":{"actor":"fio.token","permission":"eosio.code"},"weight":1}]}}' owner -p fio.token@owner
      #make the fio.token into a privileged account
      ./cleos -u http://localhost:8889 push action eosio setpriv '["fio.token",1]' -p eosio@active
      ./cleos -u http://localhost:8889 set account permission fio.system active '{"threshold": 1,"keys": [{"key": "EOS7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS","weight": 1}],"accounts": [{"permission":{"actor":"fio.system","permission":"eosio.code"},"weight":1}]}}' owner -p fio.system@owner
      #make the fio.system into a privileged account
      ./cleos -u http://localhost:8889 push action eosio setpriv '["fio.system",1]' -p eosio@active
      ./cleos -u http://localhost:8889 set account permission fio.reqobt active '{"threshold": 1,"keys": [{"key": "EOS7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS","weight": 1}],"accounts": [{"permission":{"actor":"fio.reqobt","permission":"eosio.code"},"weight":1}]}}' owner -p fio.reqobt@owner
      #make the fio.reqobt into a privileged account
      ./cleos -u http://localhost:8889 push action eosio setpriv '["fio.reqobt",1]' -p eosio@active
    fi

    #create fees for the fio protocol
    echo "creating fees"
    ./cleos -u http://localhost:8889 push action -j fio.fee create '{"end_point":"register_fio_domain","type":"0","suf_amount":"30000000000"}' --permission fio.fee@active
    ./cleos -u http://localhost:8889 push action -j fio.fee create '{"end_point":"register_fio_address","type":"0","suf_amount":"2000000000"}' --permission fio.fee@active
    ./cleos -u http://localhost:8889 push action -j fio.fee create '{"end_point":"add_pub_address","type":"1","suf_amount":"100000000"}' --permission fio.fee@active
    ./cleos -u http://localhost:8889 push action -j fio.fee create '{"end_point":"transfer_tokens_to_pub_key","type":"0","suf_amount":"250000000"}' --permission fio.fee@active
    ./cleos -u http://localhost:8889 push action -j fio.fee create '{"end_point":"transfer_tokens_to_fio_address","type":"0","suf_amount":"100000000"}' --permission fio.fee@active
    ./cleos -u http://localhost:8889 push action -j fio.fee create '{"end_point":"new_funds_request","type":"1","suf_amount":"100000000"}' --permission fio.fee@active
    ./cleos -u http://localhost:8889 push action -j fio.fee create '{"end_point":"reject_funds_request","type":"1","suf_amount":"100000000"}' --permission fio.fee@active
    ./cleos -u http://localhost:8889 push action -j fio.fee create '{"end_point":"record_send","type":"1","suf_amount":"100000000"}' --permission fio.fee@active


    echo setting accounts
    sleep 1
    dom=1

    function chkdomain ()
    {
        ./cleos  -u http://localhost:8889 get table fio.system fio.system domains | grep -q $1
        dom=$?
    }

    retries=3
    chkdomain "dapix"
    ./cleos -u http://localhost:8889 push action -j fio.system registername '{"fioname":"dapix","actor":"fio.system"}' --permission fio.system@active

    sleep 1
    chkdomain "dapix"
    if [ $dom -eq 0 ]; then
        echo dapix exists
    else
        echo failed to register dapix
    fi

    #Create Account Name
    ./cleos -u http://localhost:8889 push action -j fio.system registername '{"fioname":"casey.dapix","actor":"r41zuwovtn44"}' --permission r41zuwovtn44@active
    ./cleos -u http://localhost:8889 push action -j fio.system registername '{"fioname":"adam.dapix","actor":"htjonrkf1lgs"}' --permission htjonrkf1lgs@active

elif [ $mChoice == 3 ]; then
    read -p $'WARNING: ALL FILES ( WALLET & CHAIN ) WILL BE DELETED\n\nContinue? (1. No 2. Yes): ' bChoice

    if [ $bChoice == 1 ]; then
        cd $oldpath
         ./chain_nuke.sh

        cd ~/opt/eosio/bin
        ./nodeos --hard-replay

        echo $'\nNUKE COMPLETE - WELCOME TO YOUR NEW BUILD'
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
