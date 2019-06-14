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
    void regtpid(const string& tpid, const name& actor) {

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
       else {
         //tpids.modify()

       }
      //check fioaddress table, actor must be owner of the fioaddress to continue
      //auto other = fioaddress.find(actor.value);



    }

    [[eosio::action]]
    void unregtpid(const std::string tpid, const name& actor) {

    }



  }; //class TPIDController


  EOSIO_DISPATCH(TPIDController, (regtpid)(unregtpid))
}
