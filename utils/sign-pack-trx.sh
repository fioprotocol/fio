#!/bin/bash

# Script to sign and pack a transaction.


nPort=8889
wPort=9899
hostname="localhost"

echo ------------------------------------------
dataJson='{
  "name": "adam5.brd",
  "requestor": "fio.system"
}'

expectedPackedData=056461706978104208414933a95b
cmd="programs/cleos/cleos --no-auto-keosd --url http://$hostname:$nPort --wallet-url http://$hostname:$wPort  convert pack_action_data fio.system registername '$dataJson'"
echo CMD: $cmd
actualPackedData=`eval $cmd`
ret=$?
echo PACKED DATA: $actualPackedData
if [[ $ret != 0 ]]; then  exit $ret; fi
echo ------------------------------------------

# expirationStr=`date --date='+1 hour' "+%FT"%T`
# echo EXPIRATION: $expirationStr

headBlockTime=`programs/cleos/cleos --no-auto-keosd --url http://$hostname:$nPort --wallet-url http://$hostname:$wPort get info | grep head_block_time | awk '{print substr($2,2,length($2)-3)}'`
echo HEAD BLOCK TIME: $headBlockTime

expirationStr=`date -d "+1 hour $headBlockTime" "+%FT"%T`
echo EXPIRATION: $expirationStr

lastIrreversibleBlockNum=`programs/cleos/cleos --no-auto-keosd --url http://$hostname:$nPort --wallet-url http://$hostname:$wPort get info | grep last_irreversible_block_num | grep -o '[[:digit:]]*'`
echo LAST IRREVERSIBLE BLOCK NUM: $lastIrreversibleBlockNum

refBlockPrefix=`programs/cleos/cleos --no-auto-keosd --url http://$hostname:$nPort --wallet-url http://$hostname:$wPort get block $lastIrreversibleBlockNum | grep -m 1 -e ref_block_prefix | grep -o '[[:digit:]]*'`
echo REF BLOCK PREFIX: $refBlockPrefix
echo ------------------------------------------


# Unsigned request
unsignedRequest='{
    "expiration": "'${expirationStr}'",
    "ref_block_num": '${lastIrreversibleBlockNum}',
    "ref_block_prefix": '${refBlockPrefix}',
    "max_net_usage_words": 0,
    "max_cpu_usage_ms": 0,
    "delay_sec": 0,
    "context_free_actions": [],
    "fio_actions": [{
        "name": "registername",
        "fio_pub_key": "EOS5GpUwQtFrfvwqxAv24VvMJFeMHutpQJseTz8JYUBfZXP2zR8VY",
        "data": "'${actualPackedData}'"
      }
    ],
    "transaction_extensions": [],
    "signatures": [],
    "context_free_data": []
  },[
    "EOS5GpUwQtFrfvwqxAv24VvMJFeMHutpQJseTz8JYUBfZXP2zR8VY",
    "EOS7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS"
  ],
  "cf057bbfb72640471fd910bcb67639c22df9f92470936cddc1ade0e2f2e7dc4f"
'
# echo $unsignedRequest

# Sign request
expectedSignedRequest='{"expiration":"2018-12-13T20:59:50","ref_block_num":141,"ref_block_prefix":3586931320,"max_net_usage_words":0,"max_cpu_usage_ms":0,"delay_sec":0,"context_free_actions":[],"actions":[{"account":"fio.system","name":"registername","authorization":[{"actor":"fioname11111","permission":"active"},{"actor":"fio.system","permission":"active"}],"data":"036f6369104208414933a95b"}],"transaction_extensions":[],"signatures":["SIG_K1_Kcax7imeZM2nK3di7eZRZ5Y82eyxRHGE4gx7CT1Rky1JTVVmKCwytFLMTjg888B4RiwjhoCwk5pXndywg1pRxj8RCGqKyy","SIG_K1_K5FLUb7y2nq5EJjTRGDr5G2iFpEasX2qmrHbdexJDbYiYmiXo9b1YLTXz73b9VE6ipxs5gRtMooRyFUx9ucKQ8jBjYsR3u"],"context_free_data":[]}'
#cmd="curl --request POST --url http://$hostname:$wPort/v1/wallet/sign_transaction --data '$unsignedRequest'"
cmd="./programs/cleos/cleos --no-auto-keosd --url http://localhost:8889  --wallet-url http://localhost:9899 sign '$unsignedRequest' -k 5K2HBexbraViJLQUJVJqZc42A8dxkouCmzMamdrZsLHhUHv77jF"
echo CMD: $cmd
actualSignedResponse=`eval $cmd`
ret=$?
echo SIGNED RESPONSE: $actualSignedResponse
if [[ $ret != 0 ]]; then  exit $ret; fi
echo ------------------------------------------

