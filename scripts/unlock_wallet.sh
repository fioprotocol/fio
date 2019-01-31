#!/usr/bin/env bash

walletkey=$(head -n 1 /walletkey.ini)

cleos wallet unlock --password $walletkey