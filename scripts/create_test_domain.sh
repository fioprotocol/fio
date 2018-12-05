#!/usr/bin/env bash

read -p 'Domain Name: ' fio_domain_name

cleos -u http://localhost:8889 push action -j fio.system registername '{"name":$fio_domain_name,"requestor":"fioname11111"}' --permission fioname11111@active
