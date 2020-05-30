/** Fio system contract file
 *  Description: fio.system contains global state, producer, voting and other system level information along with
 *  the operations on these.
 *  @author Adam Androulidakis, Casey Gardiner, Ed Rotthoff
 *  @file fio.system.hpp
 *  @license FIO Foundation ( https://github.com/fioprotocol/fio/blob/master/LICENSE ) Dapix
 *
 *  Changes:
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/privileged.hpp>
#include <eosiolib/singleton.hpp>
#include <fio.address/fio.address.hpp>
#include <eosiolib/transaction.hpp>
#include <fio.fee/fio.fee.hpp>
#include "native.hpp"

#include <string>
#include <deque>
#include <type_traits>
#include <optional>

namespace eosiosystem {

    using eosio::name;
    using eosio::asset;
    using eosio::symbol;
    using eosio::symbol_code;
    using eosio::indexed_by;
    using eosio::const_mem_fun;
    using eosio::block_timestamp;
    using eosio::time_point;
    using eosio::time_point_sec;
    using eosio::microseconds;
    using eosio::datastream;
    using eosio::check;

    template<typename E, typename F>
    static inline auto has_field(F flags, E field)
    -> std::enable_if_t<std::is_integral_v < F> &&

    std::is_unsigned_v <F> &&
            std::is_enum_v<E>
    && std::is_same_v <F, std::underlying_type_t<E>>, bool> {
    return ((flags & static_cast
    <F>(field)
    ) != 0 );
}

template<typename E, typename F>
static inline auto set_field(F flags, E field, bool value = true)
-> std::enable_if_t<std::is_integral_v < F> &&

std::is_unsigned_v <F> &&
        std::is_enum_v<E>
&& std::is_same_v <F, std::underlying_type_t<E>>, F >
{
if( value )
return ( flags | static_cast
<F>(field)
);
else
return (
flags &~
static_cast
<F>(field)
);
}

struct [[eosio::table("global"), eosio::contract("fio.system")]] eosio_global_state : eosio::blockchain_parameters {
    block_timestamp last_producer_schedule_update;
    time_point last_pervote_bucket_fill;
    int64_t pervote_bucket = 0;
    int64_t perblock_bucket = 0;
    uint32_t total_unpaid_blocks = 0; /// all blocks which have been produced but not paid
    int64_t total_voted_fio = 0;
    time_point thresh_voted_fio_time;
    uint16_t last_producer_schedule_size = 0;
    double total_producer_vote_weight = 0; /// the sum of all producer votes
    block_timestamp last_name_close;
    block_timestamp last_fee_update;

    // explicit serialization macro is not necessary, used here only to improve compilation time
    EOSLIB_SERIALIZE_DERIVED( eosio_global_state, eosio::blockchain_parameters,
            (last_producer_schedule_update)(last_pervote_bucket_fill)
            (pervote_bucket)(perblock_bucket)(total_unpaid_blocks)(total_voted_fio)(thresh_voted_fio_time)
            (last_producer_schedule_size)(total_producer_vote_weight)(last_name_close)(last_fee_update)
    )
};

/**
 * Defines new global state parameters added after version 1.0
 */
struct [[eosio::table("global2"), eosio::contract("fio.system")]] eosio_global_state2 {
    eosio_global_state2() {}
    block_timestamp last_block_num; /* deprecated */
    double total_producer_votepay_share = 0;
    uint8_t revision = 0; ///< used to track version updates in the future.

    EOSLIB_SERIALIZE( eosio_global_state2,(last_block_num)
            (total_producer_votepay_share)(revision)
    )
};

struct [[eosio::table("global3"), eosio::contract("fio.system")]] eosio_global_state3 {
    eosio_global_state3() {}

    time_point last_vpay_state_update;
    double total_vpay_share_change_rate = 0;

    EOSLIB_SERIALIZE( eosio_global_state3, (last_vpay_state_update)(total_vpay_share_change_rate)
    )
};


