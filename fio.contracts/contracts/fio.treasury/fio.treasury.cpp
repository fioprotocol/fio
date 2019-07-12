#include "fio.treasury.hpp"

namespace fioio {

  class [[eosio::contract("FIOTreasury")]]  FIOTreasury : public eosio::contract {


    private:
      tpids_table tpids;
      fionames_table fionames;
      treasury_table clockstate;
      uint64_t lastpayout;

    public:
      using contract::contract;


        FIOTreasury(name s, name code, datastream<const char *> ds) : contract(s, code, ds),
                                                                      tpids(TPIDContract, TPIDContract.value),
                                                                      fionames(SystemContract, SystemContract.value),
                                                                      clockstate(_self, _self.value) {
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
       if( now() > clockiter->lastpayout + 60 ) {

          for(auto &itr : tpids) {

            if (itr.rewards >= 100000000000)  {  //100 FIO (100,000,000,000 SUF)

               print(itr.fioaddress, " has ",itr.rewards ," rewards.\n");

               auto itrfio = fionames.find(string_to_uint64_hash(itr.fioaddress.c_str()));

                    action(permission_level{get_self(), "active"_n},
                          "fio.token"_n, "transfer"_n,
                          make_tuple("fio.treasury"_n, name(itrfio->owner_account), asset(itr.rewards,symbol("FIO",9)),
                          string("Paying TPID from treasury."))
                   ).send();

                   action(permission_level{get_self(), "active"_n},
                          "fio.tpid"_n, "rewardspaid"_n,
                          make_tuple(itr.fioaddress)
                   ).send();

              } // endif

              tpids_paid++;
          } // for tpids.begin() tpids.end()

          //update the clock but only if there has been a tpid paid out.
          if(tpids_paid > 0) {
            action(permission_level{get_self(), "active"_n},
                   get_self(), "updateclock"_n,
                   make_tuple()
            ).send();
        }

      } //end if lastpayout < now() 30

       nlohmann::json json = {{"status",        "OK"},
                              {"tpids_paid",    tpids_paid}};
       send_response(json.dump().c_str());

    } //tpid_claim


    // @abi action
    [[eosio::action]]
    void updateclock() {
      require_auth("fio.treasury"_n);

      auto clockiter = clockstate.begin();

      clockstate.erase(clockiter);

      clockstate.emplace(_self, [&](treasurystate& entry) {
      entry.lastpayout = now();
    });
    }

    // @abi action
    [[eosio::action]]
    void startclock() {
      require_auth("fio.treasury"_n);

      unsigned int size = std::distance(clockstate.begin(),clockstate.end());
      if (size == 0)  {
          clockstate.emplace(_self, [&](treasurystate& entry) {
          entry.lastpayout = now();
        });
      }
    }

  }; //class TPIDController




  EOSIO_DISPATCH(FIOTreasury, (tpidclaim)(updateclock)(startclock))
}
