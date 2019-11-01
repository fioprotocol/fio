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

       echo COPYING ABI FILES FROM contracts TO ./build/contracts!
            cp ./contracts/fio.fee/fio.fee.abi ./build/contracts/fio.fee/fio.fee.abi
            cp ./contracts/fio.address/fio.address.abi ./build/contracts/fio.address/fio.address.abi
            cp ./contracts/fio.request.obt/fio.request.obt.abi ./build/contracts/fio.request.obt/fio.request.obt.abi
            cp ./contracts/fio.whitelist/fio.whitelist.abi ./build/contracts/fio.whitelist/fio.whitelist.abi
    exit -1
fi

if [ $mChoice == 1 ]; then

    #EOSIO Directory Check
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

    sleep 2s
    cd ~/opt/eosio/bin

    if [ -n $oldpath/../walletkey.ini ]; then
        #./keosd --config-dir ~/eosio-wallet --wallet-dir ~/eosio-wallet --http-server-address localhost:8879 --http-alias localhost:8900 > ~/eosio-wallet/stdout.log 2> ~/eosio-wallet/stderr.log &
        ./cleos wallet create -n fio -f $oldpath/../walletkey.ini
    else
        walletkey=$(head -n 1 $oldpath/../walletkey.ini)
        echo 'Using Password:' $walletkey
        sleep 2s
        #./keosd --config-dir ~/eosio-wallet --wallet-dir ~/eosio-wallet --http-server-address localhost:8879 --http-alias localhost:8900 > ~/eosio-wallet/stdout.log 2> ~/eosio-wallet/stderr.log &
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
        ./cleos wallet import --private-key 5JsWbwAiPWb4gk1Mox4VZrLvfFhnNRWCq1PpuSKLfGcVD9MjHAX -n fio
        ./cleos wallet import --private-key 5JMoj56cRYoST3fSyD3bDCNLu7WbMpeGAohmHoaKW8j1vEGfrv2 -n fio
        #bp accounts
        ./cleos wallet import --private-key 5KQ6f9ZgUtagD3LZ4wcMKhhvK9qy4BuwL3L1pkm6E2v62HCne2R -n fio
        ./cleos wallet import --private-key 5JnhMxfnLhZeRCRvCUsaHbrvPSxaqjkQAgw4ZFodx4xXyhZbC9P -n fio
        ./cleos wallet import --private-key 5JvmPVxPxypQEKPwFZQW4Vx7EC8cDYzorVhSWZvuYVFMccfi5mU -n fio
        ./cleos wallet import --private-key 5KLxezoCEw5Ca97FHq3HPyrzkmZQT6Wqw9DmKaJ6inE6fiN1ijT -n fio
        #voter accounts
        ./cleos wallet import --private-key 5JBbUG5SDpLWxvBKihMeXLENinUzdNKNeozLas23Mj6ZNhz3hLS -n fio
        ./cleos wallet import --private-key 5KC6Edd4BcKTLnRuGj2c8TRT9oLuuXLd3ZuCGxM9iNngc3D8S93 -n fio
        ./cleos wallet import --private-key 5JNbpb3Au9ZckYJF7KceCQ31axGgzUEvUkm6668X9jg8avEyHnc -n fio
        ./cleos wallet import --private-key 5KUTcBUc4gFAbYk61fsZbbYek4c5c3y8bY6sd78iPhM8QGYbJQ1 -n fio

    fi

    #Start Both Nodes
    if pgrep -x "nodeos" > /dev/null
    then
        pkill nodeos
    fi

    #start the bios node, this starts up, and eventually goes into dormancy.
    ./nodeos --max-transaction-time=6000 --http-server-address localhost:8879 --p2p-listen-endpoint localhost:9876 --config-dir=$HOME/node1 --http-validate-host=0 --enable-stale-production --producer-name eosio --plugin eosio::chain_api_plugin --plugin eosio::net_api_plugin  --contracts-console 2> $oldpath/../node1.txt &
    sleep 3s
    #start the first BP node
    ./nodeos --max-transaction-time=6000 --producer-name 3ddowwxs11ss --producer-name wttywsmdmfew --plugin eosio::chain_api_plugin --plugin eosio::net_api_plugin --http-server-address 0.0.0.0:8889 --http-validate-host=0 --p2p-listen-endpoint :9877 --p2p-peer-address localhost:9876 --config-dir $HOME/node2 --data-dir $HOME/node2  --private-key [\"FIO6ruJ5qLeaa6VtYVpkcU4AeWVaL2QvViyQqjxjpAWYRFsYaSbBN\",\"5KLxezoCEw5Ca97FHq3HPyrzkmZQT6Wqw9DmKaJ6inE6fiN1ijT\"] --private-key [\"FIO6oa5UV9ghWgYH9en8Cv8dFcAxnZg2i9z9gKbnHahciuKNRPyHc\",\"5JvmPVxPxypQEKPwFZQW4Vx7EC8cDYzorVhSWZvuYVFMccfi5mU\"]  --contracts-console 2> $oldpath/../node2.txt &
    sleep 3s
    #start the second BP node
    ./nodeos --max-transaction-time=6000 --producer-name qbxn5zhw2ypw --producer-name hfdg2qumuvlc  --plugin eosio::chain_api_plugin --plugin eosio::net_api_plugin --http-server-address 0.0.0.0:8890 --http-validate-host=0 --p2p-listen-endpoint :9878 --p2p-peer-address localhost:9877 --config-dir $HOME/node3 --data-dir $HOME/node3 --private-key [\"FIO7jVQXMNLzSncm7kxwg9gk7XUBYQeJPk8b6QfaK5NVNkh3QZrRr\",\"5KQ6f9ZgUtagD3LZ4wcMKhhvK9qy4BuwL3L1pkm6E2v62HCne2R\"] --private-key [\"FIO7uTisye5w2hgrCSE1pJhBKHfqDzhvqDJJ4U3vN9mbYWzataS2b\",\"5JnhMxfnLhZeRCRvCUsaHbrvPSxaqjkQAgw4ZFodx4xXyhZbC9P\"] --contracts-console 2> $oldpath/../node3.txt &
    sleep 6s

    if [ $restartneeded == 0 ]; then
        #Create Accounts

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

        echo bound all contracts

        #create the token on the bios node
        ./cleos -u http://localhost:8879 push action -j fio.token create '["1000000000.000000000 FIO"]' -p fio.token@active
        #load the bios contract on the bios node
        ./cleos -u http://localhost:8879 set contract eosio $eosio_bios_contract_name_path eosio.bios.wasm eosio.bios.abi

        sleep 3s

        # Bind more fio contracts
        ./cleos -u http://localhost:8879 set contract -j fio.reqobt $fio_reqobt_name_path fio.request.obt.wasm fio.request.obt.abi --permission fio.reqobt@active
        sleep 3s
        ./cleos -u http://localhost:8879 set contract -j fio.fee $fio_fee_name_path fio.fee.wasm fio.fee.abi --permission fio.fee@active
        sleep 3s
        ./cleos -u http://localhost:8889 set contract -j fio.treasury $fio_treasury_name_path fio.treasury.wasm fio.treasury.abi --permission fio.treasury@active
        sleep 3s
        ./cleos -u http://localhost:8889 set contract -j fio.whitelst $fio_whitelist_name_path fio.whitelist.wasm fio.whitelist.abi --permission fio.whitelst@active
        sleep 3s
        ./cleos -u http://localhost:8889 set contract -j eosio.wrap $eosio_wrap_name_path eosio.wrap.wasm eosio.wrap.abi --permission eosio.wrap@active
        sleep 3s

        echo creating dapix test accounts
        ./cleos -u http://localhost:8879 create account eosio r41zuwovtn44 FIO5oBUYbtGTxMS66pPkjC2p8pbA3zCtc8XD4dq9fMut867GRdh82 FIO5oBUYbtGTxMS66pPkjC2p8pbA3zCtc8XD4dq9fMut867GRdh82
        ./cleos -u http://localhost:8879 create account eosio htjonrkf1lgs FIO7uRvrLVrZCbCM2DtCgUMospqUMnP3JUC1sKHA8zNoF835kJBvN FIO7uRvrLVrZCbCM2DtCgUMospqUMnP3JUC1sKHA8zNoF835kJBvN
        ./cleos -u http://localhost:8879 create account eosio euwdcp13zlrj FIO8NToQB65dZHv28RXSBBiyMCp55M7FRFw6wf4G3GeRt1VsiknrB FIO8NToQB65dZHv28RXSBBiyMCp55M7FRFw6wf4G3GeRt1VsiknrB
        ./cleos -u http://localhost:8879 create account eosio mnvcf4v1flnn FIO5GpUwQtFrfvwqxAv24VvMJFeMHutpQJseTz8JYUBfZXP2zR8VY FIO5GpUwQtFrfvwqxAv24VvMJFeMHutpQJseTz8JYUBfZXP2zR8VY
        ./cleos -u http://localhost:8879 create account eosio 5spujqoyq4ie FIO77odcm3LYr6YduUxf83a4jp4pQ4YvKAjBkHJnLxq2SsgNSc13u FIO77odcm3LYr6YduUxf83a4jp4pQ4YvKAjBkHJnLxq2SsgNSc13u

        echo creating bp accounts
        #accounts for BP
        #bp1:dapix
        #Private key: 5KQ6f9ZgUtagD3LZ4wcMKhhvK9qy4BuwL3L1pkm6E2v62HCne2R
        #Public key: FIO7jVQXMNLzSncm7kxwg9gk7XUBYQeJPk8b6QfaK5NVNkh3QZrRr
        #FIO Public Address (actor name): qbxn5zhw2ypw
        ./cleos -u http://localhost:8879 create account eosio qbxn5zhw2ypw FIO7jVQXMNLzSncm7kxwg9gk7XUBYQeJPk8b6QfaK5NVNkh3QZrRr FIO7jVQXMNLzSncm7kxwg9gk7XUBYQeJPk8b6QfaK5NVNkh3QZrRr

        #bp2:dapix
        #Private key: 5JnhMxfnLhZeRCRvCUsaHbrvPSxaqjkQAgw4ZFodx4xXyhZbC9P
        #Public key: FIO7uTisye5w2hgrCSE1pJhBKHfqDzhvqDJJ4U3vN9mbYWzataS2b
        #FIO Public Address (actor name): hfdg2qumuvlc
        ./cleos -u http://localhost:8879 create account eosio hfdg2qumuvlc FIO7uTisye5w2hgrCSE1pJhBKHfqDzhvqDJJ4U3vN9mbYWzataS2b FIO7uTisye5w2hgrCSE1pJhBKHfqDzhvqDJJ4U3vN9mbYWzataS2b

        #bp3:dapix
        #Private key: 5JvmPVxPxypQEKPwFZQW4Vx7EC8cDYzorVhSWZvuYVFMccfi5mU
        #Public key: FIO6oa5UV9ghWgYH9en8Cv8dFcAxnZg2i9z9gKbnHahciuKNRPyHc
        #FIO Public Address (actor name): wttywsmdmfew
        ./cleos -u http://localhost:8879 create account eosio wttywsmdmfew FIO6oa5UV9ghWgYH9en8Cv8dFcAxnZg2i9z9gKbnHahciuKNRPyHc FIO6oa5UV9ghWgYH9en8Cv8dFcAxnZg2i9z9gKbnHahciuKNRPyHc

        #bp4:dapix
        #Private key: 5KLxezoCEw5Ca97FHq3HPyrzkmZQT6Wqw9DmKaJ6inE6fiN1ijT
        #Public key: FIO6ruJ5qLeaa6VtYVpkcU4AeWVaL2QvViyQqjxjpAWYRFsYaSbBN
        #FIO Public Address (actor name): 3ddowwxs11ss
         ./cleos -u http://localhost:8879 create account eosio 3ddowwxs11ss FIO6ruJ5qLeaa6VtYVpkcU4AeWVaL2QvViyQqjxjpAWYRFsYaSbBN FIO6ruJ5qLeaa6VtYVpkcU4AeWVaL2QvViyQqjxjpAWYRFsYaSbBN

        echo creating voter accounts
        #accounts for voters
        #vote1:dapix
        #Private key: 5JBbUG5SDpLWxvBKihMeXLENinUzdNKNeozLas23Mj6ZNhz3hLS
        #Public key: FIO6G9pXXM92Gy5eMwNquGULoCj3ZStwPLPdEb9mVXyEHqWN7HSuA
        #FIO Public Address (actor name): o2ouxipw2rt4
         ./cleos -u http://localhost:8879 create account eosio o2ouxipw2rt4 FIO6G9pXXM92Gy5eMwNquGULoCj3ZStwPLPdEb9mVXyEHqWN7HSuA FIO6G9pXXM92Gy5eMwNquGULoCj3ZStwPLPdEb9mVXyEHqWN7HSuA

        #vote2:dapix
        #Private key: 5KC6Edd4BcKTLnRuGj2c8TRT9oLuuXLd3ZuCGxM9iNngc3D8S93
        #Public key: FIO7eF3CWTpDCSWm4CvvsAhxEkAnMWWQ9CtQVfBxxWu2gkF8c4TN1
        #FIO Public Address (actor name): extjnqh3j3gt
        ./cleos -u http://localhost:8879 create account eosio extjnqh3j3gt FIO7eF3CWTpDCSWm4CvvsAhxEkAnMWWQ9CtQVfBxxWu2gkF8c4TN1 FIO7eF3CWTpDCSWm4CvvsAhxEkAnMWWQ9CtQVfBxxWu2gkF8c4TN1

        #vote3:dapix
        #Private key: 5JNbpb3Au9ZckYJF7KceCQ31axGgzUEvUkm6668X9jg8avEyHnc
        #Public key: FIO61VCrQgb4yaJukyB4gsxHfidYNVS1csYqBY8QqTBuXjWKjTWR8
        #FIO Public Address (actor name): npe3obkgoteh
        ./cleos -u http://localhost:8879 create account eosio npe3obkgoteh FIO61VCrQgb4yaJukyB4gsxHfidYNVS1csYqBY8QqTBuXjWKjTWR8 FIO61VCrQgb4yaJukyB4gsxHfidYNVS1csYqBY8QqTBuXjWKjTWR8

        sleep 5

         #create fees for the fio protocol
             echo "creating fees"
             ./cleos -u http://localhost:8879 push action -j fio.fee create '{"end_point":"register_fio_domain","type":"0","suf_amount":"40000000000"}' --permission fio.fee@active
             ./cleos -u http://localhost:8879 push action -j fio.fee create '{"end_point":"register_fio_address","type":"0","suf_amount":"5000000000"}' --permission fio.fee@active
             ./cleos -u http://localhost:8879 push action -j fio.fee create '{"end_point":"renew_fio_domain","type":"0","suf_amount":"40000000000"}' --permission fio.fee@active
             ./cleos -u http://localhost:8879 push action -j fio.fee create '{"end_point":"renew_fio_address","type":"0","suf_amount":"5000000000"}' --permission fio.fee@active
             ./cleos -u http://localhost:8879 push action -j fio.fee create '{"end_point":"add_pub_address","type":"1","suf_amount":"10000000"}' --permission fio.fee@active
             ./cleos -u http://localhost:8879 push action -j fio.fee create '{"end_point":"transfer_tokens_pub_key","type":"0","suf_amount":"250000000"}' --permission fio.fee@active
             ./cleos -u http://localhost:8879 push action -j fio.fee create '{"end_point":"transfer_tokens_fio_address","type":"0","suf_amount":"100000000"}' --permission fio.fee@active
             ./cleos -u http://localhost:8879 push action -j fio.fee create '{"end_point":"new_funds_request","type":"1","suf_amount":"10000000"}' --permission fio.fee@active
             ./cleos -u http://localhost:8879 push action -j fio.fee create '{"end_point":"reject_funds_request","type":"1","suf_amount":"10000000"}' --permission fio.fee@active
             ./cleos -u http://localhost:8879 push action -j fio.fee create '{"end_point":"record_send","type":"1","suf_amount":"10000000"}' --permission fio.fee@active
             ./cleos -u http://localhost:8889 push action -j fio.fee create '{"end_point":"set_fio_domain_public","type":"0","suf_amount":"100000000"}' --permission fio.fee@active
             ./cleos -u http://localhost:8889 push action -j fio.fee create '{"end_point":"register_producer","type":"0","suf_amount":"10000000000"}' --permission fio.fee@active
             ./cleos -u http://localhost:8889 push action -j fio.fee create '{"end_point":"register_proxy","type":"0","suf_amount":"10000000"}' --permission fio.fee@active
             ./cleos -u http://localhost:8889 push action -j fio.fee create '{"end_point":"unregister_proxy","type":"0","suf_amount":"10000000"}' --permission fio.fee@active
             ./cleos -u http://localhost:8889 push action -j fio.fee create '{"end_point":"unregister_producer","type":"0","suf_amount":"10000000"}' --permission fio.fee@active
             ./cleos -u http://localhost:8889 push action -j fio.fee create '{"end_point":"proxy_vote","type":"1","suf_amount":"10000000"}' --permission fio.fee@active
             ./cleos -u http://localhost:8889 push action -j fio.fee create '{"end_point":"vote_producer","type":"1","suf_amount":"10000000"}' --permission fio.fee@active
             ./cleos -u http://localhost:8889 push action -j fio.fee create '{"end_point":"add_to_whitelist","type":"1","suf_amount":"10000000"}' --permission fio.fee@active
             ./cleos -u http://localhost:8889 push action -j fio.fee create '{"end_point":"remove_from_whitelist","type":"1","suf_amount":"10000000"}' --permission fio.fee@active
             ./cleos -u http://localhost:8889 push action -j fio.fee create '{"end_point":"submit_bundled_transaction","type":"0","suf_amount":"0"}' --permission fio.fee@active


        echo issuing tokens

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

       if [ $restartneeded == 0 ]; then
                # Setup permissions for inline actions in the
                echo Setting up inline permissions
                echo Adding eosio code to fio.token and fio.system.
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

           fi


        echo regdomain
          ./cleos -u http://localhost:8889 push action -j fio.address regdomain '{"fio_domain":"dapix","owner_fio_public_key":"","max_fee":"40000000000","actor":"r41zuwovtn44","tpid":""}' --permission r41zuwovtn44@active
          sleep 5
          echo setdomainpub
          ./cleos -u http://localhost:8889 push action -j fio.address setdomainpub '{"fio_domain":"dapix","is_public":1,"max_fee":"40000000000","actor":"r41zuwovtn44","tpid":""}' --permission r41zuwovtn44@active
          sleep 5

        echo registering all block producers
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



    fi



    #set up bounties_table
     echo creating bounties
     ./cleos -u http://localhost:8889 push action -j fio.tpid updatebounty '{"amount":0}' --permission fio.tpid@active


    echo setting accounts
    sleep 1
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
    ./cleos -u http://localhost:8889 push action -j eosio unregproxy '{"fio_address":"adam:dapix","actor":"htjonrkf1lgs",,"max_fee":"40000000000""}' --permission htjonrkf1lgs@active
    ./cleos -u http://localhost:8889 push action -j eosio regproxy '{"fio_address":"casey:dapix","actor":"r41zuwovtn44",,"max_fee":"40000000000"}' --permission r41zuwovtn44@active
    ./cleos -u http://localhost:8889 push action -j eosio unregproxy '{"fio_address":"casey:dapix","actor":"r41zuwovtn44",,"max_fee":"40000000000"}' --permission r41zuwovtn44@active
elif [ $mChoice == 3 ]; then
    read -p $'WARNING: ALL FILES ( WALLET & CHAIN ) WILL BE DELETED\n\nContinue? (1. Yes 2. No): ' bChoice

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
