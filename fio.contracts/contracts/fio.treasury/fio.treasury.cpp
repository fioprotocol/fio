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
      eosiosystem::producers_table producers;

      uint64_t lasttpidpayout;

    public:
      using contract::contract;


        FIOTreasury(name s, name code, datastream<const char *> ds) : contract(s, code, ds),
                                                                      tpids(TPIDContract, TPIDContract.value),
                                                                      fionames(SystemContract, SystemContract.value),
                                                                      bprewards(_self, _self.value),
                                                                      clockstate(_self, _self.value),
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
    void bpclaim(const name& actor) {

    require_auth(actor);

    uint64_t bps_paid = 0;
    //If the contract has not been invoked yet, this will execute and set the initial block time
    auto clockiter = clockstate.begin();

    //This contract should only be able to iterate throughout the entire tpids table to
    //to check for rewards once every x blocks.
     if( now() > clockiter->lastbppayout + 172800 ) {



      for(auto &itr : producers) {

        print("Test output","\n");


        bps_paid++;
        if (bps_paid >= 100) break;
      }

    }



        nlohmann::json json = {{"status",        "OK"},
                               {"tpids_paid",    bps_paid}};
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
          entry.lastbppayout = now() - 172780;
        });

      }



      bprewdupdate(0);

    }

    // @abi action
    [[eosio::action]]
    void bprewdupdate(uint64_t amount) {

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
    void bppoolupdate(uint64_t amount) {

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
    void fdtnrwdupdat(uint64_t amount) {

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



  EOSIO_DISPATCH(FIOTreasury, (tpidclaim)(updateclock)(startclock)(bprewdupdate)(fdtnrwdupdat)(bppoolupdate)(maintain))
}
