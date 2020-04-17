#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/ignore.hpp>
#include <eosiolib/transaction.hpp>
#include <fio.common/fio.common.hpp>


namespace eosio {

    static const uint64_t PROPOSERAM = 1024;
    static const uint64_t APPROVERAM = 1024;

    class [[eosio::contract("eosio.msig")]] multisig : public contract {
    private:
        eosiosystem::top_producers_table topprods;
    public:
        multisig(name s, name code, datastream<const char *> ds) : contract(s, code, ds),
        topprods(SYSTEMACCOUNT, SYSTEMACCOUNT.value) {
        }
        using contract::contract;

        [[eosio::action]]
        void propose(ignore<name> proposer,
                     ignore<name> proposal_name,
                     ignore<std::vector<permission_level>> requested,
                     ignore<uint64_t> max_fee,
                     ignore<transaction> trx
        );

        [[eosio::action]]
        void approve(name proposer, name proposal_name, permission_level level, const uint64_t &max_fee,
                     const eosio::binary_extension<eosio::checksum256> &proposal_hash);

        [[eosio::action]]
        void unapprove(name proposer, name proposal_name, permission_level level, const uint64_t &max_fee);

        [[eosio::action]]
        void cancel(name proposer, name proposal_name, name canceler, const uint64_t &max_fee);

        [[eosio::action]]
        void exec(name proposer, name proposal_name, const uint64_t &max_fee, name executer);

        [[eosio::action]]
        void invalidate(name account,const uint64_t &max_fee);

        using propose_action = eosio::action_wrapper<"propose"_n, &multisig::propose>;
        using approve_action = eosio::action_wrapper<"approve"_n, &multisig::approve>;
        using unapprove_action = eosio::action_wrapper<"unapprove"_n, &multisig::unapprove>;
        using cancel_action = eosio::action_wrapper<"cancel"_n, &multisig::cancel>;
        using exec_action = eosio::action_wrapper<"exec"_n, &multisig::exec>;
        using invalidate_action = eosio::action_wrapper<"invalidate"_n, &multisig::invalidate>;
    private:
        struct [[eosio::table]] proposal {
            name proposal_name;
            std::vector<char> packed_transaction;

            uint64_t primary_key() const { return proposal_name.value; }
        };

        typedef eosio::multi_index<"proposal"_n, proposal> proposals;

        struct [[eosio::table]] old_approvals_info {
            name proposal_name;
            std::vector <permission_level> requested_approvals;
            std::vector <permission_level> provided_approvals;

            uint64_t primary_key() const { return proposal_name.value; }
        };

        typedef eosio::multi_index<"approvals"_n, old_approvals_info> old_approvals;

        struct approval {
            permission_level level;
            time_point time;
        };

        struct [[eosio::table]] approvals_info {
            uint8_t version = 1;
            name proposal_name;
            //requested approval doesn't need to cointain time, but we want requested approval
            //to be of exact the same size ad provided approval, in this case approve/unapprove
            //doesn't change serialized data size. So, we use the same type.
            std::vector <approval> requested_approvals;
            std::vector <approval> provided_approvals;

            uint64_t primary_key() const { return proposal_name.value; }
        };

        typedef eosio::multi_index<"approvals2"_n, approvals_info> approvals;

        struct [[eosio::table]] invalidation {
            name account;
            time_point last_invalidation_time;

            uint64_t primary_key() const { return account.value; }
        };

        typedef eosio::multi_index<"invals"_n, invalidation> invalidations;
    };

} /// namespace eosio
