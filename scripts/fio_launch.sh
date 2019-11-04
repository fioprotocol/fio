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
export oldpath

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

    sleep 2s
    cd ~/opt/eosio/bin
    $oldpath/launch/new_wallet.sh

    if [ $restartneeded == 0 ]; then
        #Import Wallet Keys
        $oldpath/launch/import_keys.sh
    fi

    #Start Both Nodes
    $oldpath/launch/start_test_nodes.sh

    if [ $restartneeded == 0 ]; then
        #Create Accounts
        $oldpath/launch/create_accounts.sh

        echo bound all contracts
        $oldpath/launch/bind_contracts.sh

        echo creating dapix test accounts
        $oldpath/launch/create_test_accounts.sh

        #create fees for the fio protocol
        echo "creating fees"
        $oldpath/launch/create_set_fees.sh

        echo issuing tokens
        $oldpath/launch/token_issue.sh

        if [ $restartneeded == 0 ]; then
            echo Setting up inline permissions
            echo Adding eosio code to fio.token and fio.system.
            $oldpath/launch/set_permissions.sh
        fi

        echo regdomain
        $oldpath/launch/reg_domain.sh

        echo registering all block producers
        $oldpath/launch/reg_vote_prod.sh
    fi

    #set up bounties_table
     echo creating bounties
     ./cleos -u http://localhost:8889 push action -j fio.tpid updatebounty '{"amount":0}' --permission fio.tpid@active

    echo setting accounts
    sleep 1
    $oldpath/launch/set_accounts.sh

elif [ $mChoice == 3 ]; then
    read -p $'WARNING: ALL FILES ( WALLET & CHAIN ) WILL BE DELETED\n\nContinue? (1. Yes 2. No): ' bChoice

    if [ $bChoice == 1 ]; then
        $oldpath/launch/chain_nuke.sh

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
