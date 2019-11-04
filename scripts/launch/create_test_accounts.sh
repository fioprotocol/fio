#!/usr/bin/env bash

./cleos -u http://localhost:8879 create account eosio r41zuwovtn44 FIO5oBUYbtGTxMS66pPkjC2p8pbA3zCtc8XD4dq9fMut867GRdh82 FIO5oBUYbtGTxMS66pPkjC2p8pbA3zCtc8XD4dq9fMut867GRdh82
./cleos -u http://localhost:8879 create account eosio htjonrkf1lgs FIO7uRvrLVrZCbCM2DtCgUMospqUMnP3JUC1sKHA8zNoF835kJBvN FIO7uRvrLVrZCbCM2DtCgUMospqUMnP3JUC1sKHA8zNoF835kJBvN
./cleos -u http://localhost:8879 create account eosio euwdcp13zlrj FIO8NToQB65dZHv28RXSBBiyMCp55M7FRFw6wf4G3GeRt1VsiknrB FIO8NToQB65dZHv28RXSBBiyMCp55M7FRFw6wf4G3GeRt1VsiknrB
./cleos -u http://localhost:8879 create account eosio mnvcf4v1flnn FIO5GpUwQtFrfvwqxAv24VvMJFeMHutpQJseTz8JYUBfZXP2zR8VY FIO5GpUwQtFrfvwqxAv24VvMJFeMHutpQJseTz8JYUBfZXP2zR8VY
./cleos -u http://localhost:8879 create account eosio 5spujqoyq4ie FIO77odcm3LYr6YduUxf83a4jp4pQ4YvKAjBkHJnLxq2SsgNSc13u FIO77odcm3LYr6YduUxf83a4jp4pQ4YvKAjBkHJnLxq2SsgNSc13u

echo creating bp accounts
#accounts for BP
#bp1:dapix
#Private key: 5KQ6f9ZgUtagD3LZ4wcMKhhvK9qy4BuwL3L1pkm6E2v62HCne2R
#Public key: FIO7jVQXMNLzSncm7kxwg9gk7XUBYQeJPk8b6QfaK5NVNkh3QZrRr
#FIO Public Address (actor name): qbxn5zhw2ypw
./cleos -u http://localhost:8879 create account eosio qbxn5zhw2ypw FIO7jVQXMNLzSncm7kxwg9gk7XUBYQeJPk8b6QfaK5NVNkh3QZrRr FIO7jVQXMNLzSncm7kxwg9gk7XUBYQeJPk8b6QfaK5NVNkh3QZrRr

#bp2:dapix
#Private key: 5JnhMxfnLhZeRCRvCUsaHbrvPSxaqjkQAgw4ZFodx4xXyhZbC9P
#Public key: FIO7uTisye5w2hgrCSE1pJhBKHfqDzhvqDJJ4U3vN9mbYWzataS2b
#FIO Public Address (actor name): hfdg2qumuvlc
./cleos -u http://localhost:8879 create account eosio hfdg2qumuvlc FIO7uTisye5w2hgrCSE1pJhBKHfqDzhvqDJJ4U3vN9mbYWzataS2b FIO7uTisye5w2hgrCSE1pJhBKHfqDzhvqDJJ4U3vN9mbYWzataS2b

#bp3:dapix
#Private key: 5JvmPVxPxypQEKPwFZQW4Vx7EC8cDYzorVhSWZvuYVFMccfi5mU
#Public key: FIO6oa5UV9ghWgYH9en8Cv8dFcAxnZg2i9z9gKbnHahciuKNRPyHc
#FIO Public Address (actor name): wttywsmdmfew
./cleos -u http://localhost:8879 create account eosio wttywsmdmfew FIO6oa5UV9ghWgYH9en8Cv8dFcAxnZg2i9z9gKbnHahciuKNRPyHc FIO6oa5UV9ghWgYH9en8Cv8dFcAxnZg2i9z9gKbnHahciuKNRPyHc

#bp4:dapix
#Private key: 5KLxezoCEw5Ca97FHq3HPyrzkmZQT6Wqw9DmKaJ6inE6fiN1ijT
#Public key: FIO6ruJ5qLeaa6VtYVpkcU4AeWVaL2QvViyQqjxjpAWYRFsYaSbBN
#FIO Public Address (actor name): 3ddowwxs11ss
 ./cleos -u http://localhost:8879 create account eosio 3ddowwxs11ss FIO6ruJ5qLeaa6VtYVpkcU4AeWVaL2QvViyQqjxjpAWYRFsYaSbBN FIO6ruJ5qLeaa6VtYVpkcU4AeWVaL2QvViyQqjxjpAWYRFsYaSbBN

echo creating voter accounts
#accounts for voters
#vote1:dapix
#Private key: 5JBbUG5SDpLWxvBKihMeXLENinUzdNKNeozLas23Mj6ZNhz3hLS
#Public key: FIO6G9pXXM92Gy5eMwNquGULoCj3ZStwPLPdEb9mVXyEHqWN7HSuA
#FIO Public Address (actor name): o2ouxipw2rt4
 ./cleos -u http://localhost:8879 create account eosio o2ouxipw2rt4 FIO6G9pXXM92Gy5eMwNquGULoCj3ZStwPLPdEb9mVXyEHqWN7HSuA FIO6G9pXXM92Gy5eMwNquGULoCj3ZStwPLPdEb9mVXyEHqWN7HSuA

#vote2:dapix
#Private key: 5KC6Edd4BcKTLnRuGj2c8TRT9oLuuXLd3ZuCGxM9iNngc3D8S93
#Public key: FIO7eF3CWTpDCSWm4CvvsAhxEkAnMWWQ9CtQVfBxxWu2gkF8c4TN1
#FIO Public Address (actor name): extjnqh3j3gt
./cleos -u http://localhost:8879 create account eosio extjnqh3j3gt FIO7eF3CWTpDCSWm4CvvsAhxEkAnMWWQ9CtQVfBxxWu2gkF8c4TN1 FIO7eF3CWTpDCSWm4CvvsAhxEkAnMWWQ9CtQVfBxxWu2gkF8c4TN1

#vote3:dapix
#Private key: 5JNbpb3Au9ZckYJF7KceCQ31axGgzUEvUkm6668X9jg8avEyHnc
#Public key: FIO61VCrQgb4yaJukyB4gsxHfidYNVS1csYqBY8QqTBuXjWKjTWR8
#FIO Public Address (actor name): npe3obkgoteh
./cleos -u http://localhost:8879 create account eosio npe3obkgoteh FIO61VCrQgb4yaJukyB4gsxHfidYNVS1csYqBY8QqTBuXjWKjTWR8 FIO61VCrQgb4yaJukyB4gsxHfidYNVS1csYqBY8QqTBuXjWKjTWR8
sleep 5