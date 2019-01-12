/** signature_validator definitions file
 *  Description: Takes the public key from the unpacked transaction to validate the signature.
 *  @author Casey Gardiner
 *  @file signature_validator.hpp
 *  @copyright Dapix
 *
 *  Changes:
 */

#include <string>

#pragma once

namespace fioio {

    using namespace std;

    //string signature;
    //fc::crypto::public_key publickey;

    inline bool pubadd_signature_validate(string t_unpackedSig, string fio_pub_key){
        //if tKey == fio_pub_key
        //  find pub_key inside unpackedSig (recover key in crypto library)
        //      sig_key == t_Key
        //else return false;
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
