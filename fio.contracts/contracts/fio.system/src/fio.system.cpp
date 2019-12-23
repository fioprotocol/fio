/** fio system contract file
 *  Description: this contract controls many of the core functions of the fio protocol.
 *  @author Adam Androulidakis, Casey Gardiner, Ed Rotthoff
 *  @file fio.system.cpp
 *  @copyright Dapix
 *
 *  Changes:
 */
#include <fio.system/fio.system.hpp>
#include <eosiolib/dispatcher.hpp>
#include <eosiolib/crypto.h>
#include "producer_pay.cpp"
#include "delegate_bandwidth.cpp"
#include "voting.cpp"
#include <fio.address/fio.address.hpp>
#include <fio.fee/fio.fee.hpp>

namespace eosiosystem {

    system_contract::system_contract(name s, name code, datastream<const char *> ds)
            : native(s, code, ds),
              _voters(_self, _self.value),
              _producers(_self, _self.value),
              _global(_self, _self.value),
              _global2(_self, _self.value),
              _global3(_self, _self.value),
              _lockedtokens(_self,_self.value),
              _fionames(AddressContract, AddressContract.value),
              _domains(AddressContract, AddressContract.value),
              _accountmap(AddressContract, AddressContract.value),
              _fiofees(FeeContract, FeeContract.value){
        _gstate = _global.exists() ? _global.get() : get_default_parameters();
        _gstate2 = _global2.exists() ? _global2.get() : eosio_global_state2{};
        _gstate3 = _global3.exists() ? _global3.get() : eosio_global_state3{};
    }

    eosiosystem::eosio_global_state eosiosystem::system_contract::get_default_parameters() {
        eosio_global_state dp;
        get_blockchain_parameters(dp);
        return dp;
    }

    time_point eosiosystem::system_contract::current_time_point() {
        const static time_point ct{microseconds{static_cast<int64_t>( current_time())}};
        return ct;
    }

    time_point_sec eosiosystem::system_contract::current_time_point_sec() {
        const static time_point_sec cts{current_time_point()};
        return cts;
    }

    block_timestamp eosiosystem::system_contract::current_block_time() {
        const static block_timestamp cbt{current_time_point()};
        return cbt;
    }

    eosiosystem::system_contract::~system_contract() {
        _global.set(_gstate, _self);
        _global2.set(_gstate2, _self);
        _global3.set(_gstate3, _self);
    }

    void eosiosystem::system_contract::setparams(const eosio::blockchain_parameters &params) {
        require_auth(_self);
        (eosio::blockchain_parameters & )(_gstate) = params;
        check(3 <= _gstate.max_authority_depth, "max_authority_depth should be at least 3");
        set_blockchain_parameters(params);
    }

    void eosiosystem::system_contract::setpriv(name account, uint8_t ispriv) {
        require_auth(_self);
        set_privileged(account.value, ispriv);
    }

    void eosiosystem::system_contract::rmvproducer(name producer) {
        require_auth(_self);
        auto prod = _producers.find(producer.value);
        check(prod != _producers.end(), "producer not found");
        _producers.modify(prod, same_payer, [&](auto &p) {
            p.deactivate();
        });
    }

    void eosiosystem::system_contract::updtrevision(uint8_t revision) {
        require_auth(_self);
        check(_gstate2.revision < 255, "can not increment revision"); // prevent wrap around
        check(revision == _gstate2.revision + 1, "can only increment revision by one");
        check(revision <= 1, // set upper bound to greatest revision supported in the code
              "specified revision is not yet supported by the code");
        _gstate2.revision = revision;
    }

    /**
     *  Called after a new account is created. This code enforces resource-limits rules
     *  for new accounts as well as new account naming conventions.
     *
     *  Account names containing '.' symbols must have a suffix equal to the name of the creator.
     *  This allows users who buy a premium name (shorter than 12 characters with no dots) to be the only ones
     *  who can create accounts with the creator's name as a suffix.
     *
     */
    void eosiosystem::native::newaccount(name creator,
                                         name newact,
                                         ignore <authority> owner,
                                         ignore <authority> active) {


        require_auth(creator);
        
        check((creator == SYSTEMACCOUNT || creator == TokenContract ||
                 creator == AddressContract), "new account is not permitted");
        
        if (creator != _self) {
            uint64_t tmp = newact.value >> 4;
            bool has_dot = false;

            for (uint32_t i = 0; i < 12; ++i) {
                has_dot |= !(tmp & 0x1f);
                tmp >>= 5;
            }
            if (has_dot) { // or is less than 12 characters
                auto suffix = newact.suffix();
                if (suffix != newact) {
                    check(creator == suffix, "only suffix may create this account");
                }
            }
        }

       //in the FIO protocol we want all of our accounts to be created with unlimited
       //CPU NET and RAM. to do this we need our accounts to NOT have entrees in the
       //resources table. our accounts will all be unlimited CPU NET and RAM for the
       //foreseeable future of the FIO protocol.
       // user_resources_table userres(_self, newact.value);

       // userres.emplace(newact, [&](auto &res) {
       //     res.owner = newact;
           // res.net_weight = asset(0, system_contract::get_core_symbol());
           // res.cpu_weight = asset(0, system_contract::get_core_symbol());
       // });

       // set_resource_limits(newact.value, 0, 0, 0);
    }

