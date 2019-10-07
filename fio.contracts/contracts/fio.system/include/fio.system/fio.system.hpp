/** Fio system contract file
 *  Description: fio.system contains global state, producer, voting and other system level information along with
 *  the operations on these.
 *  @author Adam Androulidakis, Casey Gardiner, Ed Rotthoff
 *  @file fio.system.hpp
 *  @copyright Dapix
 *
 *  Changes:
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/privileged.hpp>
#include <eosiolib/singleton.hpp>
#include <fio.name/fio.name.hpp>
#include <fio.fee/fio.fee.hpp>
#include "native.hpp"

#include <string>
#include <deque>
#include <type_traits>
#include <optional>

#ifdef CHANNEL_RAM_AND_NAMEBID_FEES_TO_REX
#undef CHANNEL_RAM_AND_NAMEBID_FEES_TO_REX
#endif
// CHANNEL_RAM_AND_NAMEBID_FEES_TO_REX macro determines whether ramfee and namebid proceeds are
// channeled to REX pool. In order to stop these proceeds from being channeled, the macro must
// be set to 0.
#define CHANNEL_RAM_AND_NAMEBID_FEES_TO_REX 1

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
    uint64_t free_ram() const { return max_ram_size - total_ram_bytes_reserved; }

    uint64_t max_ram_size = 64ll * 1024 * 1024 * 1024;
    uint64_t total_ram_bytes_reserved = 0;
    int64_t total_ram_stake = 0;

    block_timestamp last_producer_schedule_update;
    time_point last_pervote_bucket_fill;
    int64_t pervote_bucket = 0;
    int64_t perblock_bucket = 0;
    uint32_t total_unpaid_blocks = 0; /// all blocks which have been produced but not paid
    int64_t total_activated_stake = 0;
    time_point thresh_activated_stake_time;
    uint16_t last_producer_schedule_size = 0;
    double total_producer_vote_weight = 0; /// the sum of all producer votes
    block_timestamp last_name_close;

    // explicit serialization macro is not necessary, used here only to improve compilation time
    EOSLIB_SERIALIZE_DERIVED( eosio_global_state, eosio::blockchain_parameters,
    (max_ram_size)(total_ram_bytes_reserved)(total_ram_stake)
            (last_producer_schedule_update)(last_pervote_bucket_fill)
            (pervote_bucket)(perblock_bucket)(total_unpaid_blocks)(total_activated_stake)(thresh_activated_stake_time)
            (last_producer_schedule_size)(total_producer_vote_weight)(last_name_close)
    )
};

/**
 * Defines new global state parameters added after version 1.0
 */
struct [[eosio::table("global2"), eosio::contract("fio.system")]] eosio_global_state2 {
    eosio_global_state2() {}

    uint16_t new_ram_per_block = 0;
    block_timestamp last_ram_increase;
    block_timestamp last_block_num; /* deprecated */
    double total_producer_votepay_share = 0;
    uint8_t revision = 0; ///< used to track version updates in the future.

