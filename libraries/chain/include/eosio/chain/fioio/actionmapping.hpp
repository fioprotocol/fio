/** FioError definitions file
 *  Description: Mapping of actions to account names for use in processing FIO transactions.
 *  @author Casey Gardiner
 *  @file actionmapping.hpp
 *  @copyright Dapix
 *
 *  Changes:
 */

#pragma once

#include <string>
#include <fc/reflect/reflect.hpp>

namespace fioio {

    using namespace std;

    //map<string, string> ctType;
    vector<string> eosioActions;
    vector<string> fiosystemActions;
    vector<string> fioFinanceActions;
    vector<string> fioRequestObtActions;
    vector<string> fioTokenActions;

    static void Set_map(void){
        //eosio actions
        eosioActions.push_back("default");

        //fio.system actions
        fiosystemActions.push_back("registername");
        fiosystemActions.push_back("addaddress");
        fiosystemActions.push_back("bind2eosio");
        //fio.token actions
        fioTokenActions.push_back("transferfio");
        //fio.finance actions
        fioFinanceActions.push_back("requestfunds");

        //fio.request.obt actions
        fioRequestObtActions.push_back("recordsend");
        fioRequestObtActions.push_back("rejectfndreq");
        fioRequestObtActions.push_back("newfundsreq");
    }

    static string map_to_contract( string t ){
        if (find(fiosystemActions.begin(), fiosystemActions.end(), t) != fiosystemActions.end()){
            return "fio.system";
        }
        if (find(fioFinanceActions.begin(), fioFinanceActions.end(), t) != fioFinanceActions.end()){
            return "fio.finance";
        }
        if (find(fioRequestObtActions.begin(), fioRequestObtActions.end(), t) != fioRequestObtActions.end()){
            return "fio.reqobt";
        }
        if (find(fioTokenActions.begin(), fioTokenActions.end(), t) != fioTokenActions.end()) {
            return "fio.token";
        }
        return "eosio";
    }

    inline string returncontract(string incomingaction) {
        Set_map();

        string contract = map_to_contract( incomingaction );

        return contract;
    }

   struct registername {
      string name;
      uint64_t requestor;
   };

    //struct addaddress {
    //    string fioaddress;
    //    string tokencode;
    //    string pubaddress;
    //    uint64_t actor;
    //};
}

FC_REFLECT( fioio::registername, (name)(requestor) )
//FC_REFLECT( fioio::addaddress, (fioaddress)(tokencode)(pubaddress)(actor) )
