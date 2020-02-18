/** FioName Token header file
 *  Description: FioName smart contract allows issuance of unique domains and names for easy public address resolution
 *  @author Adam Androulidakis, Ciju John, Casey Gardiner, Ed Rotthoff
 *  @file fio.address.hpp
 *  @license FIO Foundation ( https://github.com/fioprotocol/fio/blob/master/LICENSE ) Dapix
 */

#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/asset.hpp>
#include <string>

using std::string;

namespace fioio {

    using namespace eosio;

    struct tokenpubaddr {
        string token_code;
        string chain_code;
        string public_address;
        EOSLIB_SERIALIZE( tokenpubaddr, (token_code)(chain_code)(public_address))
    };


    struct [[eosio::action]] fioname {

        uint64_t id = 0;
        string name = nullptr;
        uint128_t namehash = 0;
        string domain = nullptr;
        uint128_t domainhash = 0;
        uint64_t expiration;
        uint64_t owner_account;
        // Chain specific keys
        std::vector<tokenpubaddr> addresses;
        uint64_t bundleeligiblecountdown = 0;

        // primary_key is required to store structure in multi_index table
        uint64_t primary_key() const { return id; }
        uint128_t by_name() const { return namehash; }
        uint128_t by_domain() const { return domainhash; }
        uint64_t by_expiration() const { return expiration; }
        uint64_t by_owner() const { return owner_account; }

        EOSLIB_SERIALIZE(fioname, (id)(name)(namehash)(domain)(domainhash)(expiration)(owner_account)(addresses)(
                bundleeligiblecountdown)
        )
    };

    //Where fioname tokens are stored
    typedef multi_index<"fionames"_n, fioname,
            indexed_by<"bydomain"_n, const_mem_fun < fioname, uint128_t, &fioname::by_domain>>,
    indexed_by<"byexpiration"_n, const_mem_fun<fioname, uint64_t, &fioname::by_expiration>>,
    indexed_by<"byowner"_n, const_mem_fun<fioname, uint64_t, &fioname::by_owner>>,
    indexed_by<"byname"_n, const_mem_fun<fioname, uint128_t, &fioname::by_name>>
    >
    fionames_table;

    struct [[eosio::action]] domain {
        uint64_t id;
        string name;
        uint128_t domainhash;
        uint64_t account;
        uint8_t is_public = 0;
        uint64_t expiration;


        uint64_t primary_key() const { return id; }
        uint64_t by_account() const { return account; }
        uint64_t by_expiration() const { return expiration; }
        uint128_t by_name() const { return domainhash; }

        EOSLIB_SERIALIZE(domain, (id)(name)(domainhash)(account)(is_public)(expiration)
        )
    };

    typedef multi_index<"domains"_n, domain,
            indexed_by<"byaccount"_n, const_mem_fun < domain, uint64_t, &domain::by_account>>,
    indexed_by<"byexpiration"_n, const_mem_fun<domain, uint64_t, &domain::by_expiration>>,
    indexed_by<"byname"_n, const_mem_fun<domain, uint128_t, &domain::by_name>>
    >
    domains_table;

    // Maps client wallet generated public keys to EOS user account names.
    struct [[eosio::action]] eosio_name {

        uint64_t account = 0;
        string clientkey = nullptr;
        uint128_t keyhash = 0;

        uint64_t primary_key() const { return account; }
        uint128_t by_keyhash() const { return keyhash; }

        EOSLIB_SERIALIZE(eosio_name, (account)(clientkey)(keyhash)
        )
    };

    typedef multi_index<"accountmap"_n, eosio_name,
            indexed_by<"bykey"_n, const_mem_fun < eosio_name, uint128_t, &eosio_name::by_keyhash>>
    >
    eosio_names_table;
}
