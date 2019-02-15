/** chain_control definitions file
 *  Description: Sets up the supported chain data structure
 *  @author Casey Gardiner
 *  @file fio_name_validator.hpp
 *  @copyright Dapix
 *
 *  Changes:
 */

#include <string>
#include <boost/algorithm/string.hpp>

#pragma once

namespace fioio {

    using namespace std;

    struct chainpair {
        uint64_t index;
        string chainname;

        uint64_t primary_key() const { return index; }

        EOSLIB_SERIALIZE(chainpair, (index)(chainname))
    };

    typedef multi_index<N(chainList), chainpair> chaintable;

    inline chaintable chainInit( void ){

    }
