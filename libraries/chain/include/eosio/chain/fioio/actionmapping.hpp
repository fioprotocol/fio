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


    static void Set_map(void){
        //eosio actions
        eosioActions.push_back("default");

        //fio.system actions
        fiosystemActions.push_back("registername");
        fiosystemActions.push_back("addaddress");

        //fio.finance actions
        fioFinanceActions.push_back("requestfunds");

        //fio.request.obt actions
        fioRequestObtActions.push_back("recordsend");
    }

    static string map_to_contract( string t ){
        if (find(fiosystemActions.begin(), fiosystemActions.end(), t) != fiosystemActions.end()){
            return "fio.system";
        }
        if (find(fioFinanceActions.begin(), fioFinanceActions.end(), t) != fioFinanceActions.end()){
            return "fio.finance";
        }
        if (find(fioRequestObtActions.begin(), fioRequestObtActions.end(), t) != fioRequestObtActions.end()){
            return "fio.request.obt";
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

    struct addaddress {
        string fioaddress;
        string tokencode;
        string pubaddress;
        uint64_t actor;
    };
}

FC_REFLECT( fioio::registername, (name)(requestor) )
