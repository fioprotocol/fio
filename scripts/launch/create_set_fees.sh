#!/usr/bin/env bash

./cleos -u http://localhost:8879 push action -j fio.fee create '{"end_point":"register_fio_domain","type":"0","suf_amount":"40000000000"}' --permission fio.fee@active
./cleos -u http://localhost:8879 push action -j fio.fee create '{"end_point":"register_fio_address","type":"0","suf_amount":"5000000000"}' --permission fio.fee@active
./cleos -u http://localhost:8879 push action -j fio.fee create '{"end_point":"renew_fio_domain","type":"0","suf_amount":"40000000000"}' --permission fio.fee@active
./cleos -u http://localhost:8879 push action -j fio.fee create '{"end_point":"renew_fio_address","type":"0","suf_amount":"5000000000"}' --permission fio.fee@active
./cleos -u http://localhost:8879 push action -j fio.fee create '{"end_point":"add_pub_address","type":"1","suf_amount":"10000000"}' --permission fio.fee@active
./cleos -u http://localhost:8879 push action -j fio.fee create '{"end_point":"transfer_tokens_pub_key","type":"0","suf_amount":"250000000"}' --permission fio.fee@active
./cleos -u http://localhost:8879 push action -j fio.fee create '{"end_point":"transfer_tokens_fio_address","type":"0","suf_amount":"100000000"}' --permission fio.fee@active
./cleos -u http://localhost:8879 push action -j fio.fee create '{"end_point":"new_funds_request","type":"1","suf_amount":"10000000"}' --permission fio.fee@active
./cleos -u http://localhost:8879 push action -j fio.fee create '{"end_point":"reject_funds_request","type":"1","suf_amount":"10000000"}' --permission fio.fee@active
./cleos -u http://localhost:8879 push action -j fio.fee create '{"end_point":"record_send","type":"1","suf_amount":"10000000"}' --permission fio.fee@active
./cleos -u http://localhost:8889 push action -j fio.fee create '{"end_point":"set_fio_domain_public","type":"0","suf_amount":"100000000"}' --permission fio.fee@active
./cleos -u http://localhost:8889 push action -j fio.fee create '{"end_point":"register_producer","type":"0","suf_amount":"10000000000"}' --permission fio.fee@active
./cleos -u http://localhost:8889 push action -j fio.fee create '{"end_point":"register_proxy","type":"0","suf_amount":"10000000"}' --permission fio.fee@active
./cleos -u http://localhost:8889 push action -j fio.fee create '{"end_point":"unregister_proxy","type":"0","suf_amount":"10000000"}' --permission fio.fee@active
./cleos -u http://localhost:8889 push action -j fio.fee create '{"end_point":"unregister_producer","type":"0","suf_amount":"10000000"}' --permission fio.fee@active
./cleos -u http://localhost:8889 push action -j fio.fee create '{"end_point":"proxy_vote","type":"1","suf_amount":"10000000"}' --permission fio.fee@active
./cleos -u http://localhost:8889 push action -j fio.fee create '{"end_point":"vote_producer","type":"1","suf_amount":"10000000"}' --permission fio.fee@active
./cleos -u http://localhost:8889 push action -j fio.fee create '{"end_point":"add_to_whitelist","type":"1","suf_amount":"10000000"}' --permission fio.fee@active
./cleos -u http://localhost:8889 push action -j fio.fee create '{"end_point":"remove_from_whitelist","type":"1","suf_amount":"10000000"}' --permission fio.fee@active
./cleos -u http://localhost:8889 push action -j fio.fee create '{"end_point":"submit_bundled_transaction","type":"0","suf_amount":"0"}' --permission fio.fee@active
