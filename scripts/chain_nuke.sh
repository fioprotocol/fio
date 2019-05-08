##!/usr/bin/env bash

pkill nodeos

echo $'Deleting Logs...\n'
rm -rf node1.txt
rm -rf node2.txt
rm -rf ../walletkey.ini

cd ~/opt/eosio/bin
echo $'Deleting Node Data...\n'
if [ -d ~/opt/eosio/bin/node2 ]; then rm -rf ~/opt/eosio/bin/node2; fi # Mac OS
if [ -d ~/node2 ]; then rm -rf ~/node2; fi # Mac OS

echo $'Deleting Chain Data...\n'
if [ -d ~/Library/Application\ Support/eosio/nodeos ]; then rm -rf ~/Library/Application\ Support/eosio/nodeos; fi # Mac OS
if [ -d ~/opt/eosio/bin/eosio/nodeos ]; then rm -rf ~~/opt/eosio/bin/eosio/nodeos; fi # Mac OS
if [ -d ~/.local/share/eosio/nodeos ]; then rm -rf ~/.local/share/eosio/nodeos; fi # Linux

echo $'Deleting Wallet Data...\n'
cd
if [ -d eosio-wallet ]; then rm -rf eosio-wallet; fi # Mac OS

echo 'Chain Data Delete Complete'