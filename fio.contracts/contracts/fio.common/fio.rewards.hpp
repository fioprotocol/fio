#pragma once

#include <vector>
#include <string>
#include <eosiolib/eosio.hpp>

namespace fioio {


    // @abi table bpreward i64
    struct [[eosio::action]] bpreward {

      uint64_t rewards;
      uint64_t dailybucket;

      uint64_t primary_key() const {return rewards;}

      EOSLIB_SERIALIZE(bpreward, (rewards)(dailybucket))


    };

    typedef multi_index<"bprewards"_n, bpreward> bprewards_table;

    // @abi table bucketpool i64
    struct [[eosio::action]] bucketpool {

      uint64_t rewards;

      uint64_t primary_key() const {return rewards;}

      EOSLIB_SERIALIZE(bucketpool, (rewards))


    };

    typedef multi_index<"bpbucketpool"_n, bucketpool> bpbucketpool_table;

    // @abi table fdtnreward i64
    struct [[eosio::action]] fdtnreward {

      uint64_t rewards;

      uint64_t primary_key() const {return rewards;}

      EOSLIB_SERIALIZE(fdtnreward, (rewards))


    };

    typedef multi_index<"fdtnrewards"_n, fdtnreward> fdtnrewards_table;


    void process_rewards(const string &tpid, const uint64_t &amount, const name &actor) {

        action(
        permission_level{actor,"active"_n},
        "fio.treasury"_n,
        "fdtnrwdupdat"_n,
        std::make_tuple((uint64_t)(static_cast<double>(amount) * .02))
        ).send();



        if (!tpid.empty()) {
          action(
          permission_level{actor,"active"_n},
          "fio.tpid"_n,
          "updatetpid"_n,
          std::make_tuple(tpid, actor, amount / 10)
          ).send();


          action(
          permission_level{actor,"active"_n},
          "fio.treasury"_n,
          "bprewdupdate"_n,
          std::make_tuple((uint64_t)(static_cast<double>(amount) * .88))
          ).send();

      } else {

        action(
        permission_level{actor,"active"_n},
        "fio.treasury"_n,
        "bprewdupdate"_n,
        std::make_tuple((uint64_t)(static_cast<double>(amount) * .98))
        ).send();

      }

    }


    void processbucketrewards(const string &tpid, const uint64_t &amount, const name &actor) {

        action(
        permission_level{actor,"active"_n},
        "fio.treasury"_n,
        "fdtnrwdupdat"_n,
        std::make_tuple((uint64_t)(static_cast<double>(amount) * .02))
        ).send();



        if (!tpid.empty()) {
          action(
          permission_level{actor,"active"_n},
          "fio.tpid"_n,
          "updatetpid"_n,
          std::make_tuple(tpid, actor, amount / 10)
          ).send();


          action(
          permission_level{actor,"active"_n},
          "fio.treasury"_n,
          "bppoolupdate"_n,
          std::make_tuple((uint64_t)(static_cast<double>(amount) * .88))
          ).send();

      } else {

        action(
        permission_level{actor,"active"_n},
        "fio.treasury"_n,
        "bppoolupdate"_n,
        std::make_tuple((uint64_t)(static_cast<double>(amount) * .98))
        ).send();

      }

    }


} // namespace fioio
