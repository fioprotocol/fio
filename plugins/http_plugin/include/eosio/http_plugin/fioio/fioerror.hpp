/** FioError definitions file
 *  Description: Contains helper functions to generate appropriately formatted http error messages
 *               that are returned by the HTTP messaging layer when an assertion test fails.
 *  @author Phil Mesnier
 *  @file fioerror.hpp
 *  @copyright Dapix
 *
 *  Changes:
 */

#pragma once

namespace fioio {
   using namespace std;

   /**
    * error code definition. Error codes are bitfielded uint64_t values.
    * The fields are: An itentifier, 'FIO\0', then http error code and finally a FIO specific error number.
    */

   constexpr auto identOffset = 48;
   constexpr uint64_t ident = uint64_t((('F' << 4) | 'I' << 4) | 'O') << identOffset; // to distinguish the error codes generically
   constexpr auto httpOffset = 32;
   constexpr uint64_t httpDataError = 400LL << httpOffset;
   constexpr uint64_t httpPermissionError = 403LL << httpOffset;
   constexpr uint64_t httpLocationError = 404LL << httpOffset;
   constexpr uint64_t httpMask = 0xfffLL << httpOffset;
   constexpr uint64_t ecCodeMask = 0xff;

   constexpr auto ErrorDomainAlreadyRegistered =   ident | httpDataError | 100;   // Domain is already registered
   constexpr auto ErrorDomainNotRegistered =       ident | httpDataError | 101;   // Domain not yet registered
   constexpr auto ErrorFioNameAlreadyRegistered =  ident | httpDataError | 102;   // Fioname is already registered
   constexpr auto ErrorFioNameEmpty =              ident | httpDataError | 103;   // FIO user name is empty
   constexpr auto ErrorChainEmpty =                ident | httpDataError | 104;   // Chain name is empty
   constexpr auto ErrorChainAddressEmpty =         ident | httpDataError | 105;   // Chain address is empty
   constexpr auto ErrorChainContainsWhiteSpace =   ident | httpDataError | 106;   // Chain address contains whitespace
   constexpr auto ErrorChainNotSupported =         ident | httpDataError | 107;   // Chain isn't supported
   constexpr auto ErrorFioNameNotRegistered =      ident | httpDataError | 108;   // Fioname not yet registered
   constexpr auto ErrorDomainExpired =             ident | httpDataError | 109;   // Fioname not yet registered
   constexpr auto ErrorFioNameExpired =            ident | httpDataError | 110;   // Fioname not yet registered
   constexpr auto ErrorPubAddressEmpty =           ident | httpDataError | 111;   // Public address is empty
   constexpr auto ErrorPubKeyEmpty =               ident | httpDataError | 112;   // Public key is empty
   constexpr auto ErrorPubAddressExist =           ident | httpDataError | 113;   // Public address exists

   constexpr auto ErrorPermissions =         ident | httpPermissionError | 114;   // user permission failure
   constexpr auto ErrorNotFound =              ident | httpLocationError | 115;   // cannot locate resource

   /**
    * Helper funtions for detecting rich error messages and extracting bitfielded values
    */

   inline bool is_fio_error ( uint64_t ec) {
      constexpr uint64_t mask = ident | httpMask | ecCodeMask;
      return (ec & ~mask) == 0;
   }

   inline uint16_t get_http_result (uint64_t ec) {
      return static_cast<uint16_t>((ec & httpMask) >> httpOffset);
   }

   inline uint64_t get_fio_code (uint64_t ec) {
      return ec & ecCodeMask;
   };

   /**
    * Structures used to collect error content and format proper result messages
    */

   struct Http_Result {
      string type;
      string message;

      Http_Result (const string &t = "", const string &m = "") : type(t), message(m) {}
   };

   struct Code_400_Result : public Http_Result {
      struct field {
         string name = "";
         string value = "";
         string error = "";
      };

      vector<field> fields;
      //      Code_400_Result (const Code_400_field &) default;
      Code_400_Result (const string &fname = "", const string &fval = "", const string &ferr = "") :
         Http_Result ("invalid_input","An invalid request was sent in, please check the nested errors for details.") {
         add_field ( {fname, fval, ferr} );
      }

      void add_field (const field &f) {
         fields.emplace_back (move(f));
      }

      string to_json ( ) const {
         string json_str = "{ \n  \"type\": \"" + type +
            "\",\n  \"message\": \"" + message + "\",\n  \"fields\": [\n";
         for (auto f = fields.cbegin(); f != fields.cend(); f++ ) {
            if ( f != fields.cbegin() )json_str += ",\n";
            json_str += "    {\"name\": \"" + f->name +
               "\",\n    \"value\": \"" + f->value +
               "\",\n    \"error\": \"" + f->error + "\"}";
         }
         json_str += "]\n}";
         return json_str;
      }
   };

   struct Code_403_Result : public Http_Result {
      Code_403_Result () :
         Http_Result ("invalid_signature","Request signature not valid or not allowed.") {}

      string to_json ( ) const {
         string json_str = "{ \n  \"type\": \"" + type +
            "\",\n  \"message\": \"" + message + "\"\n}";
         return json_str;
      }
   };


   struct Code_404_Result : public Http_Result {
      Code_404_Result (const string &msg) :
         Http_Result ("", msg) {}

      string to_json ( ) const {
         string json_str = "{ \n  \"message\": \"" + message + "\"\n}";
         return json_str;
      }
   };

}
   /**
    * helper macros that hide the string conversion tedium
    */

#define fio_400_assert(test,fieldname,fieldvalue,fielderror,code) \
         eosio_assert_message_code(test, Code_400_Result(fieldname, fieldvalue, fielderror).to_json().c_str(), code)

#define fio_403_assert(test,code) \
         eosio_assert_message_code(test, Code_403_Result().to_json().c_str(), code)

#define fio_404_assert(test,message,code) \
         eosio_assert_message_code(test, Code_404_Result(message).to_json().c_str(), code)
