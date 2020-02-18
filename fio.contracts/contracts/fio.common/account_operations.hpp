/** Account Operations implementation file
 *  Description: This assists FIO is creating unique account names.
 *  @author Casey Gardiner
 *  @modifedby
 *  @file account_operations.hpp
 *  @license FIO Foundation ( https://github.com/fioprotocol/fio/blob/master/LICENSE ) Dapix
 */

#include <algorithm>
#include <cmath>
#include "../fio.system/include/fio.system/fio.system.hpp"
#include <eosiolib/eosio.hpp>
#include <string>
#include <eosiolib/asset.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/symbol.hpp>
#include <eosiolib/crypto.h>
#include <cstring>

using namespace eosio;
using namespace std;
using weight_type = uint16_t;

#include "abieos_numeric.hpp"

namespace fioio {

// Temporary authority until native is fixed. Ref: https://github.com/EOSIO/eos/issues/4669

    struct signup_public_key {
        uint8_t type;
        array<unsigned char, 33> data;
    };

    struct wait_weight {
        uint32_t wait_sec;
        weight_type weight;
    };

    struct authority {
        uint32_t threshold;
        vector<eosiosystem::key_weight> keys;
        vector<eosiosystem::permission_level_weight> accounts;
        vector<wait_weight> waits;
    };

    struct newaccount {
        name creator;
        name name;
        authority owner;
        authority active;
    };

// eosiosystem::native::newaccount Doesn't seem to want to take authorities.
    struct call {
        struct eosio {
            void newaccount(name creator, name name,
                            authority owner, authority active);
        };
    };


}
