/** chain_control definitions file
 *  Description: Sets up the supported chain data structure
 *  @author Casey Gardiner
 *  @file fio_name_validator.hpp
 *  @copyright Dapix
 *
 *  Changes:
 */

#include <string>
#include <fio.common/json.hpp>
//#include <fstream>
#include <boost/algorithm/string.hpp>

#pragma once

namespace fioio {

    using namespace std;

    struct chainpair {
        uint64_t index;
        string chain;

        uint64_t primary_key() const { return index; }

        EOSLIB_SERIALIZE(chainpair, (index)(chain))
    };

    const std::string JSONFILE = "config/bip44chains.json";
    vector<chainpair> chainList;

    inline chainpair chainInit(string index) {
        chainpair na;

        na.index = 0;
        na.chain = index;

        //std::ifstream ifs(JSONFILE.c_str());
        //nlohmann::json j = nlohmann::json::parse(ifs);

        return na;
    }

    inline int chainReturn( string chainname ){

        return 0;
    }
}
