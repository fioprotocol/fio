/** fio system contract file
 *  Description: this contract controls many of the core functions of the fio protocol.
 *  @author Adam Androulidakis, Casey Gardiner, Ed Rotthoff
 *  @file fio.system.cpp
 *  @license FIO Foundation ( https://github.com/fioprotocol/fio/blob/master/LICENSE ) Dapix
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
              _topprods(_self, _self.value),
              _global(_self, _self.value),
              _global2(_self, _self.value),
              _global3(_self, _self.value),
              _lockedtokens(_self,_self.value),
              _generallockedtokens(_self, _self.value),
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

    void eosiosystem::system_contract::setpriv(const name &account, const uint8_t &ispriv) {
        require_auth(_self);
        set_privileged(account.value, ispriv);

        fio_400_assert(transaction_size() <= MAX_TRX_SIZE, "transaction_size", std::to_string(transaction_size()),
          "Transaction is too large", ErrorTransactionTooLarge);

    }

    void eosiosystem::system_contract::rmvproducer(const name &producer) {
        require_auth(_self);
        auto prod = _producers.find(producer.value);
        check(prod != _producers.end(), "producer not found");
        _producers.modify(prod, same_payer, [&](auto &p) {
            p.deactivate();
        });

        fio_400_assert(transaction_size() <= MAX_TRX_SIZE, "transaction_size", std::to_string(transaction_size()),
          "Transaction is too large", ErrorTransactionTooLarge);

    }

    void eosiosystem::system_contract::updtrevision(const uint8_t &revision) {
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
    void eosiosystem::native::newaccount(const name &creator,
                                         const name &newact,
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


       user_resources_table userres(_self, newact.value);

        userres.emplace(newact, [&](auto &res) {
            res.owner = newact;
            res.net_weight = asset(0, FIOSYMBOL);
            res.cpu_weight = asset(0, FIOSYMBOL);
        });

        set_resource_limits(newact.value, INITIALACCOUNTRAM, -1, -1);

        fio_400_assert(transaction_size() <= MAX_TRX_SIZE, "transaction_size", std::to_string(transaction_size()),
          "Transaction is too large", ErrorTransactionTooLarge);

    }


    void eosiosystem::native::setabi(const name &acnt, const std::vector<char> &abi) {

        require_auth(acnt);
        check((acnt == SYSTEMACCOUNT ||
                      acnt == MSIGACCOUNT ||
                      acnt == WRAPACCOUNT ||
                      acnt == ASSERTACCOUNT ||
                      acnt == REQOBTACCOUNT ||
                      acnt == FeeContract ||
                      acnt == AddressContract ||
                      acnt == TPIDContract ||
                      acnt == TokenContract ||
                      acnt == TREASURYACCOUNT ||
                      acnt == FIOSYSTEMACCOUNT ||
                      acnt == FIOACCOUNT),"set abi not permitted." );


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

    void eosiosystem::system_contract::init(const unsigned_int &version, const symbol &core) {
        require_auth(_self);
        check(version.value == 0, "unsupported version for init action");
    }

    //use this action to initialize the locked token holders table for the FIO protocol.
    void eosiosystem::system_contract::addlocked(const name &owner, const int64_t &amount,
            const int16_t &locktype) {
        require_auth(_self);

        check(is_account(owner),"account must pre exist");
        check(amount > 0,"cannot add locked token amount less or equal 0.");
        check(locktype == 1 || locktype == 2 || locktype == 3 || locktype == 4,"lock type must be 1,2,3,4");

        _lockedtokens.emplace(owner, [&](struct locked_token_holder_info &a) {
                a.owner = owner;
                a.total_grant_amount = amount;
                a.unlocked_period_count = 0;
                a.grant_type = locktype;
                a.inhibit_unlocking = 1;
                a.remaining_locked_amount = amount;
                a.timestamp = now();
            });
    }


    void eosiosystem::system_contract::addgenlocked(const name &owner, const vector<lockperiods> &periods, const bool &canvote,
            const int64_t &amount) {

        print(" calling addgenlocked for account ",owner,"\n");
        require_auth(TokenContract);


        check(is_account(owner),"account must pre exist");
        check(amount > 0,"cannot add locked token amount less or equal 0.");

        _generallockedtokens.emplace(owner, [&](struct locked_tokens_info &a) {
            a.id = _generallockedtokens.available_primary_key();
            a.owner_account = owner;
            a.lock_amount = amount;
            a.payouts_performed = 0;
            a.can_vote = canvote?1:0;
            a.periods = periods;
            a.remaining_lock_amount = amount;
            a.timestamp = now();
        });
        print(" completed calling addgenlocked for account ",owner,"\n");
    }

} /// fio.system


EOSIO_DISPATCH( eosiosystem::system_contract,
// native.hpp (newaccount definition is actually in fio.system.cpp)
(newaccount)(updateauth)(deleteauth)(linkauth)(unlinkauth)(canceldelay)(onerror)(setabi)
        // fio.system.cpp
        (init)(addlocked)(addgenlocked)(setparams)(setpriv)
        (rmvproducer)(updtrevision)
        // delegate_bandwidth.cpp
        (updatepower)
        // voting.cpp
        (regproducer)(regiproducer)(unregprod)(voteproducer)(voteproxy)(inhibitunlck)
        (updlocked)(unlocktokens)(setautoproxy)(crautoproxy)(burnaction)(incram)
        (unregproxy)(regiproxy)(regproxy)
        // producer_pay.cpp
        (onblock)
        (resetclaim)(updlbpclaim)
)
