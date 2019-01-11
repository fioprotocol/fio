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
    //string publickey;

    inline bool pub_address_validate(string t_unpackedSig, string t_Key, string fio_pub_key){
        //if tKey == fio_pub_key
        //  find pub_key inside unpackedSig
        //      sig_key == t_Key
        //else return false;
        return true;
    }
}
