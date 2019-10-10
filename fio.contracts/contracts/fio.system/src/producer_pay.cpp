#include <fio.system/fio.system.hpp>

#include <fio.token/fio.token.hpp>

namespace eosiosystem {

    const int64_t min_pervote_daily_pay = 100'0000;
    const int64_t min_activated_stake = 150'000'000'0000;
    const double continuous_rate = 0.04879;          // 5% annual rate
    const double perblock_rate = 0.0025;           // 0.25%
    const double standby_rate = 0.0075;           // 0.75%
    const uint32_t blocks_per_year = 52 * 7 * 24 * 2 * 3600;   // half seconds per year
    const uint32_t seconds_per_year = 52 * 7 * 24 * 3600;
    const uint32_t blocks_per_day = 2 * 24 * 3600;
    const uint32_t blocks_per_hour = 2 * 3600;
    const int64_t useconds_per_day = 24 * 3600 * int64_t(1000000);
    const int64_t useconds_per_year = seconds_per_year * 1000000ll;

    void system_contract::onblock(ignore <block_header>) {
        using namespace eosio;

        require_auth(_self);

        block_timestamp timestamp;
        name producer;
        _ds >> timestamp >> producer;

        // _gstate2.last_block_num is not used anywhere in the system contract code anymore.
        // Although this field is deprecated, we will continue updating it for now until the last_block_num field
        // is eventually completely removed, at which point this line can be removed.
        _gstate2.last_block_num = timestamp;

        /** until activated stake crosses this threshold no new rewards are paid */
        //in FIO we are not concerned with staking so we will not check
        //the min activated stake.
        //if (_gstate.total_activated_stake < min_activated_stake)
        //    return;

        if (_gstate.last_pervote_bucket_fill == time_point())  /// start the presses
            _gstate.last_pervote_bucket_fill = current_time_point();


        /**
         * At startup the initial producer may not be one that is registered / elected
         * and therefore there may be no producer object for them.
         */
        auto prod = _producers.find(producer.value);
        if (prod != _producers.end()) {
            _gstate.total_unpaid_blocks++;
            _producers.modify(prod, same_payer, [&](auto &p) {
                p.unpaid_blocks++;
            });
        }

        /// only update block producers once every minute, block_timestamp is in half seconds
        if (timestamp.slot - _gstate.last_producer_schedule_update.slot > 120) {
            update_elected_producers(timestamp);
        }
    }

    using namespace eosio;

    void system_contract::resetclaim(const name producer) {
      require_auth(get_self());
        const auto &prod = _producers.get(producer.value);
           // Reset producer claim info
           _producers.modify(prod, get_self(), [&](auto &p) {
               p.last_claim_time = time_point {microseconds{static_cast<int64_t>( current_time())}};
               p.unpaid_blocks = 0;
           });


    }
} //namespace eosiosystem
