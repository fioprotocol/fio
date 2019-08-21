/** FioTreasury implementation file
 *  Description: FioTreasury smart contract controls block producer and tpid payments.
 *  @author Adam Androulidakis
 *  @modifedby
 *  @file fio.treasury.cpp
 *  @copyright Dapix
 */

#include "fio.treasury.hpp"

namespace fioio {

  class [[eosio::contract("FIOTreasury")]]  FIOTreasury : public eosio::contract {


    private:
      tpids_table tpids;
      fionames_table fionames;
      domains_table domains;
      rewards_table clockstate;
      bprewards_table bprewards;
      bpbucketpool_table bucketrewards;
      fdtnrewards_table fdtnrewards;
      voteshares_table voteshares;
      eosiosystem::eosio_global_state gstate;
      eosiosystem::global_state_singleton global;
      eosiosystem::producers_table producers;
      bool rewardspaid;

      uint64_t lasttpidpayout;

    public:
      using contract::contract;


        FIOTreasury(name s, name code, datastream<const char *> ds) : contract(s, code, ds),
                                                                      tpids(TPIDContract, TPIDContract.value),
                                                                      fionames(SystemContract, SystemContract.value),
                                                                      domains(SystemContract, SystemContract.value),
                                                                      bprewards(_self, _self.value),
                                                                      clockstate(_self, _self.value),
                                                                      voteshares(_self, _self.value),
                                                                      producers("eosio"_n, name("eosio").value),
                                                                      global("eosio"_n, name("eosio").value),
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

      fio_400_assert(fioiter != fionames.end(), fio_address, "fio_address",
        "FIO Address not producer or nothing payable", ErrorNoFioAddressProducer);

      auto domiter = domains.find(fioiter->domainhash);
      fio_400_assert(now() < domiter->expiration, domiter->name, "domain",
        "FIO Domain expired", ErrorDomainExpired);

      fio_400_assert(now() < fioiter->expiration, fio_address, "fio_address",
        "FIO Address expired", ErrorFioNameExpired);

      uint64_t producer = fioiter->owner_account;

      auto clockiter = clockstate.begin();

      //if it has been 24 hours, transfer remaining producer vote_shares to the foundation and record the rewards back into bprewards,
      // then erase the pay scheduler so a new one can be created.

      // If there is no pay schedule then create a new one
      uint64_t sharesize = std::distance(voteshares.begin(), voteshares.end());
      if (sharesize == 0) { //if new payschedule
        //Create the payment schedule

        double schedvotetotal;
        uint64_t bpcounter = 0;
        auto proditer = producers.get_index<"prototalvote"_n>();
        for( const auto& itr : proditer ) {

              voteshares.emplace(get_self(), [&](auto &p) {
                p.owner = itr.owner;
                p.votes = itr.total_votes;
                p.lastclaim = now();
              });
           // to this line *****

                schedvotetotal += itr.total_votes;
            clockstate.modify(clockstate.begin(),get_self(), [&](auto &entry) {
              entry.schedvotetotal = schedvotetotal;
            });

            bpcounter++;
            if (bpcounter > 42) break;
          } // &itr : producers


          //split up bprewards to bpreward->dailybucket (40%) and bpbucketpool->rewards (60%)

          uint64_t temp = bucketrewards.begin()->rewards;
          bucketrewards.erase(bucketrewards.begin());
          bucketrewards.emplace(get_self(), [&](auto &p) {
            p.rewards = temp + bprewards.begin()->rewards * .60;
          });


          temp = bprewards.begin()->dailybucket + bprewards.begin()->rewards * .40;
          bprewards.erase(bprewards.begin());
          bprewards.emplace(get_self(), [&](auto &p) {
              p.dailybucket = temp;
              p.rewards = 0; //This was emptied upon distributing to bucketrewards in the previous call
          });

          // All items are now in pay schedule, calculate the shares
          uint64_t bpcount = std::distance(voteshares.begin(),voteshares.end());
          uint64_t abpcount = 21;
          if (bpcount >= 42) bpcount = 42; //limit to 42 producers in voteshares
          if (bpcount <= 21) abpcount = bpcount;


          double todaybucket = bucketrewards.begin()->rewards / 365;


          bpcounter = 0;
          for(auto &itr : voteshares) {
              if (bpcounter<= abpcount) {

                double reward = static_cast<double>(bprewards.begin()->dailybucket / abpcount); // dailybucket / 21
                print("\ndailybucket: ",bprewards.begin()->dailybucket);
                print("\nactive producer count: ",abpcount);
                gstate = global.get();
                print("\ntotal_producer_vote_weight: ",gstate.total_producer_vote_weight);
                print("\nvotes: ",itr.votes);

                voteshares.modify(itr,get_self(), [&](auto &entry) {
                  entry.abpayshare = (reward * (itr.votes / gstate.total_producer_vote_weight));
                });
                print("\nreward: ", reward);
              }

              voteshares.modify(itr,get_self(), [&](auto &entry) {
                  entry.sbpayshare = (static_cast<double>(todaybucket / bpcount)); //todaybucket / 42
              });

              bpcounter++;
          } // &itr : voteshares


        //Start 24 track for daily pay
        clockstate.modify(clockiter, get_self(), [&](auto &entry) {
          entry.payschedtimer = now();
        });
        print("Voteshares processed","\n"); //To remove after testing
        return;

      } //if new payschedule