    void eosiosystem::native::setabi(name acnt, const std::vector<char> &abi) {

        require_auth(acnt);

        check(( acnt == fioio::MSIGACCOUNT ||
                acnt == fioio::WHITELISTACCOUNT ||
                acnt == fioio::WRAPACCOUNT ||
                acnt == fioio::FeeContract ||
                acnt == fioio::AddressContract ||
                acnt == fioio::TPIDContract ||
                acnt == fioio::REQOBTACCOUNT ||
                acnt == fioio::TokenContract ||
                acnt == fioio::FOUNDATIONACCOUNT ||
                acnt == fioio::TREASURYACCOUNT ||
                acnt == fioio::SYSTEMACCOUNT), "setabi is not permitted");


        eosio::multi_index<"abihash"_n, abi_hash> table(_self, _self.value);
        auto itr = table.find(acnt.value);
        if (itr == table.end()) {
            table.emplace(acnt, [&](auto &row) {
                row.owner = acnt;
                sha256(const_cast<char *>(abi.data()), abi.size(), &row.hash);
            });
        } else {
            table.modify(itr, same_payer, [&](auto &row) {
                sha256(const_cast<char *>(abi.data()), abi.size(), &row.hash);
            });
        }
    }

    void eosiosystem::system_contract::init(unsigned_int version, symbol core) {
        require_auth(_self);
        check(version.value == 0, "unsupported version for init action");
    }

    //use this action to initialize the locked token holders table for the FIO protocol.
    void eosiosystem::system_contract::initlocked() {
        require_auth(_self);
        auto size = std::distance(_lockedtokens.cbegin(),_lockedtokens.cend());
        if(size > 0){
            //we disallow updates to the table once its initialized.
            return;
        }else{


            //// begin QA testing locked token holders
            // issue locked token grant to tdcfarsowlnk in the amount of 10000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "tdcfarsowlnk"_n;
                a.total_grant_amount = 10000;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 0;
                a.remaining_locked_amount = 10000;
                a.timestamp = now();
            });

// issue locked token grant to evorvygfnrzk in the amount of 20000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "evorvygfnrzk"_n;
                a.total_grant_amount = 20000;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 0;
                a.remaining_locked_amount = 20000;
                a.timestamp = now();
            });

// issue locked token grant to xbfugtkzvowu in the amount of 10000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "xbfugtkzvowu"_n;
                a.total_grant_amount = 10000;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 0;
                a.remaining_locked_amount = 10000;
                a.timestamp = now();
            });

// issue locked token grant to p1kv5e2zdxbh in the amount of 335000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "p1kv5e2zdxbh"_n;
                a.total_grant_amount = 335000;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 0;
                a.remaining_locked_amount = 335000;
                a.timestamp = now();
            });

// issue locked token grant to kk2gys4vl5ve in the amount of 99999
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "kk2gys4vl5ve"_n;
                a.total_grant_amount = 99999;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 0;
                a.remaining_locked_amount = 99999;
                a.timestamp = now();
            });

// issue locked token grant to jnp3viqz32tc in the amount of 85
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "jnp3viqz32tc"_n;
                a.total_grant_amount = 85;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 0;
                a.remaining_locked_amount = 85;
                a.timestamp = now();
            });

// issue locked token grant to hcfsdi2vybrv in the amount of 991099
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "hcfsdi2vybrv"_n;
                a.total_grant_amount = 991099;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 0;
                a.remaining_locked_amount = 991099;
                a.timestamp = now();
            });

// issue locked token grant to 125nkypgqojv in the amount of 20000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "125nkypgqojv"_n;
                a.total_grant_amount = 20000;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 20000;
                a.timestamp = now();
            });

// issue locked token grant to sauhngb2eq1c in the amount of 99991
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "sauhngb2eq1c"_n;
                a.total_grant_amount = 99991;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 99991;
                a.timestamp = now();
            });

// issue locked token grant to idmwqtsmij4i in the amount of 487921
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "idmwqtsmij4i"_n;
                a.total_grant_amount = 487921;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 487921;
                a.timestamp = now();
            });

