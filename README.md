# FIOIO

#### Mac OS X Build & Install


    $ export LLVM_DIR=/usr/local/Cellar/llvm@4/4.0.1/lib/cmake/llvm; ./fioio_build.sh
    sudo ./eosio_install.sh


#### Launcher

    cd build
    chmod 755 launcher.py
    ./launcher.py


---

### Useful Commands

#### Set FIO Name Contract

    cleos -u http://localhost:8889 set contract -j fio.system ~/fio/build/contracts/fio.name/ fio.name.wasm fio.name.abi --permission fio.system@active

#### Create Domain

    cleos -u http://localhost:8889 push action -j fio.system registername '{"name":"brd","requestor":"fioname11111"}' --permission fioname11111@active

#### Create FIO Name

     cleos -u http://localhost:8889 push action -j fio.system registername '{"name":"name.brd","requestor":"fioname11111"}' --permission fioname11111@active

#### Add FIO Address

    cleos -u http://localhost:8889 push action -j fio.system addaddress '{"fio_user_name":"name.brd","chain":"FIO","address":"FIOADDRESS","requestor":"fioname11111"}' --permission fioname11111@active

#### Get Registered FIO Name

    cleos -u http://localhost:8889 get table fio.system fio.system fionames

#### Get Registered FIO Domain

    cleos -u http://localhost:8889 get table fio.system fio.system fiodomains

#### Test API Endpoints

    curl --request POST  http://localhost:8888/v1/chain/avail_check --data '{"fio_name":"ese.brd"}'


