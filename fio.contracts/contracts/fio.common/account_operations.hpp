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
        vector <eosiosystem::key_weight> keys;
        vector <eosiosystem::permission_level_weight> accounts;
        vector <wait_weight> waits;
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

/** All alphanumeric characters except for "0", "I", "O", and "l" */

    static const char *pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
    static const int8_t mapBase58[256] = {
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, -1, -1, -1, -1, -1, -1,
            -1, 9, 10, 11, 12, 13, 14, 15, 16, -1, 17, 18, 19, 20, 21, -1,
            22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, -1, -1, -1, -1, -1,
            -1, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, -1, 44, 45, 46,
            47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    };

    bool DecodeBase58(const char *psz, std::vector<unsigned char> &vch) {
        // Skip leading spaces.
        while (*psz && isspace(*psz))
            psz++;
        // Skip and count leading '1's.
        int zeroes = 0;
        int length = 0;
        while (*psz == '1') {
            zeroes++;
            psz++;
        }
        // Allocate enough space in big-endian base256 representation.
        int size = strlen(psz) * 733 / 1000 + 1; // log(58) / log(256), rounded up.
        std::vector<unsigned char> b256(size);
        // Process the characters.
        static_assert(sizeof(mapBase58) / sizeof(mapBase58[0]) == 256,
                      "mapBase58.size() should be 256"); // guarantee not out of range
        while (*psz && !isspace(*psz)) {
            // Decode base58 character
            int carry = mapBase58[(uint8_t) * psz];
            if (carry == -1)  // Invalid b58 character
                return false;
            int i = 0;
            for (std::vector<unsigned char>::reverse_iterator it = b256.rbegin();
                 (carry != 0 || i < length) && (it != b256.rend()); ++it, ++i) {
                carry += 58 * (*it);
                *it = carry % 256;
                carry /= 256;
            }
            assert(carry == 0);
            length = i;
            psz++;
        }
        // Skip trailing spaces.
        while (isspace(*psz))
            psz++;
        if (*psz != 0)
            return false;
        // Skip leading zeroes in b256.
        std::vector<unsigned char>::iterator it = b256.begin() + (size - length);
        while (it != b256.end() && *it == 0)
            it++;
        // Copy result into output vector.
        vch.reserve(zeroes + (b256.end() - it));
        vch.assign(zeroes, 0x00);
        while (it != b256.end())
            vch.push_back(*(it++));
        return true;
    }

    bool decode_base58(const string &str, vector<unsigned char> &vch) {
        return DecodeBase58(str.c_str(), vch);
    }

//NOTE -- this needs revised to make this to be the real value.
    asset rambytes_price(uint32_t bytes) {

        return asset(10000, symbol("FIO", 9));
    }

}