// issue locked token grant to dq5q2kx5oioa in the amount of -86
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "dq5q2kx5oioa"_n;
                a.total_grant_amount = -86;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 0;
                a.remaining_locked_amount = -86;
                a.timestamp = now();
            });

// issue locked token grant to zdantmcgxej5 in the amount of 100
 /*           _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "zdantmcgxej5"_n;
                a.total_grant_amount = 100;
                a.unlocked_period_count = 0;
                a.grant_type = A;
                a.inhibit_unlocking = 0;
                a.remaining_locked_amount = 100;
                a.timestamp = now();
            });*/

// issue locked token grant to bxg2u5gpgoc2 in the amount of 100
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "bxg2u5gpgoc2"_n;
                a.total_grant_amount = 100;
                a.unlocked_period_count = 0;
                a.grant_type = 7;
                a.inhibit_unlocking = 0;
                a.remaining_locked_amount = 100;
                a.timestamp = now();
            });

// issue locked token grant to dffmxsxuq1gt in the amount of 100
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "dffmxsxuq1gt"_n;
                a.total_grant_amount = 100;
                a.unlocked_period_count = 3;
                a.grant_type = 1;
                a.inhibit_unlocking = 0;
                a.remaining_locked_amount = 100;
                a.timestamp = now();
            });

// issue locked token grant to fidgtwmzrrjq in the amount of 500
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "fidgtwmzrrjq"_n;
                a.total_grant_amount = 500;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 0;
                a.remaining_locked_amount = 200;
                a.timestamp = now();
            });

// issue locked token grant to lfou4iag21qj in the amount of 0
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "lfou4iag21qj"_n;
                a.total_grant_amount = 0;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 0;
                a.remaining_locked_amount = 0;
                a.timestamp = now();
            });

// issue locked token grant to rbwfkv4s2h1y in the amount of 10000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "rbwfkv4s2h1y"_n;
                a.total_grant_amount = 10000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 0;
                a.remaining_locked_amount = 10000;
                a.timestamp = now();
            });

// issue locked token grant to kthvzmbsqywi in the amount of 20000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "kthvzmbsqywi"_n;
                a.total_grant_amount = 20000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 0;
                a.remaining_locked_amount = 20000;
                a.timestamp = now();
            });

// issue locked token grant to pk3jwhi42yvs in the amount of 10000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "pk3jwhi42yvs"_n;
                a.total_grant_amount = 10000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking =0;
                a.remaining_locked_amount = 10000;
                a.timestamp = now();
            });

// issue locked token grant to koa4cmk2dnn4 in the amount of 335000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "koa4cmk2dnn4"_n;
                a.total_grant_amount = 335000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking =0;
                a.remaining_locked_amount = 335000;
                a.timestamp = now();
            });

// issue locked token grant to fnfyxcmbqkqe in the amount of 999999
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "fnfyxcmbqkqe"_n;
                a.total_grant_amount = 999999;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking =0;
                a.remaining_locked_amount = 999999;
                a.timestamp = now();
            });

// issue locked token grant to nmhwljvqwifd in the amount of 85
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "nmhwljvqwifd"_n;
                a.total_grant_amount = 85;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 0;
                a.remaining_locked_amount = 85;
                a.timestamp = now();
            });

// issue locked token grant to ncmmejvfrgyb in the amount of 991099
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "ncmmejvfrgyb"_n;
                a.total_grant_amount = 991099;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 0;
                a.remaining_locked_amount = 991099;
                a.timestamp = now();
            });

// issue locked token grant to bonxjq4wfrdu in the amount of 10000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "bonxjq4wfrdu"_n;
                a.total_grant_amount = 10000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 10000;
                a.timestamp = now();
            });

// issue locked token grant to sk12qhnnzejl in the amount of 20000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "sk12qhnnzejl"_n;
                a.total_grant_amount = 20000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 20000;
                a.timestamp = now();
            });

// issue locked token grant to ntbvtb3d2fr4 in the amount of 10000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "ntbvtb3d2fr4"_n;
                a.total_grant_amount = 10000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 10000;
                a.timestamp = now();
            });

// issue locked token grant to hlpiinzs5zd5 in the amount of 335000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "hlpiinzs5zd5"_n;
                a.total_grant_amount = 335000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 335000;
                a.timestamp = now();
            });

// issue locked token grant to selo5rapyqgw in the amount of 999999
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "selo5rapyqgw"_n;
                a.total_grant_amount = 999999;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 999999;
                a.timestamp = now();
            });

// issue locked token grant to dzg53yiqag3t in the amount of 85
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "dzg53yiqag3t"_n;
                a.total_grant_amount = 85;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 85;
                a.timestamp = now();
            });

// issue locked token grant to qqwr22af3q3w in the amount of 991099
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "qqwr22af3q3w"_n;
                a.total_grant_amount = 991099;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 991099;
                a.timestamp = now();
            });

