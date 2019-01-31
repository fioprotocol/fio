/** fio_address_validator definitions file
 *  Description: Takes a fio.name and validates it as spec.
 *  @author Casey Gardiner
 *  @file fio_name_validator.hpp
 *  @copyright Dapix
 *
 *  Changes:
 */

#include <string>
#include <boost/algorithm/string.hpp>

#pragma once

namespace fioio {

    using namespace std;

    struct FioAddress{
        string fiopubaddress;
        string fioname;
        string fiodomain;
        bool domainOnly;
    };

    inline bool isFioNameEmpty( string p ){
        return p.empty();
    }

    inline FioAddress getFioAddressStruct( string p ){
        // Split the fio name and domain portions
        FioAddress fa;

        fa.fioname = "";
        fa.fiodomain = "";

        int pos = p.find('.');
        fa.domainOnly = false;

        //Lower Case
        fa.fiopubaddress = boost::algorithm::to_lower_copy(p); //WARNING: Fails for non-ASCII-7

        if (pos == string::npos) {
            fa.fiodomain = fa.fiopubaddress;
        } else {
            fa.fioname = fa.fiopubaddress.substr(0, pos);
            fa.fiodomain = fa.fiopubaddress.substr(pos + 1, string::npos);

            if(fa.fiopubaddress.size() < 1 || ( fa.fiodomain.size() + 1 ) == fa.fiopubaddress.size()){
                fa.domainOnly = true;
            }
        }
        return fa;
    }

    inline bool isFioNameValid( string name ){
        //Name validation.
        if (name.size() >= 1 && name.size() <= 50){
            if(name.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-") != std::string::npos) {
                return false;
            }
            else if(name.at(name.size() - 1) == '.' || name.at(name.size() - 1) == '-' ||
                    boost::algorithm::equals(name, "-") || name.at(0) == '-') {
                return false;
            }
        } else {
            return false;
        }
        return true;
    }

    inline bool isDomainNameValid( string domain, bool singlecheck ){
        if (( domain.size() >= 1 && domain.size() <= 50 ) && !singlecheck){
            if(domain.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-") != std::string::npos) {
                return false;
            }
            else if(boost::algorithm::equals(domain, "-") || domain.at(0) == '-' ||
                    domain.at(domain.size() - 1) == '-' ) {
                return false;
            }
        } else {
            return false;
        }
        return true;
    }

    inline bool fioNameSizeCheck(string fn, string fd){
        int totalsize = fn.size() + fd.size();

        if( totalsize >= 100){
            return false;
        }
        return true;
    }

    inline bool isChainNameValid( string chain ){
        if (chain.size() >= 1 && chain.size() <= 10){
            if(chain.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789") != std::string::npos) {
                return false;
            }
        } else {
            return false;
        }
        return true;
    }
}