//these locks are used for investors and emplyees and members who have grants upon integration.
//this table holds the list of FIO accounts that hold locked FIO tokens
struct [[eosio::table, eosio::contract("fio.system")]] locked_token_holder_info {
    name owner;
    uint64_t total_grant_amount = 0;
    uint32_t unlocked_period_count = 0; //this indicates time periods processed and unlocked thus far.
    uint32_t grant_type = 0;   //1,2 see FIO spec for details
    uint32_t inhibit_unlocking = 0;
    uint64_t remaining_locked_amount = 0;
    uint32_t timestamp = 0;


    uint64_t primary_key() const { return owner.value; }

    // explicit serialization macro is not necessary, used here only to improve compilation time
    EOSLIB_SERIALIZE( locked_token_holder_info, (owner)(total_grant_amount)
    (unlocked_period_count)(grant_type)(inhibit_unlocking)(remaining_locked_amount)(timestamp)
    )
};

typedef eosio::multi_index<"lockedtokens"_n, locked_token_holder_info>
locked_tokens_table;

//begin general locks, these locks are used to hold tokens granted by any fio user
//to any other fio user.
struct glockresult {
    bool lockfound = false; //did we find a general lock.
    uint64_t amount; //amount votable
    EOSLIB_SERIALIZE( glockresult, (lockfound)(amount))
};
struct lockperiods {
    int64_t duration = 0; //duration in seconds. each duration is seconds after grant creation.
    double percent; //this is the percent to be unlocked
    EOSLIB_SERIALIZE( lockperiods, (duration)(percent))
};

struct [[eosio::table, eosio::contract("fio.system")]] locked_tokens_info {
    int64_t id; //this is the identifier of the lock, primary key
    name owner_account; //this is the account that owns the lock, secondary key
    int64_t lock_amount = 0; //this is the amount of the lock in FIO SUF
    int32_t payouts_performed = 0; //this is the number of payouts performed thus far.
    int32_t can_vote = 0; //this is the flag indicating if the lock is votable/proxy-able
    std::vector<lockperiods> periods;// this is the locking periods for the lock
    int64_t remaining_lock_amount = 0; //this is the amount remaining in the lock in FIO SUF, get decremented as unlocking occurs.
    uint32_t timestamp = 0; //this is the time of creation of the lock, locking periods are relative to this time.

    uint64_t primary_key() const { return id; }
    uint64_t by_owner() const{return owner_account.value;}

    EOSLIB_SERIALIZE( locked_tokens_info, (id)(owner_account)
            (lock_amount)(payouts_performed)(can_vote)(periods)(remaining_lock_amount)(timestamp)
    )

};

typedef eosio::multi_index<"locktokens"_n, locked_tokens_info,
    indexed_by<"byowner"_n, const_mem_fun < locked_tokens_info, uint64_t, &locked_tokens_info::by_owner> >

>
general_locks_table;
//end general locks

//Top producers that are calculated every block in update_elected_producers
struct [[eosio::table, eosio::contract("fio.system")]] top_prod_info {
    name producer;

    uint64_t primary_key() const { return producer.value; }

    // explicit serialization macro is not necessary, used here only to improve compilation time
    EOSLIB_SERIALIZE( top_prod_info, (producer)
    )
};

typedef eosio::multi_index<"topprods"_n, top_prod_info>
top_producers_table;



struct [[eosio::table, eosio::contract("fio.system")]] producer_info {
    uint64_t id;
    name owner;
    string fio_address;  //this is the fio address to be used for bundled fee collection
                         //for tx that are fee type bundled, just use the one fio address
                         //you want to have pay for the bundled fee transactions associated
                         //with this producer.
    uint128_t addresshash;

