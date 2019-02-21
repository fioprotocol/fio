/** keyops definitions file
 *  Description: Takes in a pubkey and turns it into a hash which is exported to new_account
 *  @author Casey Gardiner
 *  @file keyops.hpp
 *  @copyright Dapix
 *
 *  Changes: Adam Androulidakis 2-20-2019 - Changed hashing method
 *
 */

#include <string>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/fioio/fioerror.hpp>

#pragma once

namespace fioio {

using namespace std;

inline void key_to_account(const std::string &pubkey, std::string &new_account) {
      uint32_t cap = 12;

      FIO_403_ASSERT(!pubkey.empty(), fioio::ErrorPubKeyEmpty);

      std::string pubuniq = pubkey.substr(4,pubkey.size());
      std::string encoded = fc::to_base58(pubuniq.c_str(), pubuniq.length());
      eosio::account_name result = eosio::chain::string_to_uint64_t(encoded.c_str());
      std::string shorter = (string)result;
      std::string output = shorter.erase(cap,cap+1); //Get rid of the occasional 13th character
      new_account = output;
   }

}
