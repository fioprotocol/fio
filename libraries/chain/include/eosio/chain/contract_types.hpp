#pragma once

#include <eosio/chain/authority.hpp>
#include <eosio/chain/chain_config.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/types.hpp>

namespace eosio {
    namespace chain {

        using action_name    = eosio::chain::action_name;

        struct newaccount {
            account_name creator;
            account_name name;
            authority owner;
            authority active;

            static account_name get_account() {
                return config::system_account_name;
            }

            static action_name get_name() {
                return N(newaccount);
            }
        };

        struct addaction {
            name action;
            string contract;
            name actor;

            static account_name get_account() {
                return config::system_account_name;
            }

            static action_name get_name() {
                return N(addaction);
            }
        };

        struct remaction {
            name action;
            name actor;

            static account_name get_account() {
                return config::system_account_name;
            }

            static action_name get_name() {
                return N(remaction);
            }
        };

        struct trnsfiopubky {
            string payee_public_key;
            int64_t amount;
            int64_t max_fee;
            name actor;
            string tpid;

            static account_name get_account() {
                return N(fio.token);
            }

            static action_name get_name() {
                return N(trnsfiopubky);
            }
        };

        struct newfioacc {
            string fio_public_key;
            authority owner;
            authority active;
            int64_t max_fee;
            name actor;
            string tpid;

            static account_name get_account() {
                return N(eosio);
            }

            static action_name get_name() {
                return N(newfioacc);
            }
        };

        struct lockperiodv2 {
            int64_t duration = 0; //duration in seconds. each duration is seconds after grant creation.
            int64_t amount; //this is the percent to be unlocked
        };


        struct trnsloctoks {
            string payee_public_key;
            int32_t can_vote;
            vector<lockperiodv2> periods;
            int64_t amount;
            int64_t max_fee;
            name actor;
            string tpid;

            static account_name get_account() {
                return N(fio.token);
            }

            static action_name get_name() {
                return N(trnsloctoks);
            }
        };

        struct regaddress {
            string fio_address;
            string owner_fio_public_key;
            int64_t max_fee;
            name actor;
            string tpid;

            static account_name get_account() {
                return N(fio.address);
            }

            static action_name get_name() {
                return N(regaddress);
            }
        };

        struct regdomain {
          string fio_domain;
          string owner_fio_public_key;
          int64_t max_fee;
          name actor;
          string tpid;

            static account_name get_account() {
                return N(fio.address);
            }

            static action_name get_name() {
                return N(regdomain);
            }
        };


        struct regdomadd {
          string fio_address;
          int8_t is_public;
          string owner_fio_public_key;
          int64_t max_fee;
          string tpid;
          name actor;

            static account_name get_account() {
                return N(fio.address);
            }

            static action_name get_name() {
                return N(regdomadd);
            }
        };

        struct burnaddress {
            string fio_address;
            int64_t max_fee;
            name actor;
            string tpid;

            static account_name get_account() {
                return N(fio.address);
            }

            static action_name get_name() {
                return N(burnaddress);
            }
        };

        struct xferdomain {
          string fio_domain;
          string new_owner_fio_public_key;
          int64_t max_fee;
          name actor;
          string tpid;

            static account_name get_account() {
                return N(fio.address);
            }

            static action_name get_name() {
                return N(xferdomain);
            }
        };

        struct xferescrow {
          string fio_domain;
          string public_key;
          bool isEscrow;
          name actor;

            static account_name get_account() {
                return N(fio.address);
            }

            static action_name get_name() {
                return N(xferescrow);
            }
        };

        struct xferaddress {
          string fio_address;
          string new_owner_fio_public_key;
          int64_t max_fee;
          name actor;
          string tpid;

            static account_name get_account() {
                return N(fio.address);
            }

            static action_name get_name() {
                return N(xferaddress);
            }
        };

        struct setcode {
            account_name account;
            uint8_t vmtype = 0;
            uint8_t vmversion = 0;
            bytes code;

            static account_name get_account() {
                return config::system_account_name;
            }

            static action_name get_name() {
                return N(setcode);
            }
        };

        struct setabi {
            account_name account;
            bytes abi;

            static account_name get_account() {
                return config::system_account_name;
            }

            static action_name get_name() {
                return N(setabi);
            }
        };


        struct updateauth {
            account_name account;
            permission_name permission;
            permission_name parent;
            authority auth;
            uint64_t max_fee;