    double total_votes = 0;
    eosio::public_key producer_public_key; /// a packed public key object
    bool is_active = true;
    std::string url;
    uint32_t unpaid_blocks = 0;
    time_point last_claim_time; //this is the last time a payout was given to this producer.
    uint32_t last_bpclaim;  //this is the last time bpclaim was called for this producer.
    //init this to zero here to ensure that if the location is not specified, sorting will still work.
    uint16_t location = 0;
    uint64_t primary_key() const { return id; }
    uint64_t by_owner() const{return owner.value;}
    uint128_t by_address() const{return addresshash;}

    double by_votes() const { return is_active ? -total_votes : total_votes; }

    bool active() const { return is_active; }

    void deactivate() {
        producer_public_key = public_key();
        is_active = false;
    }

    // explicit serialization macro is not necessary, used here only to improve compilation time
    EOSLIB_SERIALIZE( producer_info, (id)(owner)(fio_address)(addresshash)(total_votes)(producer_public_key)(is_active)(url)
            (unpaid_blocks)(last_claim_time)(last_bpclaim)(location)
    )
};

typedef eosio::multi_index<"producers"_n, producer_info,
        indexed_by<"prototalvote"_n, const_mem_fun < producer_info, double, &producer_info::by_votes> >,
        indexed_by<"byaddress"_n, const_mem_fun < producer_info, uint128_t, &producer_info::by_address> >,
        indexed_by<"byowner"_n, const_mem_fun < producer_info, uint64_t, &producer_info::by_owner> >

>
producers_table;


struct [[eosio::table, eosio::contract("fio.system")]] voter_info {
    uint64_t id; //one up id is primary key.
    string fioaddress;  //this is the fio address to be used for bundled fee collection
    //for tx that are fee type bundled, just use the one fio address
    //you want to have pay for the bundled fee transactions associated
    //with this voter.
    uint128_t addresshash; //this is the hash of the fio address for searching
    name owner;     /// the voter
    name proxy;     /// the proxy set by the voter, if any
    std::vector <name> producers; /// the producers approved by this voter if no proxy set
    /**
     *  Every time a vote is cast we must first "undo" the last vote weight, before casting the
     *  new vote weight.  Vote weight is calculated as:
     *
     *  stated.amount * 2 ^ ( weeks_since_launch/weeks_per_year)
     */
    double last_vote_weight = 0; /// the vote weight cast the last time the vote was updated

    /**
     * Total vote weight delegated to this voter.
     */
    double proxied_vote_weight = 0; /// the total vote weight delegated to this voter as a proxy
    bool is_proxy = 0; /// whether the voter is a proxy for others
    bool is_auto_proxy = 0;
    uint32_t reserved2 = 0;
    eosio::asset reserved3;

    uint64_t primary_key() const{return id;}
    uint128_t by_address() const {return addresshash;}
    uint64_t by_owner() const { return owner.value; }

    // explicit serialization macro is not necessary, used here only to improve compilation time
    EOSLIB_SERIALIZE( voter_info, (id)(fioaddress)(addresshash)(owner)(proxy)(producers)(last_vote_weight)(proxied_vote_weight)(is_proxy)(is_auto_proxy)(reserved2)(reserved3)
    )
};

typedef eosio::multi_index<"voters"_n, voter_info,
indexed_by<"byaddress"_n, const_mem_fun<voter_info, uint128_t, &voter_info::by_address>>,
indexed_by<"byowner"_n, const_mem_fun<voter_info, uint64_t, &voter_info::by_owner>>
> voters_table;



//MAS-522 eliminate producers2 table typedef eosio::multi_index<"producers2"_n, producer_info2> producers_table2;

typedef eosio::singleton<"global"_n, eosio_global_state> global_state_singleton;
typedef eosio::singleton<"global2"_n, eosio_global_state2> global_state2_singleton;
typedef eosio::singleton<"global3"_n, eosio_global_state3> global_state3_singleton;

static constexpr uint32_t seconds_per_day = 24 * 3600;



