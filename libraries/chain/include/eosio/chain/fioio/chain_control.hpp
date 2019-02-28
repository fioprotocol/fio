/** chain_control definitions file
 *  Description: Sets up the supported chain data structure
 *  @author Casey Gardiner
 *  @file fio_name_validator.hpp
 *  @copyright Dapix
 *
 *  Changes:
 */

#include <string>
//#include <fio.common/json.hpp>
//#include <fstream>
#include <boost/algorithm/string.hpp>

#pragma once

namespace fioio {

    using namespace std;

    const std::string JSONFILE = "config/bip44chains.json";
    vector<std::string> chainList;

    inline string getChainFromIndex(int index) {
        string chainName = chainList[index];

        return chainName;
    }

    inline int getIndexFromChain( string chainname ){
        std::vector<string>::iterator it = std::find(chainList.begin(), chainList.end(), chainname);
        int index = std::distance(chainList.begin(), it);
        int result = -1;

        if(it != chainList.end()) {
            result = distance(chainList.begin(), it);
        } else {
            //ASSERT
        }

        return result;
    }

    inline void chainInit( void ){
        //std::ifstream ifs(JSONFILE.c_str());
        //nlohmann::json j = nlohmann::json::parse(ifs);

        //chainList = j.get<vector<string>>();
    }
}
