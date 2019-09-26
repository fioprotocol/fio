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
#include "fio.common.hpp"

namespace fioio {
    static constexpr uint64_t string_to_uint64_hash(const char *str); //signature for static constexpr implementation fio.common.hpp
    static uint128_t string_to_uint128_hash(const char *str);
    // @abi table bpreward i64
    struct [[eosio::action]] bpreward {

      uint64_t rewards;

      uint64_t primary_key() const {return rewards;}

      EOSLIB_SERIALIZE(bpreward, (rewards))


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
      fionames_table fionames("fio.system"_n, name("fio.system").value);
      uint128_t fioaddhash = string_to_uint128_hash(tpid.c_str());

      auto namesbyname = fionames.get_index<"byname"_n>();
      auto fionamefound = namesbyname.find(fioaddhash);

      print("\nfionamefound: ",fionamefound->name,"\n");
      if (fionamefound != namesbyname.end()) {

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

        eosio::print("\nTest Bucket Update Amount: ",((uint64_t)(static_cast<double>(amount) * .88)), "\n");
      } else {
      print("Cannot register TPID or FIO Address not found. The transaction will continue without TPID payment.","\n");

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

        fionames_table fionames("fio.system"_n, name("fio.system").value);
        uint128_t fioaddhash = string_to_uint128_hash(tpid.c_str());
        auto namesbyname = fionames.get_index<"byname"_n>();
        auto fionamefound = namesbyname.find(fioaddhash);
        print("\nfionamefound: ",fionamefound->name,"\n");
        if (fionamefound != namesbyname.end()) {

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
          eosio::print("\nTest Bucket Update Amount: ",((uint64_t)(static_cast<double>(amount) * .88)), "\n");
        } else {
        print("Cannot register TPID or FIO Address not found. The transaction will continue without TPID payment.","\n");

        action(
        permission_level{actor,"active"_n},
        "fio.treasury"_n,
        "bppoolupdate"_n,
        std::make_tuple((uint64_t)(static_cast<double>(amount) * .98))
        ).send();

      }
    }


  //Precondition: this method should only be called by register_producer, vote_producer, unregister_producer, register_proxy, unregister_proxy, vote_proxy
  // after transaction fees have been defined
  //Postcondition: the foundation has been rewarded 2% of the transaction fee and top 21/active block producers rewarded 98% of the transaction fee
  void processrewardsnotpid(const uint64_t &amount, const name &actor) {

    action(
    permission_level{actor,"active"_n},
    "fio.treasury"_n,
    "bprewdupdate"_n,
    std::make_tuple((uint64_t)(static_cast<double>(amount) * .98))
    ).send();

    action(
    permission_level{actor,"active"_n},
    "fio.treasury"_n,
    "fdtnrwdupdat"_n,
    std::make_tuple((uint64_t)(static_cast<double>(amount) * .02))
    ).send();
  }



} // namespace fioio
