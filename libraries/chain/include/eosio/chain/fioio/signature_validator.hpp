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

#pragma once

namespace fioio {

    using namespace std;

    static void assert_recover_key( const fc::sha256& digest, const char * sig, size_t siglen, const char * pub, size_t publen ) {
        fc::crypto::signature s;
        fc::crypto::public_key p;
        fc::datastream<const char*> ds( sig, siglen );
        fc::datastream<const char*> pubds( pub, publen );

        fc::raw::unpack(ds, s);
        fc::raw::unpack(pubds, p);

        auto check = fc::crypto::public_key( s, digest, false );
        EOS_ASSERT( check == p, eosio::chain::invalid_signature_address, "Key Signature mismatch");
    }

    inline void pubadd_signature_validate(string t_unpackedSig, string fio_pub_key){
        const int sigSize = t_unpackedSig.size();
        const int pubSize = fio_pub_key.size();

        const fc::sha256 digest;
        const string tsig = t_unpackedSig;
        const string tpubkey = fio_pub_key;

        //  find pub_key inside t_unpackedSig (recover key in crypto library)
        assert_recover_key(digest, (const char *)&tsig, sizeof(t_unpackedSig), (const char *)&tpubkey, sizeof(fio_pub_key) );
    }

    inline bool is_transaction_packed(const fc::variant_object& t_vo){
        if( t_vo.contains("packed_trx") && t_vo["packed_trx"].is_string() && !t_vo["packed_trx"].as_string().empty() ) {
            return true;
        }
        return false;
    }
}