// issue locked token grant to fetnc4fljrpe in the amount of -86
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "fetnc4fljrpe"_n;
                a.total_grant_amount = -86;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 0;
                a.remaining_locked_amount = -86;
                a.timestamp = now();
            });

// issue locked token grant to ahr2zcuaycen in the amount of 100
   /*         _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "ahr2zcuaycen"_n;
                a.total_grant_amount = 100;
                a.unlocked_period_count = 0;
                a.grant_type = Z;
                a.inhibit_unlocking = 0;
                a.remaining_locked_amount = 100;
                a.timestamp = now();
            });*/

// issue locked token grant to phyffmjubnjn in the amount of 100
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "phyffmjubnjn"_n;
                a.total_grant_amount = 100;
                a.unlocked_period_count = 0;
                a.grant_type = 9;
                a.inhibit_unlocking = 0;
                a.remaining_locked_amount = 100;
                a.timestamp = now();
            });

// issue locked token grant to 5tsjifjfaexx in the amount of 100
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "5tsjifjfaexx"_n;
                a.total_grant_amount = 100;
                a.unlocked_period_count = 3;
                a.grant_type = 2;
                a.inhibit_unlocking = 0;
                a.remaining_locked_amount = 100;
                a.timestamp = now();
            });

// issue locked token grant to o3tsy1uuewzt in the amount of 500
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "o3tsy1uuewzt"_n;
                a.total_grant_amount = 500;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 0;
                a.remaining_locked_amount = 500;
                a.timestamp = now();
            });

// issue locked token grant to 5ojmuyw3cenb in the amount of -86
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "5ojmuyw3cenb"_n;
                a.total_grant_amount = -86;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = -86;
                a.timestamp = now();
            });

// issue locked token grant to 5gem1tuvtbqc in the amount of 100
      /*      _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "5gem1tuvtbqc"_n;
                a.total_grant_amount = 100;
                a.unlocked_period_count = 0;
                a.grant_type = Z;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 100;
                a.timestamp = now();
            });*/

// issue locked token grant to 1vqxm4g3rkwa in the amount of 100
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "1vqxm4g3rkwa"_n;
                a.total_grant_amount = 100;
                a.unlocked_period_count = 0;
                a.grant_type = 9;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 100;
                a.timestamp = now();
            });

// issue locked token grant to v1auz25wfon1 in the amount of 100
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "v1auz25wfon1"_n;
                a.total_grant_amount = 100;
                a.unlocked_period_count = 3;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 100;
                a.timestamp = now();
            });

// issue locked token grant to tlfpakohqbbj in the amount of 500
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "tlfpakohqbbj"_n;
                a.total_grant_amount = 500;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 200;
                a.timestamp = now();
            });