class [[eosio::contract("fio.system")]] system_contract : public native {

private:
    voters_table _voters;
    producers_table _producers;
    top_producers_table _topprods;
    locked_tokens_table _lockedtokens;
    general_locks_table _generallockedtokens;
   //MAS-522 eliminate producers2 producers_table2 _producers2;
    global_state_singleton _global;
    global_state2_singleton _global2;
    global_state3_singleton _global3;
    eosio_global_state _gstate;
    eosio_global_state2 _gstate2;
    eosio_global_state3 _gstate3;
    fioio::fionames_table _fionames;
    fioio::domains_table _domains;
    fioio::fiofee_table _fiofees;
    fioio::eosio_names_table _accountmap;

public:
    static constexpr eosio::name active_permission{"active"_n};
    static constexpr eosio::name token_account{"fio.token"_n};
    static constexpr eosio::name ram_account{"eosio.ram"_n};
    static constexpr eosio::name ramfee_account{"eosio.ramfee"_n};
    static constexpr eosio::name stake_account{"eosio.stake"_n};
    static constexpr eosio::name bpay_account{"eosio.bpay"_n};
    static constexpr eosio::name vpay_account{"eosio.vpay"_n};
    static constexpr eosio::name names_account{"eosio.names"_n};
    static constexpr eosio::name saving_account{"eosio.saving"_n};
    static constexpr eosio::name null_account{"eosio.null"_n};
    static constexpr symbol ramcore_symbol = symbol(symbol_code("FIO"),9);

    system_contract(name s, name code, datastream<const char *> ds);

    ~system_contract();

    // Actions:
    [[eosio::action]]
    void init(const unsigned_int  &version, const symbol &core);

    //this action inits the locked token holder table.
    [[eosio::action]]
    void addlocked(const name &owner, const int64_t &amount,
                    const int16_t &locktype);

    [[eosio::action]]
    void addgenlocked(const name &owner, const vector<lockperiods> &periods, const bool &canvote,const int64_t &amount);

    [[eosio::action]]
    void onblock(ignore <block_header> header);

    // functions defined in delegate_bandwidth.cp

    // functions defined in voting.cpp
    [[eosio::action]]
    void burnaction(const uint128_t &fioaddrhash);

    [[eosio::action]]
    void incram(const name &accountmn, const int64_t &amount);

    [[eosio::action]]
    void updlocked(const name &owner,const uint64_t &amountremaining);

    [[eosio::action]]
    void inhibitunlck(const name &owner,const uint32_t &value);

    [[eosio::action]]
    void unlocktokens(const name &actor);

    void regiproducer(const name &producer, const string &producer_key, const std::string &url, const uint16_t &location,
                      const string &fio_address);

    [[eosio::action]]
    void regproducer(const string &fio_address, const string &fio_pub_key, const std::string &url, const uint16_t &location, const name &actor,
                     const int64_t &max_fee);

    [[eosio::action]]
    void unregprod(const string &fio_address, const name &actor, const int64_t &max_fee);
/*
    [[eosio::action]]
    void vproducer(const name &voter, const name &proxy, const std::vector<name> &producers); //server call
    */

    [[eosio::action]]
    void voteproducer(const std::vector<string> &producers, const string &fio_address, const name &actor, const int64_t &max_fee);

    [[eosio::action]]
    void updatepower(const name &voter, bool updateonly);

    [[eosio::action]]
    void voteproxy(const string &proxy, const string &fio_address, const name &actor, const int64_t &max_fee);

    [[eosio::action]]
    void setautoproxy(const name &proxy,const name &owner);

    [[eosio::action]]
    void crautoproxy(const name &proxy,const name &owner);

    [[eosio::action]]
    void unregproxy(const std::string &fio_address, const name &actor, const int64_t &max_fee);

    [[eosio::action]]
    void regproxy(const std::string &fio_address, const name &actor, const int64_t &max_fee);

    void regiproxy(const name &proxy, const string &fio_address, const bool &isproxy);

