        
# FIOIO  

## Overview

The Foundation for Interwallet Operability (FIO) is a consortium of leading blockchain wallets, exchanges and payments providers that seeks to accelerate blockchain adoption by reducing the risk, complexity, and inconvenience of sending and receiving cryptoassets.

FIO is developing the FIO Protocol, a decentralized open-source blockchain protocol, that will enable greatly enhanced user experience inside the FIO enabled wallets, exchanges and applications of their choice.

With the FIO Protocol enabled, the sending and receiving of crypto tokens and coins from any blockchain on any wallet or exchange will be come easy and error free. 

____
#### Mac OS X Build & Install  
    
    ./fioio_build.sh  
    sudo ./fioio_install.sh  
  
#### Development Environment Setup

Run: `./fioio_launch.sh`  

Default Environment Settings: 

|    Name    | Data     |
| :---------|:--------:|
| Node Port  | 8889 |  
| Domain     | brd |
| FIO Names  | adam , casey |

#### Chain Management
###### Shutdown Local Test Node
Run: `./scripts/nodeos_kill.sh`

###### Hard Restart:

Run: `./scripts/chain_nuke.sh`

###### Signed Transaction

Run: `../utils/sign-pack-trx.sh` ( from build folder )

**Remove Folders Manually:<br>**
Linux: `~/.local/nodeos`<br>
Mac OS: `~/Library/Application Support/eosio/nodeos/`<br><br> `~../fio/build/programs/nodeos`<br>
 `~../fio/node2` or `~../fio/scripts/node2` depending on the location the nodes have been launched at. 

  
---  
  
### Useful Commands

##### Unlock Wallet

    cleos wallet unlock  

##### Import Wallet Private Key

    cleos wallet import --private-key [privatekey]
    
##### Start Nodes
###### Block Producer: 
    nodeos --http-server-address  localhost:8879  --http-validate-host=0 --enable-stale-production --producer-name eosio    --plugin eosio::chain_api_plugin --plugin eosio::net_api_plugin 
###### Validating Node:
    nodeos --producer-name inita --plugin eosio::chain_api_plugin --plugin eosio::net_api_plugin --http-server-address  0.0.0.0:8889 --http-validate-host=0 --p2p-listen-endpoint :9877  --p2p-peer-address 0.0.0.0:9876 --config-dir node2 --data-dir node2 --private-key [\"EOS79vbwYtjhBVnBRYDjhCyxRFVr6JsFfVrLVhUKoqFTnceZtPvAU\",\"5JxUfAkXoCQdeZKNMhXEqRkFcZMYa3KR3vbie7SKsPv6rS3pCHg\"]
     
  
##### Set FIO Name Contract  
  
    cleos -u http://localhost:8889 set contract -j fio.system ~/fio/build/contracts/fio.name/ fio.name.wasm fio.name.abi --permission fio.system@active  
  
##### Set FIO Finance Contract  
  
    cleos -u http://localhost:8889 set contract -j fio.finance ~/fio/build/contracts/fio.finance/ fio.finance.wasm fio.finance.abi --permission fio.finance@active  
  
##### Create Domain  
  
    cleos -u http://localhost:8889 push action -j fio.system registername '{"name":"brd","requestor":"fio.system"}' --permission fio.system@active  
  
##### Create FIO Name  
  
     cleos -u http://localhost:8889 push action -j fio.system registername '{"name":"name.brd","requestor":"fioname11111"}' --permission fio.system@active   
  
##### Add FIO Address  
  
    cleos -u http://localhost:8889 push action -j fio.system addaddress '{"fio_user_name":"name.brd","chain":"FIO","address":"FIOADDRESS","requestor":"fioname11111"}' --permission fioname11111@active  
  
##### Get Registered FIO Names  
  
    cleos -u http://localhost:8889 get table fio.system fio.system fionames  
  
##### Get Registered FIO Domains  
  
    cleos -u http://localhost:8889 get table fio.system fio.system domains  
  
##### Test API Endpoints  
  
    curl --request POST  http://localhost:8889/v1/chain/avail_check --data '{"fio_name":"test.brd"}'
    
##### API Call for register_fio_name

    curl --request POST --url http://localhost:8889/v1/chain/register_fio_name  --data '<packed signed transaction>'