// issue locked token grant to ertapz2qxqrx in the amount of 1000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "ertapz2qxqrx"_n;
                a.total_grant_amount = 1000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 6;
                a.remaining_locked_amount = 1000;
                a.timestamp = now();
            });




            //// end QA testing locked token holders





            //test cases

            //locked token holder UAT testing.


            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "o3jvcxorf4qu"_n;
                a.total_grant_amount = 1000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 1000000000000;
                a.timestamp = now();
            });

            //////////////unlocking tests inits.

            //  grant type 1, total grant  equals amount in account, and remaining_locked_amount is 'close' in value
            //     to the total grant.
            //     expected result   1/3 of total grant should be used as voting.
            // //  set now kludge in unlock_tokens to be (0*SECONDSPERDAY);
            // or set now kludge to 800 * SECONDSPERDAY
         /*  _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "euwdcp13zlrj"_n;
                a.total_grant_amount = 2000000000000;
                a.unlocked_period_count = 2;
                a.grant_type = 1;
                a.inhibit_unlocking = 0;
                a.remaining_locked_amount = 1632500000000;

            });
            */


            //  grant type 1, total grant  equals amount in account, and remaining_locked_amount is 'close' in value
            //     to the total grant.
            //     expected result   1/3 of total grant should be used as voting.
            //  set now kludge in unlock_tokens to be (181*SECONDSPERDAY);
             _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                  a.owner = "euwdcp13zlrj"_n;
                  a.total_grant_amount = 2000000000000;
                  a.unlocked_period_count = 1;
                  a.grant_type = 1;
                  a.inhibit_unlocking = 1;
                  a.remaining_locked_amount = 2000000000000;
                  a.timestamp = now();

              });


            // 1) grant type 2, total grant  equals amount in account, and remaining_locked_amount is 'close' in value
            //     to the total grant.
            //     expected result   1/3 of total grant should be used as voting.
            /*
           _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "euwdcp13zlrj"_n;
                a.total_grant_amount = 2000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 0;
                a.remaining_locked_amount = 1900000000000;
                a.timestamp = now();
            });
             */



            // 1) grant type 2, total grant  equals amount in account, and remaining_locked_amount is 'close' in value
            //     to the total grant.
            //     expected result   1/3 of total grant should be used as voting.
          /*    _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                  a.owner = "euwdcp13zlrj"_n;
                  a.total_grant_amount = 2000000000000;
                  a.unlocked_period_count = 1;
                  a.grant_type = 2;
                  a.inhibit_unlocking = 0;
                  a.remaining_locked_amount = 1860000000000;

              });
              */




            // voting tests inits

            // 1) grant type 1, total grant  equals amount in account, and remaining_locked_amount is 'close' in value
            //     to the total grant.
            //     expected result   1/3 of total grant should be used as voting.
           /* _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "euwdcp13zlrj"_n;
                a.total_grant_amount = 2000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 0;
                a.remaining_locked_amount = 1900000000000;

            });
            */




            // 2) grant type 1, total grant equals amount in account, and remaining_locked_amount is small compared
            //     to the total grant.
            //    expected result, the amount in the account is the voting weight.
          /*   _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
               a.owner = "euwdcp13zlrj"_n;
               a.total_grant_amount = 2000000000000;
               a.unlocked_period_count = 0;
               a.grant_type = 1;
               a.inhibit_unlocking = 0;
               a.remaining_locked_amount = 5000000000;


           });*/


            // 3) grant type is 2, the grant amount equals the amount in the account, the remaining_locked_amount
            //     is close in value to the grant amount, the 210 day limit is not yet occuring,
            //       the flag for inhibit unlocking is 0.
            //    expected result, the voting weight is the amount in the account.
          /*   _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
              a.owner = "euwdcp13zlrj"_n;
              a.total_grant_amount = 2000000000000;
              a.unlocked_period_count = 0;
              a.grant_type = 2;
              a.inhibit_unlocking = 0;
              a.remaining_locked_amount = 1900000000000;

          });
           */


            // 4) grant type is 2, the grant amount equals the amount in the account,he remaining_locked_amount
            //      is close in value to the grant amount, the 210 day limit is not yet occuring,
            //       the flag for inhibit unlocking is 1.
            //    expected result, the voting weight is the amount in the account.
          /*   _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
              a.owner = "euwdcp13zlrj"_n;
              a.total_grant_amount = 2000000000000;
              a.unlocked_period_count = 0;
              a.grant_type = 2;
              a.inhibit_unlocking = 1;
              a.remaining_locked_amount = 1900000000000;

          });
           */


            // 5) grant type is 2, the grant amount equals the amount in the account, the 210 day limit has occurred
            //       the flag for inhibit unlocking is 0.
            //    expected result, the voting weight is the amount in the account.
          /*   _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
             a.owner = "euwdcp13zlrj"_n;
             a.total_grant_amount = 2000000000000;
             a.unlocked_period_count = 0;
             a.grant_type = 2;
             a.inhibit_unlocking = 0;
             a.remaining_locked_amount = 1900000000000;
             //also uncomment the testing kludge for FIOLAUNCHPLUS210DAYS in fio.common.hpp
             });
             */



            // 6) grant type is 2, the grant amount equals the amount in the account, the 210 day limit has occurred
            //       the flag for inhibit unlocking is 1.
            //    expected result, the voting weight is the amount in the account - remaining locked amount.
           /*  _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                       a.owner = "euwdcp13zlrj"_n;
                       a.total_grant_amount = 2000000000000;
                       a.unlocked_period_count = 0;
                       a.grant_type = 2;
                       a.inhibit_unlocking = 1;
                       a.remaining_locked_amount = 1900000000000;
                       //also uncomment the testing kludge for FIOLAUNCHPLUS210DAYS in fio.common.hpp
                   });
                   */
           

           //////////////////////////////////begin token grant

            // issue locked token grant to qkd1aff1bf54 in the amount of 115104522257423000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "qkd1aff1bf54"_n;
                a.total_grant_amount = 115104522257423000;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 115104522257423000;
                a.timestamp = now();
            });

// issue locked token grant to s3zrkumbtnnt in the amount of 31678227029760900
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "s3zrkumbtnnt"_n;
                a.total_grant_amount = 31678227029760900;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 31678227029760900;
                a.timestamp = now();
            });

// issue locked token grant to hpdlvkgna4vk in the amount of 26834384982415500
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "hpdlvkgna4vk"_n;
                a.total_grant_amount = 26834384982415500;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 26834384982415500;
                a.timestamp = now();
            });

// issue locked token grant to wkdqs4adsghm in the amount of 10000000000000000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "wkdqs4adsghm"_n;
                a.total_grant_amount = 10000000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 10000000000000000;
                a.timestamp = now();
            });

