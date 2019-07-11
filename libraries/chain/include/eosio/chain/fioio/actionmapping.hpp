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
    vector <string> eosioActions;
    vector <string> fiosystemActions;
    vector <string> fioFinanceActions;
    vector <string> fioRequestObtActions;
    vector <string> fioTokenActions;

    static void Set_map(void) {
        //eosio actions
        eosioActions.push_back("default");

        //fio.system actions
        fiosystemActions.push_back("regaddress");
        fiosystemActions.push_back("regdomain");
        fiosystemActions.push_back("addaddress");
        fiosystemActions.push_back("renewdomain");
        fiosystemActions.push_back("renewaddress");
        fiosystemActions.push_back("expdomain");
        fiosystemActions.push_back("setdomainpub");
        fiosystemActions.push_back("expaddresses");
        fiosystemActions.push_back("bind2eosio");
        fiosystemActions.push_back("burnexpired");

        //fio.token actions
        fioTokenActions.push_back("trnsfiopubky");
        //fio.finance actions
        fioFinanceActions.push_back("requestfunds");

        //fio.request.obt actions
        fioRequestObtActions.push_back("recordsend");
        fioRequestObtActions.push_back("rejectfndreq");
        fioRequestObtActions.push_back("newfundsreq");
    }

    static string map_to_contract(string t) {
        if (find(fiosystemActions.begin(), fiosystemActions.end(), t) != fiosystemActions.end()) {
            return "fio.system";
        }
        if (find(fioFinanceActions.begin(), fioFinanceActions.end(), t) != fioFinanceActions.end()) {
            return "fio.finance";
        }
        if (find(fioRequestObtActions.begin(), fioRequestObtActions.end(), t) != fioRequestObtActions.end()) {
            return "fio.reqobt";
        }
        if (find(fioTokenActions.begin(), fioTokenActions.end(), t) != fioTokenActions.end()) {
            return "fio.token";
        }
        return "eosio";
    }

    inline string returncontract(string incomingaction) {
        Set_map();

        string contract = map_to_contract(incomingaction);

        return contract;
    }

    struct regaddress {
        string name;
        uint64_t requestor;
    };

    struct regdomain {
        string name;
        uint64_t requestor;
    };
}

FC_REFLECT(fioio::regaddress, (name)(requestor)
)
FC_REFLECT(fioio::regdomain, (name)(requestor)
)