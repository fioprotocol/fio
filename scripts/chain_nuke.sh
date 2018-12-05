#!/usr/bin/env bash

sh nodeos_kill.sh

set nodefile="~/Users/../Library/Application Support/eosio/nodeos"

find . -name node2 -type d -exec rm -r {} +
find . -name nodeos -type d -exec rm -r {} +

rm -r '$nodefile'
rm -r \$nodefile

path="$PWD"

echo 'complete'