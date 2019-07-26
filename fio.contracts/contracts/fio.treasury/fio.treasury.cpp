#include "fio.treasury.hpp"

namespace fioio {

  class [[eosio::contract("FIOTreasury")]]  FIOTreasury : public eosio::contract {


    private:
      tpids_table tpids;
      fionames_table fionames;
      rewards_table clockstate;
      bprewards_table bprewards;
      bpbucketpool_table bucketrewards;
      fdtnrewards_table fdtnrewards;
      voteshares_table voteshares;
      eosiosystem::producers_table producers;

      bool rewardspaid;

      uint64_t lasttpidpayout;

    public:
      using contract::contract;


        FIOTreasury(name s, name code, datastream<const char *> ds) : contract(s, code, ds),
                                                                      tpids(TPIDContract, TPIDContract.value),
                                                                      fionames(SystemContract, SystemContract.value),
                                                                      bprewards(_self, _self.value),
                                                                      clockstate(_self, _self.value),
                                                                      voteshares(_self, _self.value),
                                                                      producers("eosio"_n, name("eosio").value),
                                                                      fdtnrewards(_self, _self.value),
                                                                      bucketrewards(_self, _self.value) {

        }


      // @abi action
      [[eosio::action]]
      void tpidclaim(const name& actor) {

      require_auth(actor);

      uint64_t tpids_paid = 0;
      //If the contract has not been invoked yet, this will execute and set the initial block time
      auto clockiter = clockstate.begin();

      //This contract should only be able to iterate throughout the entire tpids table to
      //to check for rewards once every x blocks.
       if( now() > clockiter->lasttpidpayout + 60 ) {

          for(auto &itr : tpids) {

            //TODO: Change after MAS-425 UAT
            if (itr.rewards >= 100000)  {  //100 FIO (100,000,000,000 SUF)

               print(itr.fioaddress, " has ",itr.rewards ," rewards.\n");

               auto itrfio = fionames.find(string_to_uint64_hash(itr.fioaddress.c_str()));

               // If the fioaddress exists (address could have been burned)
                if (itrfio != fionames.end()) {
                    action(permission_level{get_self(), "active"_n},
                          "fio.token"_n, "transfer"_n,
                          make_tuple("fio.treasury"_n, name(itrfio->owner_account), asset(itr.rewards,symbol("FIO",9)),
                          string("Paying TPID from treasury."))
                   ).send();


                  }  else  { //Allocate to BP buckets instead
                   print(itr.fioaddress, " FIO address has expired and no longer exists. Allocating to block producer rewards.","\n");

                   auto bpfound = bprewards.begin();
                   uint64_t reward = bpfound->rewards;
                   reward += itr.rewards;
                   bprewards.erase(bpfound);
                   bprewards.emplace(_self, [&](struct bpreward& entry) {
                     entry.rewards = reward;
                  });


                 }

               action(permission_level{get_self(), "active"_n},
                      "fio.tpid"_n, "rewardspaid"_n,
                      make_tuple(itr.fioaddress)
               ).send();


              } // endif itr.rewards >=

              tpids_paid++;
              if (tpids_paid >= 100) break;
          } // for tpids.begin() tpids.end()

          //update the clock but only if there has been a tpid paid out.
          if(tpids_paid > 0) {
            action(permission_level{get_self(), "active"_n},
                   get_self(), "updateclock"_n,
                   make_tuple()
            ).send();
        }

      } //end if lasttpidpayout < now() 60

       nlohmann::json json = {{"status",        "OK"},
                              {"tpids_paid",    tpids_paid}};
       send_response(json.dump().c_str());

    } //tpid_claim


    // @abi action
    [[eosio::action]]
    void bpclaim(const string &fio_address, const name& actor) {

      require_auth(actor);
      auto fioiter = fionames.find(string_to_uint64_hash(fio_address.c_str()));

      //Replace with proper assertion from api spec
      if (fioiter == fionames.end()) {
        print("Failed to locate producer in fionames table.");
        return;
      }
      uint64_t producer = fioiter->owner_account;

      auto clockiter = clockstate.begin();

      //if it has been 24 hours, transfer remaining producer vote_shares to the foundation and record the rewards back into bprewards,
      // then erase the pay scheduler so a new one can be created.

      uint64_t sharesize = std::distance(voteshares.begin(), voteshares.end());
      // If there is no pay schedule then create a new one
      if (sharesize == 0) {
        //Create the payment schedule

        for(auto &itr : producers) {


          //If active block producer
          if(itr.is_active) {


          //Determine the share amount


          //Take producer and place in shares tables
            voteshares.emplace(get_self(), [&](auto &p) {
              p.owner = itr.owner;
              p.votes = itr.total_votes;
              p.lastclaim = now();
              p.votepay_share = itr.total_votes; //Changing to use the percentile
            });
          }

        }

        //Start 24 track for daily pay
        clockstate.modify(clockiter, get_self(), [&](auto &entry) {
          entry.payschedtimer = now();
        });
        print("Voteshares processed","\n"); //To remove after testing
        return;
      }

      auto bpiter = voteshares.find(producer);
      if (bpiter == voteshares.end()) {
          print("Failed to locate producer in voteshares."); //To remove after testing
          return;
      }

      //This contract should only allow the producer to be able to claim rewards once every x blocks.

      //if it has been 24 hours, transfer remaining producer vote_shares to the foundation and record the rewards back into bprewards,
      // then erase the pay schedule so a new one can be created.
      if(now() >= clockiter->payschedtimer + 17 ) { //+ 172800

        if (sharesize > 0) {

          auto iter = voteshares.begin();
          while (iter != voteshares.end()) {

                auto found = bucketrewards.begin();
                uint64_t reward = found->rewards;
                reward += static_cast<uint64_t>(iter->votepay_share);
                bucketrewards.erase(found);
                bucketrewards.emplace(_self, [&](struct bucketpool& entry) {
                  entry.rewards = reward;
                });

              iter = voteshares.erase(iter);
            }
        }

      return;
     }




   } //bpclaim