# Pack request
expectedPackedResponse='{ "signatures": [ "SIG_K1_K5C3qWUzKJ3ciWQSe98vJF5jK5enmaFguaac5FYZn5sMSgk5shu86xSsELAqePvEquTjm1JsoaeKWFjEP4hT2sJyZRA8G3", "SIG_K1_K3d5zYXdsatBW5GZCrSV5c8TrwghGiv5xJR7RJ2XMiZBagDr5njgYrEMCVnLm4aNV9oFSWcCMUQfeaSWL2yZveJdpBsme4" ], "compression": "none", "packed_context_free_data": "", "packed_trx": "46c8125c8d00783accd500000000010000000000000000a0a4995765ec98ba000c036f6369104208414933a95b00" }'
cmd="programs/cleos/cleos --no-auto-keosd --verbose --url http://$hostname:$nPort --wallet-url http://$hostname:$wPort convert pack_transaction '$actualSignedResponse'"
echo CMD: $cmd
actualPackedResponse=`eval $cmd`
ret=$?
echo PCK RSP: $actualPackedResponse
if [[ $ret != 0 ]]; then  exit $ret; fi
echo ------------------------------------------

exit 0

unsignedRequest='[{
    "expiration": "'${expirationStr}'",
    "ref_block_num": '${lastIrreversibleBlockNum}',
    "ref_block_prefix": '${refBlockPrefix}',
    "max_net_usage_words": 0,
    "max_cpu_usage_ms": 0,
    "delay_sec": 0,
    "context_free_actions": [],
    "fio_actions": [{
        "name": "registername",
        "fio_pub_key": "0xab5801a7d398351b8be11c439e05c5b3259aec9b",
        "data": "'${actualPackedData}'"
      }
    ],
    "transaction_extensions": [],
    "signatures": [],
    "context_free_data": []
  },[
    "EOS5GpUwQtFrfvwqxAv24VvMJFeMHutpQJseTz8JYUBfZXP2zR8VY",
    "EOS7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS"
  ],
  "cf057bbfb72640471fd910bcb67639c22df9f92470936cddc1ade0e2f2e7dc4f"
]'

originalEOSunsignedRequest='[{
    "expiration": "2018-12-13T20:59:50",
    "ref_block_num": 141,
    "ref_block_prefix": 3586931320,
    "max_net_usage_words": 0,
    "max_cpu_usage_ms": 0,
    "delay_sec": 0,
    "context_free_actions": [],
    "actions": [{
        "account": "fio.system",
        "name": "registername",
        "authorization": [{
            "actor": "fioname11111",
            "permission": "active"
          },{
            "actor": "fio.system",
            "permission": "active"
          }
        ],
        "data": "036f6369104208414933a95b"
      }
    ],
    "transaction_extensions": [],
    "signatures": [],
    "context_free_data": []
  },[
    "EOS5GpUwQtFrfvwqxAv24VvMJFeMHutpQJseTz8JYUBfZXP2zR8VY",
    "EOS7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS"
  ],
  "cf057bbfb72640471fd910bcb67639c22df9f92470936cddc1ade0e2f2e7dc4f"
]'
