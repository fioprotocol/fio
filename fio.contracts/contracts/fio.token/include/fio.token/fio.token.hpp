/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <fio.common/fio.common.hpp>
#include <fio.name/fio.name.hpp>
#include <fio.fee/fio.fee.hpp>
#include <fio.tpid/fio.tpid.hpp>

namespace eosiosystem {
    class system_contract;
}

namespace eosio {
using namespace fioio;

    using std::string;

    class [[eosio::contract("fio.token")]] token : public contract {
    private:
        fioio::eosio_names_table eosionames;
        fioio::fiofee_table fiofees;
        fioio::config appConfig;
        fioio::tpids_table tpids;
    public:
        token(name s, name code, datastream<const char *> ds) : contract(s, code, ds),
                                                                eosionames(fioio::SystemContract,fioio::SystemContract.value),
                                                                fiofees(fioio::FeeContract, fioio::FeeContract.value),
                                                                tpids(fioio::SystemContract, fioio::SystemContract.value) {
            fioio::configs_singleton configsSingleton(fioio::FeeContract, fioio::FeeContract.value);
            appConfig = configsSingleton.get_or_default(fioio::config());
        }

        [[eosio::action]]
        void create(name issuer,
                    asset maximum_supply);

        [[eosio::action]]
        void issue(name to, asset quantity, string memo);

        inline void fio_fees(const name &actor, const asset &fee);

        inline bool check_tpid(const string &tpid) {
          auto iter = tpids.find(string_to_uint64_hash(tpid.c_str()));
          return iter == tpids.end();
        }

        [[eosio::action]]
        void retire(asset quantity, string memo);

        [[eosio::action]]
        void transfer(name from,
                      name to,
                      asset quantity,
                      string memo);

        [[eosio::action]]
        void trnsfiopubky(string payee_public_key,
                          string amount,
                          uint64_t max_fee,
                          name actor,
                          const string &tpid);

        [[eosio::action]]
        void open(name owner, const symbol &symbol, name ram_payer);

        [[eosio::action]]
        void close(name owner, const symbol &symbol);

        static asset get_supply(name token_contract_account, symbol_code sym_code) {
            stats statstable(token_contract_account, sym_code.raw());
            const auto &st = statstable.get(sym_code.raw());
            return st.supply;
        }

        static asset get_balance(name token_contract_account, name owner, symbol_code sym_code) {
            accounts accountstable(token_contract_account, owner.value);
            const auto &ac = accountstable.get(sym_code.raw());
            return ac.balance;
        }

        using create_action = eosio::action_wrapper<"create"_n, &token::create>;
        using issue_action = eosio::action_wrapper<"issue"_n, &token::issue>;
        using retire_action = eosio::action_wrapper<"retire"_n, &token::retire>;
        using transfer_action = eosio::action_wrapper<"transfer"_n, &token::transfer>;
        using open_action = eosio::action_wrapper<"open"_n, &token::open>;
        using close_action = eosio::action_wrapper<"close"_n, &token::close>;
    private:
        struct [[eosio::table]] account {
            asset balance;

            uint64_t primary_key() const { return balance.symbol.code().raw(); }
        };

        struct [[eosio::table]] currency_stats {
            asset supply;
            asset max_supply;
            name issuer;

            uint64_t primary_key() const { return supply.symbol.code().raw(); }
        };

        typedef eosio::multi_index<"accounts"_n, account> accounts;
        typedef eosio::multi_index<"stat"_n, currency_stats> stats;

        void sub_balance(name owner, asset value);
        void add_balance(name owner, asset value, name ram_payer);

    public:

        struct transfer_args {
            name from;
            name to;
            asset quantity;
            string memo;
        };

        struct bind2eosio {
            name accountName;
            string public_key;
            bool existing;
        };
    };
} /// namespace eosio
