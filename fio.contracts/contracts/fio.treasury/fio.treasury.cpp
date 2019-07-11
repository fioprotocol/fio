#include "fio.treasury.hpp"

namespace fioio {

  class [[eosio::contract("FIOTreasury")]]  FIOTreasury : public eosio::contract {


  private:
    tpids_table tpids;
    fionames_table fionames;
    uint64_t lastrun;
  public:
    using contract::contract;


      FIOTreasury(name s, name code, datastream<const char *> ds) : contract(s, code, ds),
                                                                    tpids(TPIDContract, TPIDContract.value),
                                                                fionames(SystemContract, SystemContract.value) {
      }

  // @abi action
  [[eosio::action]]
  void tpidclaim(const name& actor) {


  require_auth(actor);

  unsigned long long tpids_paid = 0;


    for(auto &itr : tpids){
      if (itr.rewards.amount >= 100000000)  {  //100 FIO (100,000,000,000 SUF)
         print(itr.fioaddress, " has ",itr.rewards.amount ," rewards.\n");

         auto itrfio = fionames.find(string_to_uint64_hash(itr.fioaddress.c_str()));

              action(permission_level{get_self(), "active"_n},
                    "fio.token"_n, "transfer"_n,
                    make_tuple("fio.treasury"_n, name(itrfio->owner_account), itr.rewards,
                    string("Paying TPID from treasury."))
             ).send();

             print("Owner account located: ", name(itrfio->owner_account),"\n");

            action(permission_level{get_self(), "active"_n},
                    "fio.tpid"_n, "rewardspaid"_n,
                    make_tuple(itr.fioaddress)
             ).send();

             tpids_paid++;
        } // endif

    } // for tpids.begin() tpids.end()

     nlohmann::json json = {{"status",        "OK"},
                            {"tpids_paid",    tpids_paid}};
     send_response(json.dump().c_str());

   } //tpid_claim

}; //class TPIDController

  EOSIO_DISPATCH(FIOTreasury, (tpidclaim))
}
