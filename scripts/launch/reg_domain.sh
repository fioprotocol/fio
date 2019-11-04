#!/usr/bin/env bash

./cleos -u http://localhost:8889 push action -j fio.address regdomain '{"fio_domain":"dapix","owner_fio_public_key":"","max_fee":"40000000000","actor":"r41zuwovtn44","tpid":""}' --permission r41zuwovtn44@active
sleep 5
echo setdomainpub
./cleos -u http://localhost:8889 push action -j fio.address setdomainpub '{"fio_domain":"dapix","is_public":1,"max_fee":"40000000000","actor":"r41zuwovtn44","tpid":""}' --permission r41zuwovtn44@active
sleep 5