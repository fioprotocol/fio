#!/usr/bin/env bash

sh ./nodeos_kill.sh

echo $'Deleting Node Data...\n'
find . -name node2 -type d -exec rm -r {} +

echo $'Deleting Chain Data...\n'
if [ -d ~/Library/Application\ Support/eosio/nodeos ]; then rm -rf ~/Library/Application\ Support/eosio/nodeos; fi # Mac OS
if [ -d ~/.local/share/eosio/nodeos ]; then rm -rf ~/.local/share/eosio/nodeos; fi # Linux

echo 'Chain Data Delete Complete'