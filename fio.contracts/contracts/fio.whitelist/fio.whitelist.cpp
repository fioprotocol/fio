/** FioTreasury implementation file
 *  Description: FioTreasury smart contract controls block producer and tpid payments.
 *  @author Adam Androulidakis
 *  @modifedby
 *  @file fio.treasury.cpp
 *  @copyright Dapix
 */

#include <fio.fee/fio.fee.hpp>
#include "fio.whitelist.hpp"

namespace fioio {

  class [[eosio::contract("FIOWhitelist")]]  FIOWhitelist : public eosio::contract {


    private:

      whitelist_table whitelist;
      fiofee_table fiofees;



    public:
      using contract::contract;


        FIOWhitelist(name s, name code, datastream<const char *> ds) : contract(s, code, ds),
                                                                       whitelist(_self, _self.value),
                                                                       fiofees(FeeContract, FeeContract.value){
        }


      // @abi action
      [[eosio::action]]
      void addwhitelist(const string &fio_public_key_hash,
                        const string &content,
                        uint64_t max_fee,
                        const string &tpid,
                        const name& actor) {

      require_auth(actor);

          print("EDEDEDEDED addwhitelist.");

       nlohmann::json json = {{"status",        "OK"}};
       send_response(json.dump().c_str());

    }

       // @abi action
      [[eosio::action]]
      void remwhitelist(const string &fio_public_key_hash,
                        uint64_t max_fee,
                        const string &tpid,
                        const name& actor) {

          require_auth(actor);

          print("EDEDEDEDED remwhitelist.");

          nlohmann::json json = {{"status",        "OK"}};
          send_response(json.dump().c_str());

      }



  };



  EOSIO_DISPATCH(FIOWhitelist, (addwhitelist)(remwhitelist))
}