// issue locked token grant to 3c1ddfeu1o3m in the amount of 10000000000000000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "3c1ddfeu1o3m"_n;
                a.total_grant_amount = 10000000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 10000000000000000;
                a.timestamp = now();
            });

// issue locked token grant to vaapbgmd4xiv in the amount of 10000000000000000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "vaapbgmd4xiv"_n;
                a.total_grant_amount = 10000000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 10000000000000000;
                a.timestamp = now();
            });

// issue locked token grant to uekabatb1key in the amount of 10000000000000000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "uekabatb1key"_n;
                a.total_grant_amount = 10000000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 10000000000000000;
                a.timestamp = now();
            });

// issue locked token grant to npsaumrdky4a in the amount of 10000000000000000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "npsaumrdky4a"_n;
                a.total_grant_amount = 10000000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 10000000000000000;
                a.timestamp = now();
            });

// issue locked token grant to 5i2ck2m4y4sc in the amount of 8500000000000000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "5i2ck2m4y4sc"_n;
                a.total_grant_amount = 8500000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 8500000000000000;
                a.timestamp = now();
            });

// issue locked token grant to b4jpixuqelvi in the amount of 7007834149201920
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "b4jpixuqelvi"_n;
                a.total_grant_amount = 7007834149201920;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 7007834149201920;
                a.timestamp = now();
            });

// issue locked token grant to 2pt22dsscen5 in the amount of 7000000000000000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "2pt22dsscen5"_n;
                a.total_grant_amount = 7000000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 7000000000000000;
                a.timestamp = now();
            });

// issue locked token grant to wymok3sxp4hn in the amount of 5965122594634680
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "wymok3sxp4hn"_n;
                a.total_grant_amount = 5965122594634680;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 5965122594634680;
                a.timestamp = now();
            });

// issue locked token grant to 45pdy1jm3iye in the amount of 5500000000000000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "45pdy1jm3iye"_n;
                a.total_grant_amount = 5500000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 5500000000000000;
                a.timestamp = now();
            });

// issue locked token grant to s4vwxmyy4cvz in the amount of 4000000000000000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "s4vwxmyy4cvz"_n;
                a.total_grant_amount = 4000000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 4000000000000000;
                a.timestamp = now();
            });

// issue locked token grant to zhqhuylm3dm1 in the amount of 3530594240347680
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "zhqhuylm3dm1"_n;
                a.total_grant_amount = 3530594240347680;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 3530594240347680;
                a.timestamp = now();
            });

// issue locked token grant to tipu4vd32ryh in the amount of 3489969114845060
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "tipu4vd32ryh"_n;
                a.total_grant_amount = 3489969114845060;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 3489969114845060;
                a.timestamp = now();
            });

// issue locked token grant to mjaymcfrxjoh in the amount of 3385427125218320
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "mjaymcfrxjoh"_n;
                a.total_grant_amount = 3385427125218320;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 3385427125218320;
                a.timestamp = now();
            });

// issue locked token grant to bhli4ljt3beo in the amount of 3000000000000000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "bhli4ljt3beo"_n;
                a.total_grant_amount = 3000000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 3000000000000000;
                a.timestamp = now();
            });

// issue locked token grant to hwz23antnsw1 in the amount of 2708341700174660
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "hwz23antnsw1"_n;
                a.total_grant_amount = 2708341700174660;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 2708341700174660;
                a.timestamp = now();
            });

// issue locked token grant to ylpftm4jd4ie in the amount of 2500000000000000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "ylpftm4jd4ie"_n;
                a.total_grant_amount = 2500000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 2500000000000000;
                a.timestamp = now();
            });

// issue locked token grant to qsb2gjidztbj in the amount of 2000000000000000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "qsb2gjidztbj"_n;
                a.total_grant_amount = 2000000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 2000000000000000;
                a.timestamp = now();
            });

// issue locked token grant to wzlkvt5yj5an in the amount of 2000000000000000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "wzlkvt5yj5an"_n;
                a.total_grant_amount = 2000000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 2000000000000000;
                a.timestamp = now();
            });

// issue locked token grant to cxqnxtuocmy2 in the amount of 1895839190122260
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "cxqnxtuocmy2"_n;
                a.total_grant_amount = 1895839190122260;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 1895839190122260;
                a.timestamp = now();
            });

// issue locked token grant to xe3lctblyre4 in the amount of 1800000000000000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "xe3lctblyre4"_n;
                a.total_grant_amount = 1800000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 1800000000000000;
                a.timestamp = now();
            });

// issue locked token grant to nadppzyxtxjx in the amount of 1500000000000000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "nadppzyxtxjx"_n;
                a.total_grant_amount = 1500000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 1500000000000000;
                a.timestamp = now();
            });