            static account_name get_account() {
                return config::system_account_name;
            }

            static action_name get_name() {
                return N(updateauth);
            }
        };

        struct deleteauth {
            deleteauth() = default;

            deleteauth(const account_name &account, const permission_name &permission, const uint64_t &max_fee)
                    : account(account), permission(permission), max_fee(max_fee) {}

            account_name account;
            permission_name permission;
            uint64_t max_fee;

            static account_name get_account() {
                return config::system_account_name;
            }

            static action_name get_name() {
                return N(deleteauth);
            }
        };

        struct linkauth {
            linkauth() = default;

            linkauth(const account_name &account, const account_name &code, const action_name &type,
                     const permission_name &requirement, const uint64_t &max_fee)
                    : account(account), code(code), type(type), requirement(requirement), max_fee(max_fee) {}

            account_name account;
            account_name code;
            action_name type;
            permission_name requirement;
            uint64_t max_fee;

            static account_name get_account() {
                return config::system_account_name;
            }

            static action_name get_name() {
                return N(linkauth);
            }
        };

        struct unlinkauth {
            unlinkauth() = default;

            unlinkauth(const account_name &account, const account_name &code, const action_name &type)
                    : account(account), code(code), type(type) {}

            account_name account;
            account_name code;
            action_name type;

            static account_name get_account() {
                return config::system_account_name;
            }

            static action_name get_name() {
                return N(unlinkauth);
            }
        };

        struct canceldelay {
            permission_level canceling_auth;
            transaction_id_type trx_id;

            static account_name get_account() {
                return config::system_account_name;
            }

            static action_name get_name() {
                return N(canceldelay);
            }
        };

        struct onerror {
            uint128_t sender_id;
            bytes sent_trx;

            onerror(uint128_t sid, const char *data, size_t len)
                    : sender_id(sid), sent_trx(data, data + len) {}

            static account_name get_account() {
                return config::system_account_name;
            }

            static action_name get_name() {
                return N(onerror);
            }
        };

    }
} /// namespace eosio::chain

FC_REFLECT(eosio::chain::newaccount, (creator)(name)(owner)(active))
FC_REFLECT(eosio::chain::addaction, (action)(contract)(actor))
FC_REFLECT(eosio::chain::remaction, (action)(actor))
FC_REFLECT(eosio::chain::setcode, (account)(vmtype)(vmversion)(code))
FC_REFLECT(eosio::chain::setabi, (account)(abi))
FC_REFLECT(eosio::chain::updateauth, (account)(permission)(parent)(auth)(max_fee))
FC_REFLECT(eosio::chain::deleteauth, (account)(permission)(max_fee))
FC_REFLECT(eosio::chain::linkauth, (account)(code)(type)(requirement)(max_fee))
FC_REFLECT(eosio::chain::unlinkauth, (account)(code)(type))
FC_REFLECT(eosio::chain::canceldelay, (canceling_auth)(trx_id))
FC_REFLECT(eosio::chain::onerror, (sender_id)(sent_trx))
FC_REFLECT(eosio::chain::trnsfiopubky, (payee_public_key)(amount)(max_fee)(actor)(tpid))
FC_REFLECT(eosio::chain::newfioacc, (fio_public_key)(owner)(active)(max_fee)(actor)(tpid))
FC_REFLECT(eosio::chain::regaddress, (fio_address)(owner_fio_public_key)(max_fee)(actor)(tpid))
FC_REFLECT(eosio::chain::regdomain, (fio_domain)(owner_fio_public_key)(max_fee)(actor)(tpid))
FC_REFLECT(eosio::chain::regdomadd, (fio_address)(is_public)(owner_fio_public_key)(max_fee)(tpid)(actor))
FC_REFLECT(eosio::chain::burnaddress, (fio_address)(max_fee)(actor)(tpid))
FC_REFLECT(eosio::chain::xferdomain, (fio_domain)(new_owner_fio_public_key)(max_fee)(actor)(tpid))
FC_REFLECT(eosio::chain::xferescrow, (fio_domain)(public_key)(isEscrow)(actor))
FC_REFLECT(eosio::chain::xferaddress, (fio_address)(new_owner_fio_public_key)(max_fee)(actor)(tpid))
FC_REFLECT(eosio::chain::lockperiodv2, (duration)(amount))
FC_REFLECT(eosio::chain::trnsloctoks, (payee_public_key)(can_vote)(periods)(amount)(max_fee)(actor)(tpid))
