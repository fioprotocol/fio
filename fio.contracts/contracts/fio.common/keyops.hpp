/** keyops definitions file
 *  Description: Takes in a pubkey and turns it into a hash which is exported to new_account
 *  @author Casey Gardiner
 *  @file keyops.hpp
 *  @license FIO Foundation ( https://github.com/fioprotocol/fio/blob/master/LICENSE ) Dapix
 *
 *  Changes: Adam Androulidakis 2-20-2019 - Changed hashing method
 *
 */


#pragma once

#include <string>

using namespace eosio;

namespace fioio {

    static uint32_t acctcap = 12;

    const char *const ALPHABET =
            "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
    const char ALPHABET_MAP[128] = {
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, -1, -1, -1, -1, -1, -1,
            -1, 9, 10, 11, 12, 13, 14, 15, 16, -1, 17, 18, 19, 20, 21, -1,
            22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, -1, -1, -1, -1, -1,
            -1, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, -1, 44, 45, 46,
            47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, -1, -1, -1, -1, -1
    };

// result must be declared (for the worst case): char result[len * 2];
    static int DecodeBase58(
            const char *str, int len, unsigned char *result) {
        int resultlen = 1;
        if( str != nullptr ){
            result[0] = 0;
            for (int i = 0; i < len; i++) {
                unsigned int carry = (unsigned int) ALPHABET_MAP[(unsigned char) str[i]];
                for (int j = 0; j < resultlen; j++) {
                    carry += (unsigned int) (result[j]) * 58;
                    result[j] = (unsigned char) (carry & 0xff);
                    carry >>= 8;
                }
                while (carry > 0) {
                    result[resultlen++] = (unsigned int) (carry & 0xff);
                    carry >>= 8;
                }
            }

            for (int i = 0; i < len && str[i] == '1'; i++)
                result[resultlen++] = 0;

            // Poorly coded, but guaranteed to work.
            for (int i = resultlen - 1, z = (resultlen >> 1) + (resultlen & 1);
                 i >= z; i--) {
                int k = result[i];
                result[i] = result[resultlen - i - 1];
                result[resultlen - i - 1] = k;
            }
        }
        return resultlen;
    }

    static uint64_t shorten_key(unsigned char *key) {
        uint64_t res = 0;

        int done = 0;
        int i = 1;  // Ignore key head
        int len = 0;
        while (len <= 12) {
            assert(i < 33); // Means the key has > 20 bytes with trailing zeroes...

            auto trimmed_char = uint64_t(key[i] & (len == 12 ? 0x0f : 0x1f));
            if (trimmed_char == 0) {
                i++;
                continue;
            }  // Skip a zero and move to next

            auto shuffle = len == 12 ? 0 : 5 * (12 - len) - 1;
            res |= trimmed_char << shuffle;
            len++;
            i++;
        }
        return res;
    }

    inline void key_to_account(const std::string &pubkey, std::string &new_account) {
        std::string pub_wif(pubkey);
        pub_wif.erase(0, 3); // Remove 'FIO'/'EOS' prefix from wif
        unsigned char *pub_key_bytes = new unsigned char[37]; // 1 byte head, 256 bit key (32 bytes), 4 bytes checksum (usually)
        // extract key from wif -- note that if done within EOSIO, this process will be done anyways; just use the pre-decoded pubkey.
        DecodeBase58(pub_wif.c_str(), pub_wif.length(), pub_key_bytes);
        uint64_t res = shorten_key(pub_key_bytes);
        name tn = name{res};
        std::string myStr = tn.to_string();
        //throw an error for safety, since this code was just ported.
        new_account = myStr.substr(0, 12);
        delete[] pub_key_bytes;
    }

    inline std::string key_to_account(const std::string &pubkey) {
      std::string newstring;
      key_to_account(pubkey, newstring);
      return newstring;
    }
}
