#!/usr/bin/env bash

nodeos --http-server-address  localhost:8879  --http-validate-host=0 --enable-stale-production --producer-name eosio --plugin eosio::chain_api_plugin --plugin eosio::net_api_plugin
