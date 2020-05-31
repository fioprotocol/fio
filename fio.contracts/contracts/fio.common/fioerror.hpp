/** FioError definitions file
 *  Description: Contains helper functions to generate appropriately formatted http error messages
 *               that are returned by the HTTP messaging layer when an assertion test fails.
 *  @author Phil Mesnier, Adam Androulidakis, Casey Gardiner, Ed Rotthoff
 *  @file fioerror.hpp
 *  @license FIO Foundation ( https://github.com/fioprotocol/fio/blob/master/LICENSE ) Dapix
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
    constexpr uint64_t ident =
            uint64_t((('F' << 4) | 'I' << 4) | 'O') << identOffset; // to distinguish the error codes generically
    constexpr auto httpOffset = 32;
    constexpr uint64_t httpDataError = 400LL << httpOffset;
    constexpr uint64_t httpInvalidError = 403LL << httpOffset;
    constexpr uint64_t httpLocationError = 404LL << httpOffset;
    constexpr uint64_t httpMask = 0xfffLL << httpOffset;
    constexpr uint64_t ecCodeMask = 0xff;

    constexpr auto ErrorDomainAlreadyRegistered = ident | httpDataError | 100;   // Domain is already registered
    constexpr auto ErrorDomainNotRegistered = ident | httpDataError | 101;   // Domain not yet registered
    constexpr auto ErrorFioNameAlreadyRegistered = ident | httpDataError | 102;   // Fioname is already registered

    constexpr auto ErrorFioNameEmpty = ident | httpDataError | 103;   // FIO user name is empty
    constexpr auto ErrorChainEmpty = ident | httpDataError | 104;   // Chain name is empty
    constexpr auto ErrorChainAddressEmpty = ident | httpDataError | 105;   // Chain address is empty
    constexpr auto ErrorChainContainsWhiteSpace = ident | httpDataError | 106;   // Chain address contains whitespace
    constexpr auto ErrorChainNotSupported = ident | httpDataError | 107;   // Chain isn't supported
    constexpr auto ErrorFioNameNotRegistered = ident | httpLocationError | 108;
    constexpr auto ErrorFioNameNotReg = ident | httpDataError | 127;   // Fioname not yet registered
    constexpr auto ErrorDomainExpired = ident | httpDataError | 109;   // Fioname not yet registered
    constexpr auto ErrorFioNameExpired = ident | httpDataError | 110;   // Fioname not yet registered
    constexpr auto ErrorPubAddressEmpty = ident | httpDataError | 111;   // Public address is empty
    constexpr auto ErrorPubKeyEmpty = ident | httpDataError | 112;   // Public key is empty
    constexpr auto ErrorPubAddressExist = ident | httpDataError | 113;   // Public address exists

    constexpr auto ErrorSignature = ident | httpInvalidError | 114;   // user permission failure
    constexpr auto ErrorNotFound = ident | httpLocationError | 115;   // cannot locate resource
    constexpr auto ErrorInvalidFioNameFormat = ident | httpDataError | 116;   // Public address exists
    constexpr auto ErrorTransaction = ident | httpInvalidError | 117;   // Transaction error
    constexpr auto ErrorNoFIONames = ident | httpLocationError | 118; // No FIO Names
    constexpr auto ErrorInvalidJsonInput = ident | httpDataError | 119;   // invalid json sent for json input
    constexpr auto ErrorRequestContextNotFound =
            ident | httpDataError | 120;   // the specified request context record was not found
    constexpr auto ErrorChainAddressNotFound = ident | httpDataError | 121;   // Chain address not found
    constexpr auto ErrorNoFioRequestsFound = ident | httpLocationError | 122;   // no fio requests found
    constexpr auto Error400FioNameNotRegistered = ident | httpDataError | 123;   // Fioname not yet registered
    constexpr auto ErrorPubAddressNotFound = ident | httpLocationError | 124; //Pub Address not found
    constexpr auto ErrorDomainNotFound = ident | httpLocationError | 125;   // Domain not yet registered
    constexpr auto ErrorLowFunds = ident | httpDataError | 126; // Insufficient balance
    constexpr auto ErrorEndpointNotFound = ident | httpDataError | 127;   // Fee endpoint not found.
    constexpr auto ErrorNoEndpoint = ident | httpDataError | 128; // No endpoint specified.
    constexpr auto ErrorNoFeesFoundForEndpoint = ident | httpDataError | 129; // No Fees found for endpoint
    constexpr auto ErrorMaxFeeExceeded = ident | httpDataError | 130; // max fee exceeded.
    constexpr auto InvalidTPID = ident | httpDataError | 131; // max fee exceeded.
    constexpr auto ErrorProxyNotFound = ident | httpLocationError | 132;
    constexpr auto ErrorPublicKeyExists = ident | httpDataError | 133; // pub key already exists.
    constexpr auto ErrorNoFioAddressProducer = ident | httpDataError | 134; // producer does is not fioname
    constexpr auto AddressNotProxy = ident | httpDataError | 135; // This address is not a proxy
    constexpr auto InvalidAccountOrAction =
            ident | httpInvalidError | 136; // Provided account or action is not valid for this endpoint.
    constexpr auto ErrorActorNotInFioAccountMap = ident | httpDataError | 137;   // Actor not in FIO account map
    constexpr auto ErrorTokenCodeInvalid = ident | httpDataError | 138;   // token code invalid
    constexpr auto ErrorPubKeyValid = ident | httpDataError | 139; //Invalid FIO Public Key (400)
    constexpr auto ErrorInvalidMultiplier = ident | httpInvalidError | 140;   // invalid fee multiplier
    constexpr auto ErrorMaxFeeInvalid = ident | httpDataError | 141; // max fee invalid.
    constexpr auto ErrorFeeInvalid = ident | httpDataError | 142; // fee invalid.
    constexpr auto ErrorInvalidAmount = ident | httpDataError | 143; // Invalid amount value
    constexpr auto ErrorContentLimit = ident | httpDataError | 144; //     constexpr auto ErrorInvalidAmount = ident | httpDataError | 143; // Invalid amount value
    constexpr auto ErrorPagingInvalid = ident | httpDataError | 145; //Invalid FIO Public Key (400)
    constexpr auto ErrorInvalidNumberAddresses = ident | httpDataError | 146;   // Invalid number of addresses
    constexpr auto ErrorInsufficientUnlockedFunds = ident | httpDataError | 147;   // not enough unlocked funds
    constexpr auto ErrorNoWork = ident | httpDataError | 148;   // no work to perform
    constexpr auto ErrorClientKeyNotFound = ident | httpDataError | 149; // Fioname not yet registered (No clientkey)
    constexpr auto ErrorTimeViolation = ident | httpDataError | 150;
    constexpr auto ErrorNoAuthWaits = ident | httpDataError | 151;
    constexpr auto ErrorDomainOwner = ident | httpInvalidError | 153;
    constexpr auto ErrorTransactionTooLarge = ident | httpDataError | 152;   // Transaction too large
    constexpr auto ErrorRequestStatusInvalid = ident | httpDataError | 153;   // the specified request context record was not found
    constexpr auto ErrorActorIsSystemAccount = ident | httpDataError | 154;   // the specified actor is a FIO system account
    constexpr auto ErrorInvalidUnlockPeriods = ident | httpDataError | 155;   //Invalid unlock periods.
    constexpr auto ErrorInvalidValue = ident | httpDataError | 155;   //Invalid value.
    constexpr auto ErrorNoGeneralLocksFound = ident | httpLocationError | 156;   // no fio requests found
    constexpr auto ErrorUnexpectedNumberResults = ident | httpLocationError | 156;   // unexpected number of results

    /**
    * Helper funtions for detecting rich error messages and extracting bitfielded values
    */

    inline bool is_fio_error(uint64_t ec) {
        constexpr uint64_t mask = ident | httpMask | ecCodeMask;
        return (ec & ident) == ident && (ec & ~mask) == 0;
    }

    inline uint16_t get_http_result(uint64_t ec) {
        return static_cast<uint16_t>((ec & httpMask) >> httpOffset);
    }

    inline uint64_t get_fio_code(uint64_t ec) {
        return ec & ecCodeMask;
    };

    /**
     * Structures used to collect error content and format proper result messages
     */

    struct Http_Result {
        string type;
        string message;

        Http_Result(const string &t = "", const string &m = "") : type(t), message(m) {}
    };

    struct Code_400_Result : public Http_Result {
        struct field {
            string name = "";
            string value = "";
            string error = "";
        };

        vector <field> fields;

        //      Code_400_Result (const Code_400_field &) default;
        Code_400_Result(const string &fname = "", const string &fval = "", const string &ferr = "") :
                Http_Result("invalid_input",
                            "An invalid request was sent in, please check the nested errors for details.") {
            add_field({fname, fval, ferr});
        }

        void add_field(const field &f) {
            fields.emplace_back(move(f));
        }

        string to_json() const {
            string json_str = "{\n  \"type\": \"" + type +
                              "\",\n  \"message\": \"" + message + "\",\n  \"fields\": [\n";
            for (auto f = fields.cbegin(); f != fields.cend(); f++) {
                if (f != fields.cbegin())json_str += ",\n";
                json_str += "    {\"name\": \"" + f->name +
                            "\",\n    \"value\": \"" + f->value +
                            "\",\n    \"error\": \"" + f->error + "\"}";
            }
            json_str += "]\n}\n";
            return json_str;
        }
    };

    struct Code_403_Result : public Http_Result {
        Code_403_Result(uint64_t code) :
                Http_Result("invalid_signature", "Request signature not valid or not allowed.") {
            if (code == fioio::ErrorTransaction) {
                type = "invalid_transaction";
                message = "Signed transaction is not valid or is not formatted properly";
            }
            if (code == fioio::InvalidAccountOrAction) {
                type = "invalid_account_or_action";
                message = "Provided account or action is not valid for this endpoint";
            }
        }

        string to_json() const {
            string json_str = "{\n  \"type\": \"" + type +
                              "\",\n  \"message\": \"" + message + "\"\n}\n";
            return json_str;
        }
    };


    struct Code_404_Result : public Http_Result {
        Code_404_Result(const string &msg) :
                Http_Result("", msg) {}

        string to_json() const {
            string json_str = "{\n  \"message\": \"" + message + "\"\n}\n";
            return json_str;
        }
    };

}
/**
 * helper macros that hide the string conversion tedium
 */
