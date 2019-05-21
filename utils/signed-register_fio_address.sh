#!/bin/bash

# Script to sign and pack a transaction.


nPort=8889
wPort=9899
hostname="localhost"

if [ -z "$1" ]; then
    domain="eddie.freddie"
else
    domain=$1
fi

echo ------------------------------------------


fiopubkey="EOS5oBUYbtGTxMS66pPkjC2p8pbA3zCtc8XD4dq9fMut867GRdh82"

fioactor=`programs/cleos/cleos convert fiokey_to_account $fiopubkey`
#NOTE -- set the owner_fio_public_key to "" to have the owner be the account that is signing the TX
echo ------------------------------------------
dataJson="{
  \"fio_address\": \"${domain}\",
  \"actor\": \"${fioactor}\",
  \"max_fee\":\"40000000000\",
  \"owner_fio_public_key\":\"EOS6vRt3FzoRJYx1dxWigXzgkbnoZXg2zAfiofh4E3eCtNvJZhvWY\"
}"

expectedPackedData=056461706978104208414933a95b
cmd="programs/cleos/cleos --no-auto-keosd --url http://$hostname:$nPort --wallet-url http://$hostname:$wPort  convert pack_action_data fio.system regaddress '${dataJson}'"
echo CMD: $cmd
actualPackedData=`eval $cmd`
ret=$?
echo PACKED DATA: $actualPackedData
if [[ $ret != 0 ]]; then  exit $ret; fi
echo ------------------------------------------

headBlockTime=`programs/cleos/cleos --no-auto-keosd --url http://$hostname:$nPort --wallet-url http://$hostname:$wPort get info | grep head_block_time | awk '{print substr($2,2,length($2)-3)}'`
echo HEAD BLOCK TIME: $headBlockTime

cmd="date -d \"+1 hour $headBlockTime\" \"+%FT\"%T"
if [[ "$OSTYPE" == "darwin"* ]]; then
    cmd="date -j -v+1H -f \"%Y-%m-%dT%H:%M:%S.\" \"$headBlockTime\" \"+%FT\"%T"
fi
echo CMD: $cmd
expirationStr=`eval $cmd`
echo EXPIRATION: $expirationStr

cmd="programs/cleos/cleos --no-auto-keosd --url http://$hostname:$nPort --wallet-url http://$hostname:$wPort get info | grep last_irreversible_block_num | grep -o '[0-9]\+'"
echo CMD: $cmd
lastIrreversibleBlockNum=`eval $cmd`
#lastIrreversibleBlockNum=`programs/cleos/cleos --no-auto-keosd --url http://$hostname:$nPort --wallet-url http://$hostname:$wPort get info | grep last_irreversible_block_num | grep -o '[[:digit:]]*'`
echo LAST IRREVERSIBLE BLOCK NUM: $lastIrreversibleBlockNum

cmd="programs/cleos/cleos --no-auto-keosd --url http://$hostname:$nPort --wallet-url http://$hostname:$wPort get block $lastIrreversibleBlockNum | grep -m 1 -e ref_block_prefix | grep -o '[0-9]\+'"
echo CMD: $cmd
refBlockPrefix=`eval $cmd`
#refBlockPrefix=`programs/cleos/cleos --no-auto-keosd --url http://$hostname:$nPort --wallet-url http://$hostname:$wPort get block $lastIrreversibleBlockNum | grep -m 1 -e ref_block_prefix | grep -o '[[:digit:]]*'`
echo REF BLOCK PREFIX: $refBlockPrefix
echo ------------------------------------------

# Unsigned request
unsignedRequest='{
    "chain_id": "cf057bbfb72640471fd910bcb67639c22df9f92470936cddc1ade0e2f2e7dc4f",
    "expiration": "'${expirationStr}'",
    "ref_block_num": '${lastIrreversibleBlockNum}',
    "ref_block_prefix": '${refBlockPrefix}',
    "max_net_usage_words": 0,
    "max_cpu_usage_ms": 0,
    "delay_sec": 0,
    "context_free_actions": [],
    "actions": [{
        "account":"fio.system",
        "name": "regaddress"
        "authorization":[{
             "actor":"'${fioactor}'",
             "permission":"active"
        }]
    "data": "'${actualPackedData}'"
      }
    ],
    "transaction_extensions": [],
    "signatures": [],
    "context_free_data": []
  }
'
# echo $unsignedRequest

# Sign request
expectedSignedRequest='{
    "signatures":["SIG_K1_Kcax7imeZM2nK3di7eZRZ5Y82eyxRHGE4gx7CT1Rky1JTVVmKCwytFLMTjg888B4RiwjhoCwk5pXndywg1pRxj8RCGqKyy",
                    "SIG_K1_K5FLUb7y2nq5EJjTRGDr5G2iFpEasX2qmrHbdexJDbYiYmiXo9b1YLTXz73b9VE6ipxs5gRtMooRyFUx9ucKQ8jBjYsR3u"],
"context_free_data":[]}'


cmd="./programs/cleos/cleos --no-auto-keosd --url http://localhost:8889  --wallet-url http://localhost:9899 sign '$unsignedRequest' -k 5JLxoeRoMDGBbkLdXJjxuh3zHsSS7Lg6Ak9Ft8v8sSdYPkFuABF"
echo CMD: $cmd
actualSignedResponse=`eval $cmd`
ret=$?
echo SIGNED RESPONSE: $actualSignedResponse
if [[ $ret != 0 ]]; then  exit $ret; fi
echo ------------------------------------------

# Pack request
expectedPackedResponse='{
    "signatures": [
      "SIG_K1_K5C3qWUzKJ3ciWQSe98vJF5jK5enmaFguaac5FYZn5sMSgk5shu86xSsELAqePvEquTjm1JsoaeKWFjEP4hT2sJyZRA8G3",
      "SIG_K1_K3d5zYXdsatBW5GZCrSV5c8TrwghGiv5xJR7RJ2XMiZBagDr5njgYrEMCVnLm4aNV9oFSWcCMUQfeaSWL2yZveJdpBsme4" ],
    "compression": "none",
    "packed_context_free_data": "",
    "packed_trx": "46c8125c8d00783accd500000000010000000000000000a0a4995765ec98ba000c036f6369104208414933a95b00" }'


cmd="programs/cleos/cleos --no-auto-keosd --verbose --url http://$hostname:$nPort --wallet-url http://$hostname:$wPort convert pack_transaction '$actualSignedResponse'"
echo CMD: $cmd
actualPackedResponse=`eval $cmd`
ret=$?
echo PCK RSP: $actualPackedResponse
if [[ $ret != 0 ]]; then  exit $ret; fi
echo ------------------------------------------

exit 0