// issue locked token grant to fcauvsmk4k22 in the amount of 1300000000000000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "fcauvsmk4k22"_n;
                a.total_grant_amount = 1300000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 1300000000000000;
                a.timestamp = now();
            });

// issue locked token grant to 5dqft4wdc2db in the amount of 541668340034931
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "5dqft4wdc2db"_n;
                a.total_grant_amount = 541668340034931;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 541668340034931;
                a.timestamp = now();
            });

// issue locked token grant to idc4gmq4rgal in the amount of 406251255026199
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "idc4gmq4rgal"_n;
                a.total_grant_amount = 406251255026199;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 406251255026199;
                a.timestamp = now();
            });

// issue locked token grant to 2qpbxzrn35vz in the amount of 1000000000000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "2qpbxzrn35vz"_n;
                a.total_grant_amount = 1000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 1000000000000;
                a.timestamp = now();
            });

// issue locked token grant to mkfhpfljzbfp in the amount of 1000000000000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "mkfhpfljzbfp"_n;
                a.total_grant_amount = 1000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 1000000000000;
                a.timestamp = now();
            });

// issue locked token grant to eabveqqym1wa in the amount of 1000000000000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "eabveqqym1wa"_n;
                a.total_grant_amount = 1000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 1000000000000;
                a.timestamp = now();
            });

// issue locked token grant to d23aut35itiu in the amount of 1000000000000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "d23aut35itiu"_n;
                a.total_grant_amount = 1000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 1000000000000;
                a.timestamp = now();
            });

// issue locked token grant to 3ycd3j3slv3o in the amount of 1000000000000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "3ycd3j3slv3o"_n;
                a.total_grant_amount = 1000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 1000000000000;
                a.timestamp = now();
            });

// issue locked token grant to gc2cuzibmsul in the amount of 1000000000000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "gc2cuzibmsul"_n;
                a.total_grant_amount = 1000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 1000000000000;
                a.timestamp = now();
            });

// issue locked token grant to ufvugm1vs3ye in the amount of 1000000000000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "ufvugm1vs3ye"_n;
                a.total_grant_amount = 1000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 1000000000000;
                a.timestamp = now();
            });

// issue locked token grant to 5z4dmr3elv54 in the amount of 1000000000000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "5z4dmr3elv54"_n;
                a.total_grant_amount = 1000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 1000000000000;
                a.timestamp = now();
            });

// issue locked token grant to vcszbfqmlnpw in the amount of 1000000000000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "vcszbfqmlnpw"_n;
                a.total_grant_amount = 1000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 1000000000000;
                a.timestamp = now();
            });

// issue locked token grant to uwnnph1avy4a in the amount of 1000000000000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "uwnnph1avy4a"_n;
                a.total_grant_amount = 1000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 1000000000000;
                a.timestamp = now();
            });

// issue locked token grant to s35yqzbjrubv in the amount of 1000000000000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "s35yqzbjrubv"_n;
                a.total_grant_amount = 1000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 1000000000000;
                a.timestamp = now();
            });

// issue locked token grant to wadhixhfw1kh in the amount of 1000000000000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "wadhixhfw1kh"_n;
                a.total_grant_amount = 1000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 1000000000000;
                a.timestamp = now();
            });

// issue locked token grant to 4injhqwxctby in the amount of 1000000000000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "4injhqwxctby"_n;
                a.total_grant_amount = 1000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 1000000000000;
                a.timestamp = now();
            });

// issue locked token grant to q2w4k4mswmtj in the amount of 1000000000000
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "q2w4k4mswmtj"_n;
                a.total_grant_amount = 1000000000000;
                a.unlocked_period_count = 0;
                a.grant_type = 2;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 1000000000000;
                a.timestamp = now();
            });

// issue locked token grant to qa2y4o4cpcvj in the amount of 0
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "qa2y4o4cpcvj"_n;
                a.total_grant_amount = 0;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 0;
                a.timestamp = now();
            });

// issue locked token grant to cmtp2knsn3wj in the amount of 0
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "cmtp2knsn3wj"_n;
                a.total_grant_amount = 0;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 0;
                a.timestamp = now();
            });

// issue locked token grant to rq3bitethod3 in the amount of 0
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "rq3bitethod3"_n;
                a.total_grant_amount = 0;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 0;
                a.timestamp = now();
            });

// issue locked token grant to bti22jpq1fdc in the amount of 0
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "bti22jpq1fdc"_n;
                a.total_grant_amount = 0;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 0;
                a.timestamp = now();
            });

// issue locked token grant to ysktedzjdzm2 in the amount of 0
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "ysktedzjdzm2"_n;
                a.total_grant_amount = 0;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 0;
                a.timestamp = now();
            });

