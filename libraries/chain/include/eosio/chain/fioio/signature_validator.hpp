/** signature_validator definitions file
 *  Description: Takes the public key from the unpacked transaction to validate the signature.
 *  @author Casey Gardiner
 *  @file signature_validator.hpp
 *  @copyright Dapix
 *
 *  Changes:
 */

#include <string>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/fioio/fioerror.hpp>

#pragma once

namespace fioio {

    using namespace std;

   static void assert_recover_key( const fc::sha256& digest,
                                   const eosio::chain::signature_type &sig,
                                   const fc::string &pub)
   {
        auto check = fc::crypto::public_key( sig, digest, false );
        FIO_403_ASSERT(string (check) == pub, ErrorPermissions);
    }

   inline void pubadd_signature_validate(const fc::sha256& digest,
                                         const vector<eosio::chain::signature_type> &t_unpackedSig,
                                         const fc::string &fio_pub_key)
   {
        assert_recover_key(digest, t_unpackedSig[0], fio_pub_key );
    }

    inline bool is_transaction_packed(const fc::variant_object& t_vo)
    {
       return t_vo.contains("packed_trx") &&
          t_vo["packed_trx"].is_string() &&
          !t_vo["packed_trx"].as_string().empty();
    }
}
