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
    vector<string> fioAddressActions;
    vector<string> fioFinanceActions;
    vector<string> fioFeeActions;
    vector<string> fioRequestObtActions;
    vector<string> whitelistActions;
    vector<string> fioTokenActions;
    vector<string> fioTreasuryActions;

    static void Set_map(void) {
        //eosio actions
        eosioActions.push_back("default");
        //whitelist actions
        whitelistActions.push_back("addwhitelist");
        whitelistActions.push_back("remwhitelist");

        //fio.system actions
        fioAddressActions.push_back("regaddress");
        fioAddressActions.push_back("regdomain");
        fioAddressActions.push_back("addaddress");
        fioAddressActions.push_back("renewdomain");
        fioAddressActions.push_back("renewaddress");
        fioAddressActions.push_back("expdomain");
        fioAddressActions.push_back("setdomainpub");
        fioAddressActions.push_back("expaddresses");
        fioAddressActions.push_back("bind2eosio");
        fioAddressActions.push_back("burnexpired");

        //fio.fee actions
        fioFeeActions.push_back("setfeemult");
        fioFeeActions.push_back("bundlevote");
        fioFeeActions.push_back("setfeevote");

        fioTreasuryActions.push_back("tpidclaim");
        fioTreasuryActions.push_back("bpclaim");

        //fio.token actions
        fioTokenActions.push_back("trnsfiopubky");
        //fio.finance actions
        fioFinanceActions.push_back("requestfunds");

        //fio.request.obt actions
        fioRequestObtActions.push_back("recordobt");
        fioRequestObtActions.push_back("rejectfndreq");
        fioRequestObtActions.push_back("newfundsreq");
    }

    static string map_to_contract(string t) {
        if (find(fioAddressActions.begin(), fioAddressActions.end(), t) != fioAddressActions.end()) {
            return "fio.address";
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
        if (find(fioTreasuryActions.begin(), fioTreasuryActions.end(), t) != fioTreasuryActions.end()) {
            return "fio.treasury";
        }
        if (find(whitelistActions.begin(), whitelistActions.end(), t) != whitelistActions.end()) {
            return "fio.whitelst";
        }
        if (find(fioFeeActions.begin(), fioFeeActions.end(), t) != fioFeeActions.end()) {
            return "fio.fee";
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
