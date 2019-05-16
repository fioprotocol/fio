#pragma once

#include <vector>
#include <string>
#include <eosiolib/eosio.hpp>
#include <eosiolib/system.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/crypto.hpp>
#include <fio.common/json.hpp>
#include <fio.common/keyops.hpp>
#include <fio.common/fioerror.hpp>
#include <fio.common/fio_common_validator.hpp>
#include <fio.common/chain_control.hpp>
#include <fio.common/account_operations.hpp>

#ifndef FEE_CONTRACT
#define FEE_CONTRACT "fio.fee"
#endif

#ifndef TOKEN_CONTRACT
#define TOKEN_CONTRACT "eosio.token"
#endif

#ifndef FIO_SYSTEM
#define FIO_SYSTEM "fio.system"
#endif

#ifndef NAME_CONTRACT
#define NAME_CONTRACT "fio.name"
#endif

#ifndef FINANCE_CONTRACT
#define FINANCE_CONTRACT "fio.finance"
#endif

#ifndef YEARTOSECONDS
#define YEARTOSECONDS 31561920
#endif

namespace fioio {

    using namespace eosio;
    using namespace std;
    using time = uint32_t;
    
    static const name FeeContract = name(FEE_CONTRACT);    // account hosting the fee contract
    static const name SystemContract = name(FIO_SYSTEM);

    struct config {
        name tokencontr; // owner of the token contract
        bool pmtson = false; // enable/disable payments

        EOSLIB_SERIALIZE(config, (tokencontr)(pmtson))
    };
    typedef singleton<"configs"_n, config> configs_singleton;


    static constexpr  char char_to_symbol( char c ) {
        if( c >= 'a' && c <= 'z' )
            return (c - 'a') + 6;
        if( c >= '1' && c <= '5' )
            return (c - '1') + 1;
        return 0;
    }


    static constexpr uint64_t string_to_name( const char* str ) {

        uint32_t len = 0;
        while( str[len] ) ++len;

        uint64_t value = 0;

        for( uint32_t i = 0; i <= 12; ++i ) {
            uint64_t c = 0;
            if( i < len && i <= 12 ) c = uint64_t(char_to_symbol( str[i] ));
            if( i < 12 ) {
                c &= 0x1f;
                c <<= 64-5*(i+1);
            }
            else {
                c &= 0x0f;
            }

            value |= c;
        }

        return value;
    }

    static constexpr uint64_t string_to_uint64_hash( const char* str ) {

        uint32_t len = 0;
        while( str[len] ) ++len;
        
        uint64_t value = 0;
        uint64_t multv = 0;
        if (len > 0) {
            multv = 60 / len;
        }
        for( uint32_t i = 0; i < len; ++i ) {
            uint64_t c = 0;
            if( i < len ) c = uint64_t(str[i]);
            
            if( i < 60 ) {
                c &= 0x1f;
                c <<= 64-multv*(i+1);
            }
            else {
                c &= 0x0f;
            }
            
            value |= c;
        }
        
        return value;
    }

} // namespace fioio
