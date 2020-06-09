#include <fio.system/fio.system.hpp>

#include <fio.token/fio.token.hpp>

namespace eosiosystem {

    const int64_t min_pervote_daily_pay = 100'0000;
    const double continuous_rate = 0.04879;          // 5% annual rate
    const double perblock_rate = 0.0025;           // 0.25%
    const double standby_rate = 0.0075;           // 0.75%
    const uint32_t blocks_per_year = 52 * 7 * 24 * 2 * 3600;   // half seconds per year
    const uint32_t seconds_per_year = 52 * 7 * 24 * 3600;
    const uint32_t blocks_per_day = 2 * 24 * 3600;
    const uint32_t blocks_per_hour = 2 * 3600;
    const int64_t useconds_per_day = 24 * 3600 * int64_t(1000000);
    const int64_t useconds_per_year = seconds_per_year * 1000000ll;

    using namespace eosio;

    void system_contract::onblock(ignore <block_header>) {


        require_auth(_self);

        block_timestamp timestamp;
        name producer;
        _ds >> timestamp >> producer;

        _gstate2.last_block_num = timestamp;

        /** until voting activated fio crosses this threshold no new rewards are paid */
        if( _gstate.total_voted_fio < MINVOTEDFIO && _gstate.thresh_voted_fio_time == time_point()  ){
            return;
        }

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
            //invoke the fee computation.

        }

        /* this needs testing, until tested we will remove it.
        if (timestamp.slot - _gstate.last_fee_update.slot > 126) {

          action(permission_level{get_self(), "active"_n},
                 "fio.fee"_n, "updatefees"_n,
                 make_tuple(_self)
          ).send();

          _gstate.last_fee_update = timestamp;

        }
         */
    }

    void system_contract::resetclaim(const name &producer) {
      check((has_auth(SYSTEMACCOUNT) ||  has_auth(TREASURYACCOUNT)) ,
               "missing required authority of treasury or eosio");
           auto prodbyowner = _producers.get_index<"byowner"_n>();
           auto proditer = prodbyowner.find(producer.value);
          // Reset producer claim info
           prodbyowner.modify(proditer, get_self(), [&](auto &p) {
               p.last_claim_time = time_point {microseconds{static_cast<int64_t>( current_time())}};
               p.unpaid_blocks = 0;
           });
    }

    void system_contract::updlbpclaim(const name &producer) {
        check((has_auth(SYSTEMACCOUNT) ||  has_auth(TREASURYACCOUNT)) ,
              "missing required authority of treasury or eosio");
        auto prodbyowner = _producers.get_index<"byowner"_n>();
        auto proditer = prodbyowner.find(producer.value);
        // update last_bpclaim
        prodbyowner.modify(proditer, get_self(), [&](auto &p) {
            p.last_bpclaim = now();
        });


    }
} //namespace eosiosystem
