/** FioError definitions file
 *  Description: Mapping of actions to account names for use in processing FIO transactions.
 *  @author Casey Gardiner
 *  @file actionmapping.hpp
 *  @copyright Dapix
 *
 *  Changes:
 */

#include <string>
#include <map>

#pragma once

namespace fioio {

    using namespace std;

    //map<string, string> ctType;
    vector<string> eosioActions;
    vector<string> fiosystemActions;
    vector<string> fioFinanceActions;

    static void Set_map(void){

        eosioActions.push_back("default");
        //ctType.insert(pair<string, string>("eosio")("default"));

        //ctType.insert(pair<string, string>("fio.system")("registername"));
        //ctType.insert(pair<string, string>("fio.system")("addaddress"));
        fiosystemActions.push_back("registername");
        fiosystemActions.push_back("addaddress");

        fioFinanceActions.push_back("requestfunds");
        //ctType.insert(pair<string, string>("fio.finance")("requestfunds"));


    }

    static string map_to_contract( string t ){

        if (find(eosioActions.begin(), eosioActions.end(), t) != eosioActions.end()){
            return "eosio";
        }
        if (find(fiosystemActions.begin(), fiosystemActions.end(), t) != fiosystemActions.end()){
            return "fio.system";
        }
        if (find(fioFinanceActions.begin(), fioFinanceActions.end(), t) != fioFinanceActions.end()){
            return "fio.finance";
        }
    }

    inline string returncontract(string incomingaction) {
        string contract;
        Set_map();

        contract = map_to_contract( incomingaction );

        return contract;
    }
}
