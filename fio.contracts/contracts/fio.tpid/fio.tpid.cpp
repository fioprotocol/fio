#include "fio.tpid.hpp"

namespace fioio {

  class [[eosio::contract("TPIDController")]]  TPIDController : public eosio::contract {


  private:
    tpids_table tpids;
    fionames_table fionames;
    eosiosystem::voters_table voters;
    bounties_table bounties;

  public:
    using contract::contract;

      TPIDController(name s, name code, datastream<const char *> ds) :
      contract(s, code, ds), tpids(_self, _self.value),  bounties(_self, _self.value), fionames(SystemContract, SystemContract.value),
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


      //@abi action
      [[eosio::action]]
      void updatetpid(const string& tpid,  const name owner, const uint64_t& amount) {

          print ("calling updatetpid with tpid ",tpid," owner ",owner," amount ", amount,"\n");

          uint64_t fioaddhash = string_to_uint64_hash(tpid.c_str());
          auto fionamefound = fionames.find(fioaddhash);
          print("\nfionamefound: ",fionamefound->name,"\n");
          if (fionamefound != fionames.end()) {
              auto tpidfound = tpids.find(fioaddhash);
              print("\ntpidfound: ",tpidfound->fioaddress,"\n");
              if (tpidfound == tpids.end()) {
                  print("TPID does not exist. Creating TPID.", "\n");
                  tpids.emplace(get_self(), [&](struct tpid &f) {
                      f.fioaddress  = tpid;
                      f.fioaddhash = fioaddhash;
                      f.rewards = 0;
                  });

                  process_auto_proxy(tpid,owner);
              }
              if(std::distance(tpids.begin(), tpids.end()) > 0) {
                tpidfound  = tpids.find(fioaddhash);
                print("Updating TPID.", tpidfound->fioaddress,"\n");
                tpids.modify(tpidfound, get_self(), [&](struct tpid &f) {
                    f.rewards += amount;
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
        f.rewards = 0;
      });
    }

  }


  //Must be called at least once at genesis for tokensminted check in fio.rewards.hpp
      //@abi action
  [[eosio::action]]
  void updatebounty(const uint64_t &amount) {
    eosio_assert((has_auth("fio.tpid"_n) || has_auth("fio.treasury"_n)),
                 "missing required authority of fio.tpid, or fio.treasury");

    if(std::distance(bounties.begin(), bounties.end()) == 0)
    {
      bounties.emplace(get_self(), [&](struct bounty &b) {
        b.tokensminted = 0;
      });
    }
    else {
      uint64_t temp = bounties.begin()->tokensminted;
      bounties.erase(bounties.begin());
      bounties.emplace(get_self(), [&](struct bounty &b) {
        b.tokensminted = temp + amount;
      });
    }
  }

}; //class TPIDController

  EOSIO_DISPATCH(TPIDController, (updatetpid)(rewardspaid)(updatebounty))
}