    [[eosio::action]]
    void setparams(const eosio::blockchain_parameters &params);

    // functions defined in producer_pay.cpp
    [[eosio::action]]
    void resetclaim(const name &producer);

    //update last bpclaim time
    [[eosio::action]]
    void updlbpclaim(const name &producer);

    [[eosio::action]]
    void setpriv(const name &account,const uint8_t &is_priv);

    [[eosio::action]]
    void rmvproducer(const name &producer);

    [[eosio::action]]
    void updtrevision(const uint8_t &revision);

    using init_action = eosio::action_wrapper<"init"_n, &system_contract::init>;
    using regproducer_action = eosio::action_wrapper<"regproducer"_n, &system_contract::regproducer>;
    using regiproducer_action = eosio::action_wrapper<"regiproducer"_n, &system_contract::regiproducer>;
    using unregprod_action = eosio::action_wrapper<"unregprod"_n, &system_contract::unregprod>;
   // using vproducer_action = eosio::action_wrapper<"vproducer"_n, &system_contract::vproducer>;
    using voteproducer_action = eosio::action_wrapper<"voteproducer"_n, &system_contract::voteproducer>;
    using voteproxy_action = eosio::action_wrapper<"voteproxy"_n, &system_contract::voteproxy>;
    using regproxy_action = eosio::action_wrapper<"regproxy"_n, &system_contract::regproxy>;
    using regiproxy_action = eosio::action_wrapper<"regiproxy"_n, &system_contract::regiproxy>;
    using crautoproxy_action = eosio::action_wrapper<"crautoproxy"_n, &system_contract::crautoproxy>;
    using rmvproducer_action = eosio::action_wrapper<"rmvproducer"_n, &system_contract::rmvproducer>;
    using updtrevision_action = eosio::action_wrapper<"updtrevision"_n, &system_contract::updtrevision>;
    using setpriv_action = eosio::action_wrapper<"setpriv"_n, &system_contract::setpriv>;
    using setparams_action = eosio::action_wrapper<"setparams"_n, &system_contract::setparams>;

private:

    // Implementation details:

    //defined in fio.system.cpp
    static eosio_global_state get_default_parameters();

    static time_point current_time_point();

    static time_point_sec current_time_point_sec();

    static block_timestamp current_block_time();

    // defined in delegate_bandwidth.cpp
    void changebw(name from, name receiver,
                  asset stake_net_quantity, asset stake_cpu_quantity, bool transfer);

    //void update_voting_power(const name &voter, const asset &total_update);

    // defined in voting.hpp
    void update_elected_producers(const block_timestamp& timestamp);

    uint64_t get_votable_balance(const name &tokenowner);

    glockresult get_general_votable_balance(const name &tokenowner);

    void unlock_tokens(const name &actor);


    void update_votes(const name &voter, const name &proxy, const std::vector <name> &producers, const bool &voting);

    void propagate_weight_change(const voter_info &voter);

    double update_total_votepay_share(time_point ct,
                                      double additional_shares_delta = 0.0, double shares_rate_delta = 0.0);

    template<auto system_contract::*...Ptrs>
    class registration {
    public:
        template<auto system_contract::*P, auto system_contract::*...Ps>
        struct for_each {
            template<typename... Args>
            static constexpr void call(system_contract *this_contract, Args &&... args) {
                std::invoke(P, this_contract, std::forward<Args>(args)...);
                for_each<Ps...>::call(this_contract, std::forward<Args>(args)...);
            }
        };

        template<auto system_contract::*P>
        struct for_each<P> {
            template<typename... Args>
            static constexpr void call(system_contract *this_contract, Args &&... args) {
                std::invoke(P, this_contract, std::forward<Args>(args)...);
            }
        };

        template<typename... Args>
        constexpr void operator()(Args &&... args) {
            for_each<Ptrs...>::call(this_contract, std::forward<Args>(args)...);
        }

        system_contract *this_contract;
    };
};

} /// eosiosystem
