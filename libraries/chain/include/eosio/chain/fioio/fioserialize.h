/** fio serialize definitions file
 *  Description: This account will help the chain plugin and contracts to serialize fioaddresses into hashed values.
 *  @author Casey Gardiner / Ed Rotthoff
 *  @file fioserialize.hpp
 *  @license FIO Foundation ( https://github.com/fioprotocol/fio/blob/master/LICENSE )
 *
 */

#include <string>
#include <eosio/chain/name.hpp>
#include <fc/crypto/sha1.hpp>

#pragma once

namespace fioio {
    using uint128_t           = unsigned __int128;

    // Converts a string to a uint64_t using the same hashing mechanism as string_to_name without using the base32 symbols
    static uint128_t string_to_uint128_t(const char *str) {
        uint128_t retval = 0;
        uint32_t len = 0;

        if( str != nullptr ){
            while (str[len]) ++len;

            fc::sha1 res = fc::sha1::hash(str, len);
            memcpy(&retval, &res, sizeof(retval));
        }
        return retval;
    }

    //use this to see the value of your uint128_t, this will match what shows in get table.
    static std::string to_hex_little_endian(const char *d, uint32_t s) {
        std::string r;

        if( d != nullptr ){
            const char *to_hexi = "0123456789abcdef";
            uint8_t *c = (uint8_t *) d;
            for (int i = (s - 1); i >= 0; --i) {
                (r += to_hexi[(c[i] >> 4)]) += to_hexi[(c[i] & 0x0f)];
            }
        }
        return r;
    }
}