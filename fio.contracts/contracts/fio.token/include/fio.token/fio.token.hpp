/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <string>

#include <fio.fee/fio.fee.hpp>
#include <fio.common/fio.common.hpp>
#include <fio.name/fio.name.hpp>

namespace eosiosystem {
    class system_contract;
}

namespace eosio {

    using std::string;

    class [[eosio::contract("fio.token")]] token : public contract {
    private:
        fioio::eosio_names_table eosionames;
        fioio::fiofee_table fiofees;
        fioio::config appConfig;
    public:
        token(account_name _self) : contract(_self),
                                    eosionames("fio.system"_n, "fio.system"_n),
                                    fiofees(fioio::FeeContract, fioio::FeeContract) {
            fioio::configs_singleton configsSingleton(fioio::FeeContract, fioio::FeeContract);
            appConfig = configsSingleton.get_or_default(fioio::config());
        }

        using contract::contract;

        [[eosio::action]]
        void create(name issuer,
                    asset maximum_supply);

        [[eosio::action]]
        void issue(name to, asset quantity, string memo);

        inline void fio_fees(const account_name &actor, const asset &fee);

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
                          name actor);

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
            account_name from;
            account_name to;
            asset quantity;
            string memo;
        };

        struct bind2eosio {
            account_name accountName;
            string public_key;
            bool existing;
        };
    };

    [[eosio::action]]
    asset token::get_supply(symbol_name sym) const {
        stats statstable(_self, sym);
        const auto &st = statstable.get(sym);
        return st.supply;
    }

    [[eosio::action]]
    asset token::get_balance(account_name owner, symbol_name sym) const {
        accounts accountstable(_self, owner);
        const auto &ac = accountstable.get(sym);
        return ac.balance;
    }
} /// namespace eosio
