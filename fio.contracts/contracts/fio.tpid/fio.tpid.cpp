#include "fio.tpid.hpp"

namespace fioio {

  class [[eosio::contract("TPIDController")]]  TPIDController : public eosio::contract {


  private:
    tpids_table tpids;
    fionames_table fionames;
    eosiosystem::voters_table voters;

  public:
    using contract::contract;

      TPIDController(name s, name code, datastream<const char *> ds) :
      contract(s, code, ds), tpids(_self, _self.value), fionames(SystemContract, SystemContract.value),
      voters(SystemContract, SystemContract.value){
      }

      // this action will perform the logic of checking the voter_info,
      // and setting the proxy and auto proxy for auto proxy.
      inline void autoproxy(name proxy_name, name owner_name )
      {

          print(" called autoproxy with proxy_nme ", proxy_name, " owner_name ",owner_name,"\n");
          //check the voter_info table for a record matching owner_name.
          auto viter = voters.find(owner_name.value);
          if (viter == voters.end())
          {

                  print(" autoproxy voter info not found, calling crautoproxy", "\n");
                  //if the record is not there then send inline action to crautoprx (a new action in the system contract).
                  //note this action will set the auto proxy and is_aut_proxy, so return after.
                  INLINE_ACTION_SENDER(eosiosystem::system_contract, crautoproxy)(
                          "eosio"_n, {{get_self(), "active"_n}},
                          {proxy_name, owner_name});

              return;
          }
          else
          {
              //check if the record has auto proxy and proxy matching proxy_name, set has_proxy. if so return.
              if (viter->is_auto_proxy)
              {
                  if (proxy_name == viter->proxy) {
                      return;
                  }
              }
              else if ((viter->proxy) || (viter->producers.size() > 0) || viter->is_proxy)
              {
                  //check if the record has another proxy or producers. if so return.
                  return;
              }

              //invoke the fio.system contract action to set auto proxy and proxy name.
              action(
                      permission_level{get_self(), "active"_n},
                      "eosio"_n,
                      "setautoproxy"_n,
                      std::make_tuple(proxy_name, owner_name)
              ).send();
          }
      }

      inline void process_auto_proxy(const string &tpid, name owner) {

          uint64_t hashname = string_to_uint64_hash(tpid.c_str());
          print("process tpid hash of name ", tpid, " is value ", hashname ,"\n");

          auto iter = tpids.find(hashname);
          if (iter == tpids.end()){
              print("process tpid, tpid not found ","\n");
              //tpid does not exist. do nothing.
          }
          else{
              print("process tpid, found a tpid ","\n");
              //tpid exists, use the info to find the owner of the tpid
              auto iternm = fionames.find(iter->fioaddhash);
              if (iternm != fionames.end()) {
                  print("process found the fioname associated with the TPID in the fionames ","\n");
                  name proxy_name = name(iternm->owner_account);
                  //do the auto proxy
                  autoproxy(proxy_name,owner);
              }
          }
      }

      // @abi action
      [[eosio::action]]
      void createtpid(const string& tpid, name owner) {

          eosio_assert((has_auth(SystemContract) || has_auth("fio.token"_n)) || (has_auth("fio.reqobt"_n)),
                       "missing required authority of fio.system, fio.token, or fio.reqobt");


          //see if FIO Address already exists before creating TPIDController

          uint64_t fioaddhash = string_to_uint64_hash(tpid.c_str());

          auto fionamefound = fionames.find(fioaddhash);
          fio_400_assert(fionamefound != fionames.end(), "TPID", tpid,
                         "Invalid TPID", InvalidTPID);
          //get the account owner, register the proxy in the voters table.
          name proxy_name = name(fionamefound->owner_account);
          //call register proxy on the system contract
          print ("calling regproxy for name ",proxy_name);
          INLINE_ACTION_SENDER(eosiosystem::system_contract, regproxy)(
                  "eosio"_n, {{get_self(), "active"_n}},
                  {proxy_name, true} );


          auto tpidfound = tpids.find(fioaddhash);
          if (tpidfound == tpids.end()) {
              print("Creating new TPID.", "\n");
              tpids.emplace(_self, [&](struct tpid &f) {

                  f.fioaddress  = tpid;
                  f.fioaddhash = fioaddhash;
                  f.rewards = 0;
              });
          } else {
              print("TPID already exists.", "\n");
          } //end if fiofound
          process_auto_proxy(tpid,owner);

      }//createtpid




      //@abi action
      [[eosio::action]]
      void updatetpid(const string& tpid,  const name owner, const uint64_t& amount) {

          print ("calling updtpid with tpid ",tpid," owner ",owner," amount ", amount,"\n");

          uint64_t fioaddhash = string_to_uint64_hash(tpid.c_str());
          auto fionamefound = fionames.find(fioaddhash);

          if (fionamefound != fionames.end()) {
              auto tpidfound = tpids.find(fioaddhash);
              if (tpidfound == tpids.end()) {
                  print("TPID does not exist. Creating TPID.", "\n");
                  createtpid(tpid,owner);
                  updatetpid(tpid,owner, amount);
              }
              else {
                  print("Updating TPID.", "\n");
                  tpids.modify(tpidfound, _self, [&](struct tpid &f) {
                      f.rewards += amount;
                  });
              }
          } else {
              print("Cannot register TPID, FIO Address not found. The transaction will continue without TPID payment.","\n");
          }
      } //updtpid


      //@abi action
  [[eosio::action]]
  void rewardspaid(const string& tpid) {
    require_auth("fio.treasury"_n); //This action can only be called by fio.treasury after successful rewards payment to tpid
    uint64_t fioaddhash = string_to_uint64_hash(tpid.c_str());
    auto tpidfound = tpids.find(fioaddhash);

    if (tpidfound != tpids.end()) {
      tpids.modify(tpidfound, _self, [&](struct tpid &f) {
        f.rewards = 0;
      });
    }

  }


  }; //class TPIDController


  EOSIO_DISPATCH(TPIDController, (createtpid)(updatetpid)(rewardspaid))
}
