#include "fio.tpid.hpp"

namespace fioio {

  class [[eosio::contract("TPIDController")]]  TPIDController : public eosio::contract {


  private:
    tpids_table tpids;
    fionames_table fionames;

  public:
    using contract::contract;

      TPIDController(name s, name code, datastream<const char *> ds) : contract(s, code, ds), tpids(_self, _self.value), fionames(SystemContract, SystemContract.value) {
      }

  // @abi action
  [[eosio::action]]
  void createtpid(const string& tpid) {

    eosio_assert((has_auth(SystemContract) || has_auth("fio.token"_n)) || (has_auth("fio.reqobt"_n)),
      "missing required authority of fio.system, fio.token, or fio.reqobt");


    //see if FIO Address already exists before creating TPIDController

   uint64_t fioaddhash = string_to_uint64_hash(tpid.c_str());

   auto fionamefound = fionames.find(fioaddhash);
   fio_400_assert(fionamefound != fionames.end(), "TPID", tpid,
                  "Invalid TPID", InvalidTPID);

   auto tpidfound = tpids.find(fioaddhash);
       if (tpidfound == tpids.end()) {
         print("Creating new TPID.", "\n");
         tpids.emplace(_self, [&](struct tpid &f) {

           f.fioaddress  = tpid;
           f.fioaddhash = fioaddhash;
           f.rewards.amount = 0;
           f.rewards.symbol = symbol("FIO",9);
         });
       } else {
         print("TPID already exists.", "\n");
     } //end if fiofound

}//createtpid

   //@abi action
   [[eosio::action]]
   void updatetpid(const string& tpid, const uint64_t& amount) {

     eosio_assert((has_auth(SystemContract) || has_auth("fio.token"_n)) || (has_auth("fio.reqobt"_n)),
  "missing required authority of fio.system, fio.token, or fio.reqobt");


     uint64_t fioaddhash = string_to_uint64_hash(tpid.c_str());
     auto fionamefound = fionames.find(fioaddhash);
     if (fionamefound != fionames.end()) {
       auto tpidfound = tpids.find(fioaddhash);
       if (tpidfound == tpids.end()) {
         print("TPID does not exist. Creating TPID.", "\n");
         createtpid(tpid);
         updatetpid(tpid, amount);
       }
       else {
         print("Updating TPID.", "\n");
         tpids.modify(tpidfound, _self, [&](struct tpid &f) {
           f.rewards.amount += amount;
         });
       }
    } else {
      print("Cannot register TPID, FIO Address not found. The transaction will continue without TPID payment.","\n");
    }
  } //updatetpid

  //@abi action
  [[eosio::action]]
  void rewardspaid(const string& tpid) {
    require_auth("fio.treasury"_n); //This action can only be called by fio.treasury after successful rewards payment to tpid
    uint64_t fioaddhash = string_to_uint64_hash(tpid.c_str());
    auto tpidfound = tpids.find(fioaddhash);

    if (tpidfound != tpids.end()) {
      tpids.modify(tpidfound, _self, [&](struct tpid &f) {
        f.rewards.amount = 0;
      });
    }

  }


  }; //class TPIDController


  EOSIO_DISPATCH(TPIDController, (createtpid)(updatetpid)(rewardspaid))
}
