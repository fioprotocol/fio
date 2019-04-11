        
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
| Domain     | dapix |
| FIO Names  | casey, adam |

#### Chain Management
###### Shutdown Local Test Node
Run: `pkill nodeos`


###### Signed Transaction

Run: `../utils/sign-pack-trx.sh` ( from build folder )

**Remove Folders Manually:<br>**
Linux: `~/.local/nodeos`<br>
Mac OS: `~/Library/Application Support/eosio/nodeos/`<br><br> `~../fio/build/programs/nodeos`<br>
 `~../fio/node2` or `~../fio/scripts/node2` depending on the location the nodes have been launched at. 

  
---  
  
### Useful Commands
https://github.com/dapixio/fio/wiki/Useful-Commands
