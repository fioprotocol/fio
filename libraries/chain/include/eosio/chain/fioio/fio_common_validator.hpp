/** fio_address_validator definitions file
 *  Description: Takes a fio.name and validates it as spec.
 *  @author Casey Gardiner
 *  @file fio_name_validator.hpp
 *  @copyright Dapix
 *
 *  Changes:
 */

#include <string>
//#include <boost/algorithm/string.hpp> // *DANGER - don't use! Severe performance side effects may result

#pragma once

namespace fioio {

    constexpr auto maxFioNameLen = 50;
    constexpr auto maxFioDomainLen = 50;

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

   inline void getFioAddressStruct(const string &p, FioAddress& fa){
        // Split the fio name and domain portions
        fa.fioname = "";
        fa.fiodomain = "";

        size_t pos = p.find('.');
        fa.domainOnly = pos == 0 || pos == string::npos;

        //Lower Case
        fa.fiopubaddress = p;
        for (auto &c : fa.fiopubaddress) {
           c = char(::tolower(c));
        }

        if (pos == string::npos) {
            fa.fiodomain = fa.fiopubaddress;
        } else {
           if (!fa.domainOnly)
              fa.fioname = fa.fiopubaddress.substr(0, pos);
           fa.fiodomain = fa.fiopubaddress.substr(pos + 1);
        }
    }

   inline int isFioNameValid( const string &name ){
        //Name validation.
      if (name.size() < 1 || name.size() > maxFioNameLen) {
         return 1;
      }
      if(name.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-") != std::string::npos) {
         return 2;
      }
      if( name.front() == '-' || name.back() == '-') {
         return 3;
      }
      return 0;
    }

    inline int isDomainNameValid( string domain, bool domainOnly ){
       return domainOnly ? isFioNameValid (domain)*10 : 0;
    }

    inline int fioNameSizeCheck(string fn, string fd){
        size_t totalsize = fn.size() + fd.size();

        if( totalsize > maxFioNameLen+maxFioDomainLen){
            return 100;
        }
        return 0;
    }

    inline bool isChainNameValid( string chain ){
        if (chain.size() >= 1 && chain.size() <= 10){
            if(chain.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789") != std::string::npos) {
                return false;
            }
        } else {
            return false;
        }
        return true;
    }

   inline string chainToUpper( string chain ) {
      string my_chain = chain;

      transform(my_chain.begin(), my_chain.end(), my_chain.begin(), ::toupper);

      return my_chain;
   }

}
