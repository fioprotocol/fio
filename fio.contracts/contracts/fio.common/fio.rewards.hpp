/** FioRewards header file
 *  Description: FioRewards header is included in fio.common.hpp and contains tables and actions to issue rewards.
 *    These actions should only be used by fee invoking transactions originating from the end user.
 *  @author Adam Androulidakis
 *  @modifedby
 *  @file fio.rewards.hpp
 *  @copyright Dapix
 */

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

    // @abi table bounties i64
    struct [[eosio::action]] bounty {

      uint64_t tokensminted;
      uint64_t primary_key() const {return tokensminted;}

      EOSLIB_SERIALIZE(bounty, (tokensminted))


    };

    typedef multi_index<"bounties"_n, bounty> bounties_table;

    void process_rewards(const string &tpid, const uint64_t &amount, const name &actor) {

        action(
        permission_level{actor,"active"_n},
        "fio.treasury"_n,
        "fdtnrwdupdat"_n,
        std::make_tuple((uint64_t)(static_cast<double>(amount) * .02))
        ).send();


        if (!tpid.empty()) {
          bounties_table bounties("fio.tpid"_n, name("fio.tpid").value);
          uint64_t bamount = 0;
          eosio::print("\nBounties: ",bounties.begin()->tokensminted,"\n");
          if (bounties.begin()->tokensminted < 200000000000000000) {
            bamount = (uint64_t)(static_cast<double>(amount) * .65);
            eosio::print("\nBounty payout: ", bamount, "\n");
            action(permission_level{"fio.treasury"_n, "active"_n},
              "fio.token"_n, "mintfio"_n,
              make_tuple(bamount)
            ).send();

            action(
            permission_level{"fio.treasury"_n,"active"_n},
            "fio.tpid"_n,
            "updatebounty"_n,
            std::make_tuple(bamount)
            ).send();

          }

          action(
          permission_level{actor,"active"_n},
          "fio.tpid"_n,
          "updatetpid"_n,
          std::make_tuple(tpid, actor, (amount / 10) + bamount)
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

          bounties_table bounties("fio.tpid"_n, name("fio.tpid").value);
          uint64_t bamount = 0;
          eosio::print("\nBounties: ",bounties.begin()->tokensminted,"\n");
          if (bounties.begin()->tokensminted < 200000000000000000) {
            bamount = (uint64_t)(static_cast<double>(amount) * .65);
            eosio::print("\nBounty payout: ", bamount, "\n");
            action(permission_level{"fio.treasury"_n, "active"_n},
              "fio.token"_n, "mintfio"_n,
              make_tuple(bamount)
            ).send();

            action(
            permission_level{"fio.treasury"_n,"active"_n},
            "fio.tpid"_n,
            "updatebounty"_n,
            std::make_tuple(bamount)
            ).send();

          }

          action(
          permission_level{actor,"active"_n},
          "fio.tpid"_n,
          "updatetpid"_n,
          std::make_tuple(tpid, actor, (amount / 10) + bamount)
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