    EOSLIB_SERIALIZE( eosio_global_state2, (new_ram_per_block)(last_ram_increase)(last_block_num)
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

struct [[eosio::table, eosio::contract("fio.system")]] producer_info {
    name owner;
    string fio_address;
    double total_votes = 0;
    eosio::public_key producer_fio_public_key; /// a packed public key object
    bool is_active = true;
    std::string url;
    uint32_t unpaid_blocks = 0;
    time_point last_claim_time;
    //init this to zero here to ensure that if the location is not specified, sorting will still work.
    uint16_t location = 0;

    uint64_t primary_key() const { return owner.value; }

    double by_votes() const { return is_active ? -total_votes : total_votes; }

    bool active() const { return is_active; }

    void deactivate() {
        producer_fio_public_key = public_key();
        is_active = false;
    }

    // explicit serialization macro is not necessary, used here only to improve compilation time
    EOSLIB_SERIALIZE( producer_info, (owner)(fio_address)(total_votes)(producer_fio_public_key)(is_active)(url)
            (unpaid_blocks)(last_claim_time)(location)
    )
};


struct [[eosio::table, eosio::contract("fio.system")]] voter_info {
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

    uint64_t primary_key() const { return owner.value; }

    // explicit serialization macro is not necessary, used here only to improve compilation time
    EOSLIB_SERIALIZE( voter_info, (owner)(proxy)(producers)(last_vote_weight)(proxied_vote_weight)(is_proxy)(is_auto_proxy)(reserved2)(reserved3)
    )
};

typedef eosio::multi_index<"voters"_n, voter_info> voters_table;


typedef eosio::multi_index<"producers"_n, producer_info,
        indexed_by<"prototalvote"_n, const_mem_fun < producer_info, double, &producer_info::by_votes> >
>
producers_table;
//MAS-522 eliminate producers2 table typedef eosio::multi_index<"producers2"_n, producer_info2> producers_table2;

typedef eosio::singleton<"global"_n, eosio_global_state> global_state_singleton;
typedef eosio::singleton<"global2"_n, eosio_global_state2> global_state2_singleton;
typedef eosio::singleton<"global3"_n, eosio_global_state3> global_state3_singleton;

static constexpr uint32_t seconds_per_day = 24 * 3600;



class [[eosio::contract("fio.system")]] system_contract : public native {

private:
    voters_table _voters;
    producers_table _producers;
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
    void init(unsigned_int version, symbol core);

    [[eosio::action]]
    void onblock(ignore <block_header> header);

    // functions defined in delegate_bandwidth.cpp

    // functions defined in voting.cpp

    [[eosio::action]]
    void regiproducer(const name producer, const string &producer_key, const std::string &url, uint16_t location,
                      string fio_address);

    [[eosio::action]]
    void regproducer(const string fio_address, const std::string &url, uint16_t location, const name actor,
                     const uint64_t max_fee);

    [[eosio::action]]
    void unregprod(const string fio_address, const name actor, const uint64_t max_fee);

    [[eosio::action]]
    void vproducer(const name voter, const name proxy, const std::vector<name> &producers); //server call

    [[eosio::action]]
    void voteproducer(const std::vector<string> &producers, const name actor, const uint64_t max_fee);

    [[eosio::action]]
    void updatepower(const name &voter, bool updateonly);

    [[eosio::action]]
    void voteproxy(const string fio_address, const name actor, const uint64_t max_fee);

    [[eosio::action]]
    void setautoproxy(name proxy,name owner);

    [[eosio::action]]
    void crautoproxy(name proxy,name owner);

    [[eosio::action]]
    void unregproxy(const std::string &fio_address, const name &actor, const uint64_t max_fee);

    [[eosio::action]]
    void regproxy(const std::string &fio_address, const name &actor, const uint64_t max_fee);

    [[eosio::action]]
    void regiproxy(const name proxy, const string &fio_address, bool isproxy);

    [[eosio::action]]
    void setparams(const eosio::blockchain_parameters &params);

    // functions defined in producer_pay.cpp
    [[eosio::action]]
    void claimrewards(const name owner);

    [[eosio::action]]
    void resetclaim(const name producer);

    [[eosio::action]]
    void setpriv(name account, uint8_t is_priv);

    [[eosio::action]]
    void rmvproducer(name producer);

    [[eosio::action]]
    void updtrevision(uint8_t revision);

    using init_action = eosio::action_wrapper<"init"_n, &system_contract::init>;
    using regproducer_action = eosio::action_wrapper<"regproducer"_n, &system_contract::regproducer>;
    using regiproducer_action = eosio::action_wrapper<"regiproducer"_n, &system_contract::regiproducer>;
    using unregprod_action = eosio::action_wrapper<"unregprod"_n, &system_contract::unregprod>;
    using vproducer_action = eosio::action_wrapper<"vproducer"_n, &system_contract::vproducer>;
    using voteproducer_action = eosio::action_wrapper<"voteproducer"_n, &system_contract::voteproducer>;
    using voteproxy_action = eosio::action_wrapper<"voteproxy"_n, &system_contract::voteproxy>;
    using regproxy_action = eosio::action_wrapper<"regproxy"_n, &system_contract::regproxy>;
    using regiproxy_action = eosio::action_wrapper<"regiproxy"_n, &system_contract::regiproxy>;
    using crautoproxy_action = eosio::action_wrapper<"crautoproxy"_n, &system_contract::crautoproxy>;
    using claimrewards_action = eosio::action_wrapper<"claimrewards"_n, &system_contract::claimrewards>;
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
    void update_elected_producers(block_timestamp timestamp);

    void update_votes(const name voter, const name proxy, const std::vector <name> &producers, bool voting);

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
