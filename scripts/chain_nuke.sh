#!/usr/bin/env bash

sh ./nodeos_kill.sh

echo $'Deleting Node Data...\n'
if [ -d ../node2 ]; then rm -rf ../node2; fi # Mac OS
if [ -d ~/node2 ]; then rm -rf ~/node2; fi # Mac OS

echo $'Deleting Chain Data...\n'
if [ -d ~/Library/Application\ Support/eosio/nodeos ]; then rm -rf ~/Library/Application\ Support/eosio/nodeos; fi # Mac OS
if [ -d ~/.local/share/eosio/nodeos ]; then rm -rf ~/.local/share/eosio/nodeos; fi # Linux

echo $'Deleting Wallet Data...\n'
if [ -d ~/eosio-wallet ]; then rm -rf ~/eosio-wallet; fi # Mac OS
rm -rf ../walletkey.ini

echo 'Chain Data Delete Complete'