#if 0
#define FIO_400_THROW(fieldname,fieldvalue,fielderror,code) \
   FC_MULTILINE_MACRO_BEGIN                            \
   throw eosio::chain::fio_data_exception(code,fioio::Code_400_Result(fieldname, fieldvalue, fielderror).to_json().c_str()); \
   FC_MULTILINE_MACRO_END

#define FIO_403_THROW(code) \
   FC_MULTILINE_MACRO_BEGIN                            \
   throw eosio::chain::fio_invalid_exception(code,fioio::Code_403_Result(code).to_json().c_str()); \
   FC_MULTILINE_MACRO_END

#define FIO_404_THROW(message,code) \
   FC_MULTILINE_MACRO_BEGIN                            \
   throw eosio::chain::fio_location_exception(code,fioio::Code__400_Result(message).to_json().c_str()); \
   FC_MULTILINE_MACRO_END
#endif

#define FIO_400_ASSERT(expr, fieldname, fieldvalue, fielderror, code)     \
   FC_MULTILINE_MACRO_BEGIN                                           \
   if( !(expr) )                                                      \
      throw eosio::chain::eosio_assert_code_exception(code, "message", fioio::Code_400_Result(fieldname, fieldvalue, fielderror).to_json().c_str()); \
   FC_MULTILINE_MACRO_END

#define FIO_403_ASSERT(expr, code) \
   FC_MULTILINE_MACRO_BEGIN                                           \
   if( !(expr) )                                                      \
      { throw  eosio::chain::eosio_assert_code_exception(code, "message", fioio::Code_403_Result(code).to_json().c_str()); } \
   FC_MULTILINE_MACRO_END

#define FIO_404_ASSERT(expr, message, code) \
   FC_MULTILINE_MACRO_BEGIN                                           \
   if( !(expr) )                                                      \
      throw  eosio::chain::eosio_assert_code_exception(code, "message", fioio::Code_404_Result(message).to_json().c_str()); \
   FC_MULTILINE_MACRO_END

#define fio_400_assert(test, fieldname, fieldvalue, fielderror, code) \
   eosio_assert_message_code(test, fioio::Code_400_Result(fieldname, fieldvalue, fielderror).to_json().c_str(), code)

#define fio_403_assert(test, code) \
   eosio_assert_message_code(test, fioio::Code_403_Result(code).to_json().c_str(), code)

#define fio_404_assert(test, message, code) \
   eosio_assert_message_code(test, fioio::Code_404_Result(message).to_json().c_str(), code)
