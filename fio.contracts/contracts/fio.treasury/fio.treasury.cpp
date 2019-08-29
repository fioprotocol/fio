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
              if (tpids_paid >= 100) break; //only paying 100 tpids
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

      uint64_t producer = fioiter->owner_account;

      auto clockiter = clockstate.begin();

      uint64_t payout = 0;
      nlohmann::json json = {{"status",        "OK"},
                             {"amount",    payout}};

     /***************  Pay schedule expiration *******************/
     //if it has been 24 hours, transfer remaining producer vote_shares to the foundation and record the rewards back into bprewards,
     // then erase the pay schedule so a new one can be created in a subsequent call to bpclaim.
     if(now() >= clockiter->payschedtimer + 120 ) { //+ 172800

       if (std::distance(voteshares.begin(), voteshares.end()) > 0) {

         auto iter = voteshares.begin();
         while (iter != voteshares.end()) {

               uint64_t reward = bucketrewards.begin()->rewards;
               print("\nReward initialized to: ", reward);
               reward += (iter->sbpayshare + iter->abpayshare);
               print("\nspbayshare: ",iter->sbpayshare);
               print("\nabpayshare: ", iter->abpayshare);
               //reward = 100;
               print("\nreward: ",reward);

               bucketrewards.erase(bucketrewards.begin());
               bucketrewards.emplace(_self, [&](struct bucketpool &p) {
                 p.rewards = reward;
               });

               iter = voteshares.erase(iter);
               print("\n\nDone\n\n\n\n");
           }

           print("\nPay schedule erased... ");
        }

        // if clockstate.begin()->rewardspaid < 5000000000000 && clockstate.begin()->reservetokensminted < 20000000000000000
        if (clockiter->rewardspaid < 50000 && clockiter->reservetokensminted < 200000000 && now() > clockiter->fortyeightmonths) { // lowered values for testing

          //Mint new tokens up to 50,000 FIO
          uint64_t tomint = 5000000000000 - clockiter->rewardspaid;
          /*action(permission_level{"fio.token"_n, "active"_n},
            "fio.token"_n, "issue"_n,
            make_tuple("fio.treasury"_n, asset(tomint, symbol("FIO",9)))
          ).send();
          */
          clockstate.modify(clockiter, get_self(), [&](auto &entry) {
            entry.reservetokensminted += tomint;
          });

        }

     }

      // If there is no pay schedule then create a new one
      if (std::distance(voteshares.begin(), voteshares.end()) == 0) { //if new payschedule
        //Create the payment schedule
        print("\nCreating new pay schedule... ");
        uint64_t bpcounter = 0;
        auto proditer = producers.get_index<"prototalvote"_n>();
        for( const auto& itr : proditer ) {

              voteshares.emplace(get_self(), [&](auto &p) {
                p.owner = itr.owner;
                p.votes = itr.total_votes;
                p.lastclaim = now();
              });
           // to this line *****

            bpcounter++;
            if (bpcounter > 42) break;
          } // &itr : producers


          //split up bprewards to bpreward->dailybucket (40%) and bpbucketpool->rewards (60%)
          uint64_t temp = bucketrewards.begin()->rewards;
          bucketrewards.erase(bucketrewards.begin());
          bucketrewards.emplace(get_self(), [&](auto &p) {
            p.rewards = temp + static_cast<uint64_t>(bprewards.begin()->rewards * .60);
          });

          temp = bprewards.begin()->rewards;
          bprewards.erase(bprewards.begin());
          bprewards.emplace(get_self(), [&](auto &p) {
              p.dailybucket = static_cast<uint64_t>((temp) * .40);
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
              if (bpcounter <= abpcount) {
                print("\n,block producers:, ", bpcount);
                print("\n,active producers:, ", abpcount);
                print("\n,BPCounter (current bp rank):, ", bpcounter);
                double reward = static_cast<double>(bprewards.begin()->dailybucket); // dailybucket / 21
                print("\n,Today bucket (365 / bpcount):, ", todaybucket);
                print("\n,standby bp reward bucket (bprewards):, ", reward);
                gstate = global.get();

                voteshares.modify(itr,get_self(), [&](auto &entry) {
                  entry.abpayshare = static_cast<uint64_t>(double(reward) * (itr.votes / gstate.total_producer_vote_weight));
                });
                print("\n,active producer share:,  ",static_cast<uint64_t>(double(reward) * (itr.votes / gstate.total_producer_vote_weight)));
                print("\n,active payout percent:, ",itr.votes / gstate.total_producer_vote_weight);
              }
              voteshares.modify(itr,get_self(), [&](auto &entry) {
                  entry.sbpayshare = (static_cast<uint64_t>(todaybucket / bpcount)); //todaybucket / 42
              });
              print("\n,standby block producer payshare (todaybucket / bpcount):, ", static_cast<uint64_t>(todaybucket / bpcount));
              bpcounter++;

              // Reduce the producers share of dailybucket and bucketrewards

              auto temp = bucketrewards.begin()->rewards;
              bucketrewards.erase(bucketrewards.begin());
              bucketrewards.emplace(get_self(), [&](auto &p) {
                p.rewards = temp - itr.sbpayshare;
              });

              temp = bprewards.begin()->rewards;
              auto temp2 = bprewards.begin()->dailybucket;
              bprewards.erase(bprewards.begin());
              bprewards.emplace(get_self(), [&](auto &p) {
                  p.dailybucket = temp2 - itr.abpayshare;
                  p.rewards = temp;
              });


          } // &itr : voteshares


        //Start 24 track for daily pay
        clockstate.modify(clockiter, get_self(), [&](auto &entry) {
          entry.payschedtimer = now();
        });
        print("Voteshares processed","\n"); //To remove after testing

      } //if new payschedule

      //This contract should only allow the producer to be able to claim rewards once every x blocks.

     //This check must happen after the payschedule so a producer account can terminate the old pay schedule and spawn a new one in a subsequent call to bpclaim

     auto bpiter = voteshares.find(producer);

     const auto &prod = producers.get(producer);

     /******* Payouts *******/
     //This contract should only allow the producer to be able to claim rewards once every 172800 blocks (1 day).
     payout = static_cast<uint64_t>(bpiter->abpayshare+bpiter->sbpayshare);
     print("/n/n****************");
     print("/n/nPayout Amount: ", payout);

     fio_400_assert(fioiter != fionames.end(), fio_address, "fio_address",
     "FIO Address not producer or nothing payable", ErrorNoFioAddressProducer);

     if(bpiter != voteshares.end() ) {

          auto domiter = domains.find(fioiter->domainhash);
        fio_400_assert(now() < domiter->expiration, domiter->name, "domain",
        "FIO Domain expired", ErrorDomainExpired);

        fio_400_assert(now() < fioiter->expiration, fio_address, "fio_address",
        "FIO Address expired", ErrorFioNameExpired);

        check(prod.active(), "producer does not have an active key");

         action(permission_level{get_self(), "active"_n},
           "fio.token"_n, "transfer"_n,
           make_tuple("fio.treasury"_n, name(bpiter->owner), asset(payout, symbol("FIO",9)),
           string("Paying producer from treasury."))
       ).send();


    // PAY FOUNDATION //
     auto fdtniter = fdtnrewards.begin();
     if (fdtniter->rewards > 100) { // 100 FIO = 100000000000 SUFs
         action(permission_level{get_self(), "active"_n},
               "fio.token"_n, "transfer"_n,
               make_tuple("fio.treasury"_n, FOUNDATIONACCOUNT, asset(fdtniter->rewards,symbol("FIO",9)),
               string("Paying foundation from treasury."))
             ).send();

       //Clear the foundation rewards counter

        clockstate.modify(clockiter, get_self(), [&](auto &entry) {
          entry.rewardspaid += payout;
        });

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

     json = {{"status",        "OK"},
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
          entry.fortyeightmonths = now() + 252480000; //time the treasury contract was spawned plus 252480000 blocks
        });

      }

      bucketrewards.emplace(get_self(), [&](auto &p) {
        p.rewards = 0;
      });

      bprewdupdate(0);

    }

    // @abi action
    [[eosio::action]]
    void bprewdupdate(const uint64_t &amount) {

      eosio_assert((has_auth(SystemContract) || has_auth("fio.token"_n)) || has_auth("fio.treasury"_n) || (has_auth("fio.reqobt"_n)),
        "missing required authority of fio.system, fio.treasury, fio.token, or fio.reqobt");

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
        "missing required authority of fio.system, fio.treasury, fio.token, or fio.reqobt");

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
        "missing required authority of fio.system, fio.token, fio.treasury or fio.reqobt");

        uint64_t size = std::distance(fdtnrewards.begin(),fdtnrewards.end());
        if (size == 0)  {
          fdtnrewards.emplace(_self, [&](struct fdtnreward& entry) {
            entry.rewards = amount;
         });

       } else {
         uint64_t reward = fdtnrewards.begin()->rewards;
         reward += amount;
         fdtnrewards.erase(fdtnrewards.begin());
         fdtnrewards.emplace(_self, [&](struct fdtnreward& entry) {
           entry.rewards = reward;
        });
       }

    }


    // @abi action
    [[eosio::action]]
    void fdtnrwdreset(const bool &paid) {

      eosio_assert((has_auth(SystemContract) || has_auth("fio.token"_n)) || has_auth("fio.treasury"_n) || (has_auth("fio.reqobt"_n)),
        "missing required authority of fio.system, fio.token, fio.treasury or fio.reqobt");

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