    // @abi action
    [[eosio::action]]
    void updateclock() {
      require_auth("fio.treasury"_n);

      auto clockiter = clockstate.begin();

      clockstate.erase(clockiter);

      clockstate.emplace(_self, [&](struct treasurystate& entry) {
      entry.lasttpidpayout = now();
    });
    }

    // @abi action
    [[eosio::action]]
    void startclock() {
      require_auth("fio.treasury"_n);

      unsigned int size = std::distance(clockstate.begin(),clockstate.end());
      if (size == 0)  {
          clockstate.emplace(_self, [&](struct treasurystate& entry) {
          entry.lasttpidpayout = now() - 56;
          entry.payschedtimer = now() - 172780;
        });

      }

      bprewdupdate(0);

    }

    // @abi action
    [[eosio::action]]
    void bprewdupdate(const uint64_t &amount) {

      eosio_assert((has_auth(SystemContract) || has_auth("fio.token"_n)) || has_auth("fio.treasury"_n) || (has_auth("fio.reqobt"_n)),
        "missing required authority of fio.system, fio.token, or fio.reqobt");

        uint64_t size = std::distance(bprewards.begin(),bprewards.end());
        if (size == 0)  {
          bprewards.emplace(_self, [&](struct bpreward& entry) {
            entry.rewards = amount;
         });

       } else {
         auto found = bprewards.begin();
         uint64_t reward = found->rewards;
         reward += amount;
         bprewards.erase(found);
         bprewards.emplace(_self, [&](struct bpreward& entry) {
           entry.rewards = reward;
        });
       }

    }

    // @abi action
    [[eosio::action]]
    void bppoolupdate(const uint64_t &amount) {

      eosio_assert((has_auth(SystemContract) || has_auth("fio.token"_n)) || has_auth("fio.treasury"_n) || (has_auth("fio.reqobt"_n)),
        "missing required authority of fio.system, fio.token, or fio.reqobt");

        uint64_t size = std::distance(bucketrewards.begin(),bucketrewards.end());
        if (size == 0)  {
          bucketrewards.emplace(_self, [&](struct bucketpool& entry) {
            entry.rewards = amount;
         });

       } else {
         auto found = bucketrewards.begin();
         uint64_t reward = found->rewards;
         reward += amount;
         bucketrewards.erase(found);
         bucketrewards.emplace(_self, [&](struct bucketpool& entry) {
           entry.rewards = reward;
        });
       }

    }

    // @abi action
    [[eosio::action]]
    void fdtnrwdupdat(const uint64_t &amount) {

      eosio_assert((has_auth(SystemContract) || has_auth("fio.token"_n)) || has_auth("fio.treasury"_n) || (has_auth("fio.reqobt"_n)),
        "missing required authority of fio.system, fio.token, or fio.reqobt");

        uint64_t size = std::distance(fdtnrewards.begin(),fdtnrewards.end());
        if (size == 0)  {
          fdtnrewards.emplace(_self, [&](struct fdtnreward& entry) {
            entry.rewards = amount;
         });

       } else {
         auto found = fdtnrewards.begin();
         uint64_t reward = found->rewards;
         reward += amount;
         fdtnrewards.erase(found);
         fdtnrewards.emplace(_self, [&](struct fdtnreward& entry) {
           entry.rewards = reward;
        });
       }

    }


    // @abi action
    [[eosio::action]]
    void fdtnrwdreset(const bool &paid) {

      eosio_assert((has_auth(SystemContract) || has_auth("fio.token"_n)) || has_auth("fio.treasury"_n) || (has_auth("fio.reqobt"_n)),
        "missing required authority of fio.system, fio.token, or fio.reqobt");

        if (!paid) {




          rewardspaid = true;

        }



    }


    // maintain
    // Can only iterate through tpids table to be called once every 1200000 blocks
    // @params none
    // @abi action
    [[eosio::action]]
    void maintain() {


    auto clockiter = clockstate.begin();

    // Maintenance check can be run every 1200000 blocks
     if( now() > clockiter->lasttpidpayout + 1200000 ) {


       for(auto &itr : tpids) {

       }



     }
   }


  }; //class TPIDController



  EOSIO_DISPATCH(FIOTreasury, (tpidclaim)(updateclock)(startclock)(bprewdupdate)(fdtnrwdupdat)(bppoolupdate)(bpclaim)(maintain))
}
