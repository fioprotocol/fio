/** chain_control definitions file
 *  Description: Sets up the supported chain data structure
 *  @author Casey Gardiner
 *  @file fio_name_validator.hpp
 *  @copyright Dapix
 *
 *  Changes:
 */
#pragma once

#include <string>

namespace fioio {

    using namespace std;

    const string JSONFILE = "config/bip44chains.json";
    vector<string> chainList;

    inline string getChainFromIndex(int index) {
        string chainName = chainList[index];

        return chainName;
    }

    inline int getIndexFromChain( string chainname ){
        vector<string>::iterator it = find(chainList.begin(), chainList.end(), chainname);
        int index = distance(chainList.begin(), it);
        int result = -1;

        if(it != chainList.end()) {
            result = distance(chainList.begin(), it);
        } else {
            //ASSERT
        }

        return result;
    }

    inline void chainInit( void ){
        try {
            fc::json::from_file(JSONFILE).as<vector<string>>(chainList);
            /// do what you will with the chain list
        } catch (...) {
            fc::elog ("failed to read ${f}",("f",JSONFILE));
        }
    }
}
