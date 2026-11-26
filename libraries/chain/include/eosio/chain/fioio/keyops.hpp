/** keyops definitions file
 *  Description: Takes in a pubkey and turns it into a hash which is exported to new_account
 *  @author Casey Gardiner
 *  @file keyops.hpp
 *  @license FIO Foundation ( https://github.com/fioprotocol/fio/blob/master/LICENSE ) Dapix
 *
 *  Changes: Adam Androulidakis 2-20-2019 - Changed hashing method
 *
 */

#include <algorithm>
#include <string>
#include <vector>
#include <eosio/chain/name.hpp>

#pragma once

using namespace eosio;

namespace fioio {

    const uint32_t acctcap = 12;
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
// maxlen is the capacity of result. Returns the decoded length, or -1 if the
// input holds a non-base58 character or the output would exceed maxlen.
    static int DecodeBase58(
            const char *str, int len, unsigned char *result, int maxlen) {
        if (str == nullptr || result == nullptr || len < 0 || maxlen < 1) return -1;
        int resultlen = 1;
        result[0] = 0;
        for (int i = 0; i < len; i++) {
            const unsigned char c = (unsigned char) str[i];
            if (c >= sizeof(ALPHABET_MAP)) return -1;
            const signed char digit = (signed char) ALPHABET_MAP[c];
            if (digit < 0) return -1;
            unsigned int carry = (unsigned int) digit;
            for (int j = 0; j < resultlen; j++) {
                carry += (unsigned int) (result[j]) * 58;
                result[j] = (unsigned char) (carry & 0xff);
                carry >>= 8;
            }
            while (carry > 0) {
                if (resultlen >= maxlen) return -1;
                result[resultlen++] = (unsigned char) (carry & 0xff);
                carry >>= 8;
            }
        }

        for (int i = 0; i < len && str[i] == '1'; i++) {
            if (resultlen >= maxlen) return -1;
            result[resultlen++] = 0;
        }

        // Reverse in place.
        for (int i = resultlen - 1, z = (resultlen >> 1) + (resultlen & 1);
             i >= z; i--) {
            int k = result[i];
            result[i] = result[resultlen - i - 1];
            result[resultlen - i - 1] = k;
        }
        return resultlen;
    }

    // key must hold at least 33 bytes.
    static uint64_t shorten_key(const unsigned char *key) {
        uint64_t res = 0;

        int i = 1;  // Ignore key head
        int len = 0;
        while (len <= 12) {
            if (i >= 33) break; // Key has > 20 bytes with trailing zeroes; stop at the buffer end.

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
        new_account.clear();
        if (pubkey.length() < 3) return;
        std::string pub_wif(pubkey);
        pub_wif.erase(0, 3); // Remove 'FIO'/'EOS' prefix from wif
        // 1 byte head, 256 bit key (32 bytes), 4 bytes checksum; DecodeBase58 needs len * 2 in the worst case.
        std::vector<unsigned char> pub_key_bytes(std::max<size_t>(pub_wif.length() * 2, 37), 0);
        // extract key from wif -- note that if done within EOSIO, this process will be done anyways; just use the pre-decoded pubkey.
        if (DecodeBase58(pub_wif.c_str(), (int) pub_wif.length(), pub_key_bytes.data(),
                         (int) pub_key_bytes.size()) < 0) {
            return; // Not a base58 key: no account.
        }
        uint64_t res = shorten_key(pub_key_bytes.data());
        name tn = name{res};
        std::string myStr = tn.to_string();
        new_account = myStr.substr(0, 12);
    }

    inline std::string key_to_account(const std::string &pubkey) {
      std::string newstring;
      key_to_account(pubkey, newstring);
      return newstring;
    }
}
