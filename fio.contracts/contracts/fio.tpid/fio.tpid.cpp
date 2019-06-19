#include "fio.tpid.hpp"

namespace fioio {

  class [[eosio::contract("TPIDController")]]  TPIDController : public eosio::contract {


  private:
    tpids_table tpids;


  public:
    using contract::contract;

      TPIDController(name s, name code, datastream<const char *> ds) : contract(s, code, ds), tpids(_self, _self.value) {
      }

  // @abi action
  [[eosio::action]]
  void createtpid(const string& tpid, const name& actor) {

    require_auth(actor);

    //see if TPID already exists (come back to this after emplacing a hash that can be tested)
     uint64_t fioaddhash = string_to_uint64_hash(tpid.c_str());

     auto tpidfound = tpids.find(fioaddhash);
     if (tpidfound == tpids.end()) {
       print("Registering new TPID.", "\n");
       tpids.emplace(_self, [&](struct tpid &f) {

         f.fioaddress  = tpid;
         f.fioaddhash = fioaddhash;
         f.rewards.amount = 0;
         f.rewards.symbol = symbol("FIO",9);
       });


     }
   } //createtpid


   //@abi action
   [[eosio::action]]
   void updatetpid(const string& tpid, const name& actor, const asset& amount) {

     require_auth(actor);

     uint64_t fioaddhash = string_to_uint64_hash(tpid.c_str());

     auto tpidfound = tpids.find(fioaddhash);
     if (tpidfound == tpids.end()) {
       print("Updating TPID.", "\n");
       tpids.modify(tpidfound, _self, [&](struct tpid &f) {

         f.rewards.amount += amount.amount;

       });
   }
  } //updatetpid

  }; //class TPIDController


  EOSIO_DISPATCH(TPIDController, (createtpid)(updatetpid))
}
