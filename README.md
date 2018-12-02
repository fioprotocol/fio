
# FIOIO  

#### Mac OS X Build & Install  
  
    export LLVM_DIR=/usr/local/Cellar/llvm@4/4.0.1/lib/cmake/llvm; ./fioio_build.sh  
    sudo ./fioio_install.sh  
  
#### Wallet Management
##### Create Default Wallet  
  
    cleos wallet create --to-console  
    
##### Import Wallet Private Key

    cleos wallet import --private-key [privatekey]

##### Unlock Wallet

    cleos wallet unlock
    
#### Development Environment Setup
Run: `./start_test_env.sh`  
Both Nodes will start<br>

Open new Terminal
Run: `./start_test_env.sh`

Default fio.name Folder: `~../fio/build/contracts/fio.name`
#### Test Launcher  
  
    cd build/tests  
    cp launcher.py  ../build  
    cd ..  
    chmod 755 launcher.py  
    ./launcher.py  
    
#### Start Nodes
###### Node 1: 
    nodeos --http-server-address  localhost:8879  --http-validate-host=0 --enable-stale-production --producer-name eosio    --plugin eosio::chain_api_plugin --plugin eosio::net_api_plugin 
###### Node 2:
    nodeos --producer-name inita --plugin eosio::chain_api_plugin --plugin eosio::net_api_plugin --http-server-address  0.0.0.0:8889 --http-validate-host=0 --p2p-listen-endpoint :9877  --p2p-peer-address 0.0.0.0:9876 --config-dir node2 --data-dir node2 --private-key [\"EOS79vbwYtjhBVnBRYDjhCyxRFVr6JsFfVrLVhUKoqFTnceZtPvAU\",\"5JxUfAkXoCQdeZKNMhXEqRkFcZMYa3KR3vbie7SKsPv6rS3pCHg\"]

    
 
##### Chain Data Folder
Linux: `~/.local/share/eosio/nodeos/data`

Mac OS: `~/Library/Application Support/eosio/nodeos/data`

One way to confirm you're looking at the correct wallets directory is to do:

`cleos wallet open -n boguswalletname`


  
---  
  
### Useful Commands  
  
##### Set FIO Name Contract  
  
    cleos -u http://localhost:8889 set contract -j fio.system ~/fio/build/contracts/fio.name/ fio.name.wasm fio.name.abi --permission fio.system@active  
  
##### Set FIO Finance Contract  
  
    cleos -u http://localhost:8889 set contract -j fio.finance ~/fio/build/contracts/fio.finance/ fio.finance.wasm fio.finance.abi --permission fio.finance@active  
  
##### Create Domain  
  
    cleos -u http://localhost:8889 push action -j fio.system registername '{"name":"brd","requestor":"fioname11111"}' --permission fioname11111@active  
  
##### Create FIO Name  
  
     cleos -u http://localhost:8889 push action -j fio.system registername '{"name":"name.brd","requestor":"fioname11111"}' --permission fioname11111@active   
  
##### Add FIO Address  
  
    cleos -u http://localhost:8889 push action -j fio.system addaddress '{"fio_user_name":"name.brd","chain":"FIO","address":"FIOADDRESS","requestor":"fioname11111"}' --permission fioname11111@active  
  
##### Get Registered FIO Name  
  
    cleos -u http://localhost:8889 get table fio.system fio.system fionames  
  
##### Get Registered FIO Domain  
  
    cleos -u http://localhost:8889 get table fio.system fio.system fiodomains  
  
##### Test API Endpoints  
  
    curl --request POST  http://localhost:8888/v1/chain/avail_check --data '{"fio_name":"ese.brd"}'