      //This contract should only allow the producer to be able to claim rewards once every x blocks.

      // Pay schedule expiration

      //if it has been 24 hours, transfer remaining producer vote_shares to the foundation and record the rewards back into bprewards,
      // then erase the pay schedule so a new one can be created in a subsequent call to bpclaim.
      if(now() >= clockiter->payschedtimer + 17 ) { //+ 172800

        if (sharesize > 0) {

          auto iter = voteshares.begin();
          while (iter != voteshares.end()) {

                uint64_t reward = bucketrewards.begin()->rewards;
                reward += static_cast<uint64_t>(iter->sbpayshare + iter->abpayshare);
                bucketrewards.erase(bucketrewards.begin());
                bucketrewards.emplace(_self, [&](struct bucketpool& entry) {
                  entry.rewards = reward;
                });

              iter = voteshares.erase(iter);
            }

            // reset total schedule vote shares, needs to be recalculated when spawning new pay schedule

            clockstate.modify(clockstate.begin(),get_self(), [&](auto &entry) {
              entry.schedvotetotal = 0;
            });

            print("Pay schedule erased... Creating new pay schedule...","\n"); //To remove after testing
            bpclaim(fio_address, actor); // Call self to create a new pay schedule
        }

      return;
     }

     //This check must happen after the payschedule so a producer account can terminate the old pay schedule and spawn a new one in a subsequent call to bpclaim
     auto bpiter = voteshares.find(producer);

     fio_400_assert(bpiter != voteshares.end(), fio_address, "fio_address",
       "FIO Address not producer or nothing payable", ErrorNoFioAddressProducer);

     const auto &prod = producers.get(producer);

     /******* Payouts *******/
     //This contract should only allow the producer to be able to claim rewards once every 172800 blocks (1 day).
     uint64_t payout = 0;

     if( now() > bpiter->lastclaim + 17 ) { //+ 172800
       check(prod.active(), "producer does not have an active key");

             action(permission_level{get_self(), "active"_n},
               "fio.token"_n, "transfer"_n,
               make_tuple("fio.treasury"_n, name(bpiter->owner), asset(bpiter->abpayshare+bpiter->sbpayshare, symbol("FIO",9)),
               string("Paying producer from treasury."))
           ).send();

     // Reduce the producers share of dailybucket and bucketrewards

     bucketrewards.erase(bucketrewards.begin());
     bucketrewards.emplace(get_self(), [&](auto &p) {
       p.rewards -= bpiter->sbpayshare;
     });

     auto temp = bprewards.begin()->rewards;
     bprewards.erase(bprewards.begin());
     bprewards.emplace(get_self(), [&](auto &p) {
         p.dailybucket -= bpiter->abpayshare;
         p.rewards = temp;
     });

    // PAY FOUNDATION //
     auto fdtniter = fdtnrewards.begin();
     if (fdtniter->rewards > 100) { // 100 FIO = 100000000000 SUFs
         action(permission_level{get_self(), "active"_n},
               "fio.token"_n, "transfer"_n,
               make_tuple("fio.treasury"_n, FOUNDATIONACCOUNT, asset(fdtniter->rewards,symbol("FIO",9)),
               string("Paying foundation from treasury."))
             ).send();

       //Clear the foundation rewards counter

          fdtnrewards.erase(fdtniter);
          fdtnrewards.emplace(_self, [&](struct fdtnreward& entry) {
            entry.rewards = 0;
         });
    //////////////////////////////////////

     }


     //Invoke system contract to reset producer last_claim_time and unpaid_blocks
     action(permission_level{get_self(), "active"_n},
           "fio.system"_n, "resetclaim"_n,
           make_tuple(producer)
         ).send();

     //remove the producer from payschedule
     voteshares.erase(bpiter);


   } //endif now() > bpiter + 172800



     nlohmann::json json = {{"status",        "OK"},
                            {"amount",    payout}};
     send_response(json.dump().c_str());



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
