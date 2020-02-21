#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/ignore.hpp>
#include <eosiolib/transaction.hpp>

namespace eosio {

    class [[eosio::contract("eosio.wrap")]] wrap : public contract {
    public:
        using contract::contract;

        [[eosio::action]]
        void execute(ignore <name> executer, ignore <transaction> trx);

        using exec_action = eosio::action_wrapper<"execute"_n, &wrap::execute>;
    };

} /// namespace eosio
