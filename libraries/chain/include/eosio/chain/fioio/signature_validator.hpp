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

#include <eosio/chain/wasm_interface.hpp>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/transaction_context.hpp>
#include <eosio/chain/producer_schedule.hpp>
#include <eosio/chain/exceptions.hpp>
#include <boost/core/ignore_unused.hpp>
#include <eosio/chain/authorization_manager.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/wasm_interface_private.hpp>
#include <eosio/chain/wasm_eosio_validation.hpp>
#include <eosio/chain/wasm_eosio_injection.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/account_object.hpp>
#include <fc/exception/exception.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/sha1.hpp>
#include <fc/io/raw.hpp>

#include <softfloat.hpp>
#include <compiler_builtins.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <fstream>



#pragma once

namespace fioio {

    using namespace std;

    //string signature;
    //fc::crypto::public_key publickey;

    static void assert_recover_key( const fc::sha256& digest, const char * sig, size_t siglen, const char * pub, size_t publen ) {
        fc::crypto::signature s;
        fc::crypto::public_key p;
        datastream<const char*> ds( sig, siglen );
        datastream<const char*> pubds( pub, publen );

        fc::raw::unpack(ds, s);
        fc::raw::unpack(pubds, p);

        auto check = fc::crypto::public_key( s, digest, false );
        EOS_ASSERT( check == p, eosio::chain::packed_transaction_type_exception, "Key Signature mismatch");
    }

    inline bool pubadd_signature_validate(string t_unpackedSig, string fio_pub_key){
        const int sigSize = t_unpackedSig.size();
        const int pubSize = fio_pub_key.size();

        const fc::sha256 digest;
        const string tsig = t_unpackedSig;
        const string tpubkey = fio_pub_key;

        //  find pub_key inside t_unpackedSig (recover key in crypto library)
        assert_recover_key(digest, (const char *)&tsig, sizeof(t_unpackedSig), (const char *)&tpubkey, sizeof(fio_pub_key) );

        return true;
    }

    inline bool fio_transaction_validator(const fc::variant_object& t_vo){
        if( t_vo.contains("packed_trx") && t_vo["packed_trx"].is_string() && !t_vo["packed_trx"].as_string().empty() ) {
            if( t_vo.contains("signatures") && t_vo["signatures"].is_string() && !t_vo["signatures"].as_string().empty() ) {
                return true;
            }
        }
        return false;
    }
}
