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
  void createtpid(const string& tpid, const name& actor) {

    require_auth(actor);

    //see if FIO Address already exists before creating TPIDController

   uint64_t fioaddhash = string_to_uint64_hash(tpid.c_str());

   auto tpidfound = tpids.find(fioaddhash);
   auto fionamefound = fionames.find(fioaddhash);

     fio_400_assert(fionamefound != fionames.end(), "TPID", tpid,
                    "Invalid TPID", InvalidTPID);

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
   void updatetpid(const string& tpid, const name& actor, const uint64_t& amount) {

     require_auth(actor);

     uint64_t fioaddhash = string_to_uint64_hash(tpid.c_str());

     auto tpidfound = tpids.find(fioaddhash);
     if (tpidfound != tpids.end()) {
       print("Updating TPID.", "\n");
       tpids.modify(tpidfound, _self, [&](struct tpid &f) {

         f.rewards.amount += amount;

       });
   }
   else {
      print("TPID does not exist. Please call createtpid action.", "\n");
    }
  } //updatetpid

  }; //class TPIDController


  EOSIO_DISPATCH(TPIDController, (createtpid)(updatetpid))
}
