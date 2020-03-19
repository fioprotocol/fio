/**
 *  @file
 *  @license FIO Foundation ( https://github.com/fioprotocol/fio/blob/master/LICENSE ) defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/action.hpp>
#include <eosiolib/public_key.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/privileged.h>
#include <eosiolib/producer_schedule.hpp>
#include <eosiolib/contract.hpp>
#include <eosiolib/ignore.hpp>
#include "fio.common/fio.accounts.hpp"
#include "fio.common/fioerror.hpp"


using namespace fioio;
namespace eosiosystem {
    using eosio::name;
    using eosio::permission_level;
    using eosio::public_key;
    using eosio::ignore;
    using eosio::check;


    struct permission_level_weight {
        permission_level permission;
        uint16_t weight;

        // explicit serialization macro is not necessary, used here only to improve compilation time
        EOSLIB_SERIALIZE( permission_level_weight, (permission)(weight))
    };

    struct key_weight {
        eosio::public_key key;
        uint16_t weight;

        // explicit serialization macro is not necessary, used here only to improve compilation time
        EOSLIB_SERIALIZE( key_weight, (key)(weight))
    };

    struct wait_weight {
        uint32_t wait_sec;
        uint16_t weight;

        // explicit serialization macro is not necessary, used here only to improve compilation time
        EOSLIB_SERIALIZE( wait_weight, (wait_sec)(weight))
    };

    struct authority {
        uint32_t threshold = 0;
        std::vector <key_weight> keys;
        std::vector <permission_level_weight> accounts;
        std::vector <wait_weight> waits;

        // explicit serialization macro is not necessary, used here only to improve compilation time
        EOSLIB_SERIALIZE( authority, (threshold)(keys)(accounts)(waits))
    };

    struct block_header {
        uint32_t timestamp;
        name producer;
        uint16_t confirmed = 0;
        capi_checksum256 previous;
        capi_checksum256 transaction_mroot;
        capi_checksum256 action_mroot;
        uint32_t schedule_version = 0;
        std::optional <eosio::producer_schedule> new_producers;

        // explicit serialization macro is not necessary, used here only to improve compilation time
        EOSLIB_SERIALIZE(block_header, (timestamp)(producer)(confirmed)(previous)(transaction_mroot)(action_mroot)
                (schedule_version)(new_producers))
    };


    struct [[eosio::table("abihash"), eosio::contract("fio.system")]] abi_hash {
        name owner;
        capi_checksum256 hash;

        uint64_t primary_key() const { return owner.value; }

        EOSLIB_SERIALIZE( abi_hash, (owner)(hash)
        )
    };

    static const uint64_t LINKAUTHRAM = 1024; //integrated.
    static const uint64_t REGPRODUCERRAM = 1024; //integrated.
    static const uint64_t REGPROXYRAM = 1024;//integrated.
    static const uint64_t VOTEPROXYRAM = 512; //integrated.
    static const uint64_t VOTEPRODUCERRAM = 1024; //integrated.
    static const uint64_t UPDATEAUTHRAM = 1024; //integrated.

    /*
     * Method parameters commented out to prevent generation of code that parses input data.
     */
    class [[eosio::contract("fio.system")]] native : public eosio::contract {
    public:

        using eosio::contract::contract;


        /**
         *  Called after a new account is created. This code enforces resource-limits rules
         *  for new accounts as well as new account naming conventions.
         *
         *  1. accounts cannot contain '.' symbols which forces all acccounts to be 12
         *  characters long without '.' until a future account auction process is implemented
         *  which prevents name squatting.
         *
         *  2. new accounts must stake a minimal number of tokens (as set in system parameters)
         *     therefore, this method will execute an inline buyram from receiver for newacnt in
         *     an amount equal to the current new account creation fee.
         */
        [[eosio::action]]
        void newaccount(name creator,
                        name name,
                        ignore <authority> owner,
                        ignore <authority> active);


        [[eosio::action]]
        void updateauth(name account,
                        name permission,
                        name parent,
                        authority auth,
                        const uint64_t max_fee) {
            require_auth(account);

            //check the list of system accounts, if it is one of these do not charge the fees.
            if(!(account == fioio::MSIGACCOUNT ||
                 account == fioio::WRAPACCOUNT ||
                 account == fioio::SYSTEMACCOUNT ||
                 account == fioio::ASSERTACCOUNT ||
                 account == fioio::REQOBTACCOUNT ||
                 account == fioio::FeeContract ||
                 account == fioio::AddressContract ||
                 account == fioio::TPIDContract ||
                 account == fioio::TokenContract ||
                 account == fioio::TREASURYACCOUNT ||
                 account == fioio::FIOSYSTEMACCOUNT ||
                 account == fioio::FIOACCOUNT)
                ) {

                //get the sizes of the tx.
                uint64_t sizep = transaction_size();

                eosio::action{
                        permission_level{account, "active"_n},
                        fioio::FeeContract, "bytemandfee"_n,
                        std::make_tuple(std::string("auth_update"), account, max_fee,sizep)
                }.send();
            }

            if (permission == fioio::ACTIVE || permission == fioio::OWNER){
                eosio_assert((auth.keys.size() == 0) || (auth.keys.size() == 1),
                             "update auth not permitted on owner or active unless keys is empty or has a single entry matching the account public key");
                //todo add code to check that if there is a single auth key, the key matches the value in the account map.
            }

            fio_400_assert(auth.waits.size() == 0, "authorization_waits", "authorization_waits",
                           "Waits not supported", ErrorNoAuthWaits);

            if (UPDATEAUTHRAM > 0) {
                action(
                        permission_level{SYSTEMACCOUNT, "active"_n},
                        "eosio"_n,
                        "incram"_n,
                        std::make_tuple(account, UPDATEAUTHRAM)
                ).send();
            }

        }

        [[eosio::action]]
        void deleteauth(name account,
                        name permission,
                        const uint64_t max_fee) {
            require_auth(account);

            eosio::action{
                   permission_level{account, "active"_n},
                   fioio::FeeContract, "mandatoryfee"_n,
                   std::make_tuple(std::string("auth_delete"), account, max_fee)
            }.send();

            fio_400_assert(transaction_size() <= MAX_TRX_SIZE, "transaction_size", std::to_string(transaction_size()),
              "Transaction is too large", ErrorTransaction);
        }

        [[eosio::action]]
        void linkauth(name account,
                      name code,
                      name type,
                      name requirement,
                      uint64_t max_fee) {

            require_auth(account);

            eosio::action{
                    permission_level{account, "active"_n},
                    fioio::FeeContract, "mandatoryfee"_n,
                    std::make_tuple(std::string("auth_link"), account, max_fee)
            }.send();

            if (LINKAUTHRAM > 0) {
                action(
                        permission_level{SYSTEMACCOUNT, "active"_n},
                        "eosio"_n,
                        "incram"_n,
                        std::make_tuple(account, LINKAUTHRAM)
                ).send();
            }

            fio_400_assert(transaction_size() <= MAX_TRX_SIZE, "transaction_size", std::to_string(transaction_size()),
              "Transaction is too large", ErrorTransaction);
        }

        [[eosio::action]]
        void unlinkauth(name account,
                        ignore <name> code,
                        ignore <name> type) {
            require_auth(account);
            //let them do this as much as they can for free since it decreases state size.
        }

        [[eosio::action]]
        void canceldelay(ignore <permission_level> canceling_auth, ignore <capi_checksum256> trx_id) {
            require_auth(_self);
        }

        [[eosio::action]]
        void onerror(ignore <uint128_t> sender_id, ignore <std::vector<char>> sent_trx) {
            require_auth(_self);
        }

        [[eosio::action]]
        void setabi(name account, const std::vector<char> &abi);

        [[eosio::action]]
        void setcode(name account, uint8_t vmtype, uint8_t vmversion, const std::vector<char> &code);
        //special note, dont add code here, setcode will not run this code.

        using newaccount_action = eosio::action_wrapper<"newaccount"_n, &native::newaccount>;
        using updateauth_action = eosio::action_wrapper<"updateauth"_n, &native::updateauth>;
        using deleteauth_action = eosio::action_wrapper<"deleteauth"_n, &native::deleteauth>;
        using linkauth_action = eosio::action_wrapper<"linkauth"_n, &native::linkauth>;
        using unlinkauth_action = eosio::action_wrapper<"unlinkauth"_n, &native::unlinkauth>;
        using canceldelay_action = eosio::action_wrapper<"canceldelay"_n, &native::canceldelay>;
        using setcode_action = eosio::action_wrapper<"setcode"_n, &native::setcode>;
        using setabi_action = eosio::action_wrapper<"setabi"_n, &native::setabi>;
    };
}
