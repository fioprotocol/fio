pkill nodeos
pkill keosd
rm -rf ~/eosio-wallet
rm ../walletkey.ini
rm -rf ../node2
rm -rf ~/Library/Application\ Support/eosio

read -n 1 -r -p "press a key to gen first signature" key
../utils/sign-pack-trx.sh 2>/dev/null | grep "PCK RSP" | sed 's/PCK RSP: //' | pbcopy
read -n 1 -r -p "press a key to gen second signature" key
../utils/sign-pack-trx.sh notdapix 2>/dev/null | grep "PCK RSP" | sed 's/PCK RSP: //' | pbcopy
read -n 1 -r -p "press a key to termnate test" key
pkill nodeos
pkill keosd