// issue locked token grant to arrbpyo1zmrq in the amount of 0
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "arrbpyo1zmrq"_n;
                a.total_grant_amount = 0;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 0;
                a.timestamp = now();
            });

// issue locked token grant to jtbjxejtww13 in the amount of 0
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "jtbjxejtww13"_n;
                a.total_grant_amount = 0;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 0;
                a.timestamp = now();
            });

// issue locked token grant to dzpfsuivzdjm in the amount of 0
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "dzpfsuivzdjm"_n;
                a.total_grant_amount = 0;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 0;
                a.timestamp = now();
            });

// issue locked token grant to vdxkfs1gb5yo in the amount of 0
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "vdxkfs1gb5yo"_n;
                a.total_grant_amount = 0;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 0;
                a.timestamp = now();
            });

// issue locked token grant to ntnziscgq4mg in the amount of 0
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "ntnziscgq4mg"_n;
                a.total_grant_amount = 0;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 0;
                a.timestamp = now();
            });

// issue locked token grant to rcgkeoz1klx2 in the amount of 0
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "rcgkeoz1klx2"_n;
                a.total_grant_amount = 0;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 0;
                a.timestamp = now();
            });

// issue locked token grant to adyi1etuquwi in the amount of 0
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "adyi1etuquwi"_n;
                a.total_grant_amount = 0;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 0;
                a.timestamp = now();
            });

// issue locked token grant to 3duj2x1dyved in the amount of 0
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "3duj2x1dyved"_n;
                a.total_grant_amount = 0;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 0;
                a.timestamp = now();
            });

// issue locked token grant to m2aocihsrqzz in the amount of 0
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "m2aocihsrqzz"_n;
                a.total_grant_amount = 0;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 0;
                a.timestamp = now();
            });

// issue locked token grant to wiymqfhpxs3a in the amount of 0
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "wiymqfhpxs3a"_n;
                a.total_grant_amount = 0;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 0;
                a.timestamp = now();
            });

// issue locked token grant to gh4jpmcswrn1 in the amount of 0
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "gh4jpmcswrn1"_n;
                a.total_grant_amount = 0;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 0;
                a.timestamp = now();
            });

// issue locked token grant to 3oqjgbvxuis3 in the amount of 0
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "3oqjgbvxuis3"_n;
                a.total_grant_amount = 0;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 0;
                a.timestamp = now();
            });

// issue locked token grant to zsnucevtuagq in the amount of 0
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "zsnucevtuagq"_n;
                a.total_grant_amount = 0;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 0;
                a.timestamp = now();
            });

// issue locked token grant to cuheww5pghoy in the amount of 0
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "cuheww5pghoy"_n;
                a.total_grant_amount = 0;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 0;
                a.timestamp = now();
            });

// issue locked token grant to apvknxyplddx in the amount of 0
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "apvknxyplddx"_n;
                a.total_grant_amount = 0;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 0;
                a.timestamp = now();
            });

// issue locked token grant to gnardcwar3el in the amount of 0
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "gnardcwar3el"_n;
                a.total_grant_amount = 0;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 0;
                a.timestamp = now();
            });

// issue locked token grant to kjqsmvr1zxwy in the amount of 0
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "kjqsmvr1zxwy"_n;
                a.total_grant_amount = 0;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 0;
                a.timestamp = now();
            });

// issue locked token grant to un4fiidd1ua1 in the amount of 0
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "un4fiidd1ua1"_n;
                a.total_grant_amount = 0;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 0;
                a.timestamp = now();
            });

// issue locked token grant to hms4cymjqdjk in the amount of 0
            _lockedtokens.emplace(_self, [&](struct locked_token_holder_info &a) {
                a.owner = "hms4cymjqdjk"_n;
                a.total_grant_amount = 0;
                a.unlocked_period_count = 0;
                a.grant_type = 1;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = 0;
                a.timestamp = now();
            });



            //////////////////////////////////end token grant



        }

    }

} /// fio.system


EOSIO_DISPATCH( eosiosystem::system_contract,
// native.hpp (newaccount definition is actually in fio.system.cpp)
(newaccount)(updateauth)(deleteauth)(linkauth)(unlinkauth)(canceldelay)(onerror)(setabi)
        // fio.system.cpp
        (init)(initlocked)(setparams)(setpriv)
        (rmvproducer)(updtrevision)
        // delegate_bandwidth.cpp
        (updatepower)
        // voting.cpp
        (regproducer)(regiproducer)(unregprod)(voteproducer)(voteproxy)(inhibitunlck)
        (updlocked)(unlocktokens)(setautoproxy)(crautoproxy)
        (unregproxy)(regiproxy)(regproxy)
        // producer_pay.cpp
        (onblock)
        (resetclaim)
)
