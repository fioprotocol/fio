/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <fio.system/fio.system.hpp>

#include <eosiolib/eosio.hpp>
#include <eosiolib/datastream.hpp>
#include <eosiolib/serialize.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/privileged.h>
#include <eosiolib/transaction.hpp>

#include <fio.token/fio.token.hpp>


#include <cmath>
#include <map>

namespace eosiosystem {
    using eosio::asset;
    using eosio::indexed_by;
    using eosio::const_mem_fun;
    using eosio::permission_level;
    using eosio::time_point_sec;
    using std::map;
    using std::pair;

    static constexpr uint32_t refund_delay_sec = 3 * 24 * 3600;
    static constexpr int64_t ram_gift_bytes = 1400;

    struct [[eosio::table, eosio::contract("fio.system")]] user_resources {
        name owner;
        asset net_weight;
        asset cpu_weight;
        int64_t ram_bytes = 0;

        bool is_empty() const { return net_weight.amount == 0 && cpu_weight.amount == 0 && ram_bytes == 0; }

        uint64_t primary_key() const { return owner.value; }

        // explicit serialization macro is not necessary, used here only to improve compilation time
        EOSLIB_SERIALIZE( user_resources, (owner)(net_weight)(cpu_weight)(ram_bytes)
        )
    };


    /**
     *  Every user 'from' has a scope/table that uses every receipient 'to' as the primary key.
     */
    struct [[eosio::table, eosio::contract("fio.system")]] delegated_bandwidth {
        name from;
        name to;
        asset net_weight;
        asset cpu_weight;

        bool is_empty() const { return net_weight.amount == 0 && cpu_weight.amount == 0; }

        uint64_t primary_key() const { return to.value; }

        // explicit serialization macro is not necessary, used here only to improve compilation time
        EOSLIB_SERIALIZE( delegated_bandwidth, (from)(to)(net_weight)(cpu_weight)
        )

    };

    struct [[eosio::table, eosio::contract("fio.system")]] refund_request {
        name owner;
        time_point_sec request_time;
        eosio::asset net_amount;
        eosio::asset cpu_amount;

        bool is_empty() const { return net_amount.amount == 0 && cpu_amount.amount == 0; }

        uint64_t primary_key() const { return owner.value; }

        // explicit serialization macro is not necessary, used here only to improve compilation time
        EOSLIB_SERIALIZE( refund_request, (owner)(request_time)(net_amount)(cpu_amount)
        )
    };

    /**
     *  These tables are designed to be constructed in the scope of the relevant user, this
     *  facilitates simpler API for per-user queries
     */
    typedef eosio::multi_index<"userres"_n, user_resources> user_resources_table;
    typedef eosio::multi_index<"delband"_n, delegated_bandwidth> del_bandwidth_table;
    typedef eosio::multi_index<"refunds"_n, refund_request> refunds_table;






    void validate_b1_vesting(int64_t stake) {
        const int64_t base_time = 1527811200; /// 2018-06-01
        const int64_t max_claimable = 100'000'000'0000ll;
        const int64_t claimable = int64_t(max_claimable * double(now() - base_time) / (10 * seconds_per_year));

        check(max_claimable - claimable <= stake, "b1 can only claim their tokens over 10 years");
    }


    //MAS-522 remove staking from voting.
   // void system_contract::update_voting_power(const name &voter, const asset &total_update) {
   void system_contract::updatepower(const name &voter,bool updateonly) {
        print(" called update power.","\n");
        auto voter_itr = _voters.find(voter.value);
        if ((voter_itr == _voters.end())&& updateonly) {
            print(" could not find voter.",voter,"\n");
            //its not there so return.
            return;
        }
        if ((voter_itr == _voters.end())&& !updateonly) {
            voter_itr = _voters.emplace(voter, [&](auto &v) {
                v.owner = voter;
                //MAS-522 remove stake from voting
               // v.staked = total_update.amount;
            });
        }
        //MAS-522 remove stake from voting
        //else {
           // _voters.modify(voter_itr, same_payer, [&](auto &v) {
           //     v.staked += total_update.amount;
          //  });
       // }

       // check(0 <= voter_itr->staked, "stake for voting cannot be negative");

       //NOTE -- lets leave this hear, we may need similar logic possibly for
       //  FIO launch...
       // if (voter == "b1"_n) {
       //     validate_b1_vesting(voter_itr->staked);
        //}
        //MAS-522 remove stake from voting END

        if (voter_itr->producers.size() || voter_itr->proxy) {
            update_votes(voter, voter_itr->proxy, voter_itr->producers, false);
        }
    }

    void system_contract::refund(const name owner) {
        require_auth(owner);

        refunds_table refunds_tbl(_self, owner.value);
        auto req = refunds_tbl.find(owner.value);
        check(req != refunds_tbl.end(), "refund request not found");
        check(req->request_time + seconds(refund_delay_sec) <= current_time_point(),
              "refund is not available yet");

        INLINE_ACTION_SENDER(eosio::token, transfer)(
                token_account, {{stake_account, active_permission},
                                {req->owner,    active_permission}},
                {stake_account, req->owner, req->net_amount + req->cpu_amount, std::string("unstake")}
        );

        refunds_tbl.erase(req);
    }


} //namespace eosiosystem
