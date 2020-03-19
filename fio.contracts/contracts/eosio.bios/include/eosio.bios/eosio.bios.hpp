#pragma once

#include <eosiolib/action.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/eosio.hpp>
#include <eosiolib/privileged.hpp>
#include <eosiolib/producer_schedule.hpp>
#include "fio.common/fio.accounts.hpp"

namespace eosio {
    using eosio::permission_level;
    using eosio::public_key;
    using eosio::ignore;
    using eosio::name;

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

    class [[eosio::contract("eosio.bios")]] bios : public contract {
    public:
        using contract::contract;

        [[eosio::action]]
        void newaccount(name creator,
                        name name,
                        ignore<authority> owner,
                        ignore<authority> active) {
            check((creator == fioio::SYSTEMACCOUNT || creator == fioio::TokenContract ||
                   creator == fioio::AddressContract), "new account is not permitted");

        }


        [[eosio::action]]
        void updateauth(name account,
                        name permission,
                        name parent,
                        authority auth,
                        const uint64_t &max_fee) {

            require_auth(account);
            eosio::action{
                    permission_level{account, "active"_n},
                    fioio::FeeContract, "mandatoryfee"_n,
                    std::make_tuple(std::string("auth_update"), account, max_fee)
            }.send();


            if (permission == fioio::ACTIVE || permission == fioio::OWNER){
                eosio_assert((auth.keys.size() == 0) || (auth.keys.size() == 1),
                             "update auth not permitted on owner or active unless keys is empty or has a single entry matching the account public key");
                //todo add code to check that if there is a single auth key, the key matches the value in the account map.
            }

        }

        [[eosio::action]]
        void deleteauth(name account,
                        name permission,
                        uint64_t max_fee) {
            require_auth(account);

            eosio::action{
                    permission_level{account, "active"_n},
                    fioio::FeeContract, "mandatoryfee"_n,
                    std::make_tuple(std::string("auth_delete"), account, max_fee)
            }.send();

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

        }

        [[eosio::action]]
        void unlinkauth(name account,
                        ignore<name> code,
                        ignore<name> type) {
            require_auth(account);
            //let them do this as much as they can for free since it decreases state size.
        }

        [[eosio::action]]
        void canceldelay(ignore <permission_level> canceling_auth, ignore <capi_checksum256> trx_id) {
            require_auth(_self);
        }

        [[eosio::action]]
        void setpriv(name account, uint8_t is_priv) {
            require_auth(_self);
            set_privileged(account.value, is_priv);
        }

        [[eosio::action]]
        void setprods(std::vector <eosio::producer_key> schedule) {
            (void) schedule; // schedule argument just forces the deserialization of the action data into vector<producer_key> (necessary check)
            require_auth(_self);

            constexpr size_t max_stack_buffer_size = 512;
            size_t size = action_data_size();
            char *buffer = (char *) (max_stack_buffer_size < size ? malloc(size) : alloca(size));
            read_action_data(buffer, size);
            set_proposed_producers(buffer, size);
            free(buffer);
            buffer = nullptr;
        }

        [[eosio::action]]
        void setcode(name account, uint8_t vmtype, uint8_t vmversion, const std::vector<char> &code) {
            require_auth(_self);
        }

        [[eosio::action]]
        void reqauth( name from );

        [[eosio::action]]
        void setparams(const eosio::blockchain_parameters &params) {
            require_auth(_self);
            set_blockchain_parameters(params);
        }

        [[eosio::action]]
        void setabi(name account, const std::vector<char> &abi) {

            check( (account == fioio::MSIGACCOUNT ||
                    account == fioio::WRAPACCOUNT ||
                    account == fioio::FeeContract ||
                    account == fioio::AddressContract ||
                    account == fioio::TPIDContract ||
                    account == fioio::TokenContract ||
                    account == fioio::REQOBTACCOUNT ||
                    account == fioio::TREASURYACCOUNT ||
                    account == fioio::SYSTEMACCOUNT), "setabi is not permitted");

            abi_hash_table table(_self, _self.value);
            auto itr = table.find(account.value);
            if (itr == table.end()) {
                table.emplace(account, [&](auto &row) {
                    row.owner = account;
                    sha256(const_cast<char *>(abi.data()), abi.size(), &row.hash);
                });
            } else {
                table.modify(itr, same_payer, [&](auto &row) {
                    sha256(const_cast<char *>(abi.data()), abi.size(), &row.hash);
                });
            }
        }

        struct [[eosio::table]] abi_hash {
            name owner;
            capi_checksum256 hash;

            uint64_t primary_key() const { return owner.value; }

            EOSLIB_SERIALIZE( abi_hash, (owner)(hash)
            )
        };

        typedef eosio::multi_index<"abihash"_n, abi_hash> abi_hash_table;

        using newaccount_action = action_wrapper<"newaccount"_n, &bios::newaccount>;
        using updateauth_action = action_wrapper<"updateauth"_n, &bios::updateauth>;
        using deleteauth_action = action_wrapper<"deleteauth"_n, &bios::deleteauth>;
        using linkauth_action = action_wrapper<"linkauth"_n, &bios::linkauth>;
        using unlinkauth_action = action_wrapper<"unlinkauth"_n, &bios::unlinkauth>;
        using setpriv_action = action_wrapper<"setpriv"_n, &bios::setpriv>;
        using setprods_action = action_wrapper<"setprods"_n, &bios::setprods>;
        using setparams_action = action_wrapper<"setparams"_n, &bios::setparams>;
        using setabi_action = action_wrapper<"setabi"_n, &bios::setabi>;
    };

} /// namespace eosio
