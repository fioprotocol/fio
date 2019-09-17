/** FioName Token header file
 *  Description: FioName smart contract allows issuance of unique domains and names for easy public address resolution
 *  @author Adam Androulidakis, Ciju John, Casey Gardiner, Ed Rotthoff
 *  @file fio.name.hpp
 *  @copyright Dapix
 */

#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/asset.hpp>
#include <string>

using std::string;

namespace fioio {

    using namespace eosio;

    // @abi table fionames i64
    struct [[eosio::action]] fioname {

        string name = nullptr;
        uint64_t namehash = 0;
        string domain = nullptr;
        uint64_t domainhash = 0;
        uint64_t expiration;
        uint64_t owner_account;
        // Chain specific keys
        std::vector <string> addresses;
        uint64_t bundleeligiblecountdown = 0;

        // primary_key is required to store structure in multi_index table
        uint64_t primary_key() const { return namehash; }
        uint64_t by_domain() const { return domainhash; }
        uint64_t by_expiration() const { return expiration; }

        uint64_t by_owner() const { return owner_account; }

        EOSLIB_SERIALIZE(fioname, (name)(namehash)(domain)(domainhash)(expiration)(owner_account)(addresses)(
                bundleeligiblecountdown)
        )
    };

    //Where fioname tokens are stored
    typedef multi_index<"fionames"_n, fioname,
    indexed_by<"bydomain"_n, const_mem_fun < fioname, uint64_t, &fioname::by_domain>>,
    indexed_by<"byexpiration"_n, const_mem_fun<fioname, uint64_t, &fioname::by_expiration>>,
    indexed_by<"byowner"_n, const_mem_fun<fioname, uint64_t, &fioname::by_owner>>
    >
    fionames_table;

    // @abi table domains i64
    struct [[eosio::action]] domain {
        string name;
        uint64_t domainhash;
        uint8_t is_public = false;
        uint64_t expiration;
        uint64_t account;

        uint64_t primary_key() const { return domainhash; }
        uint64_t by_account() const { return account; }
        uint64_t by_expiration() const { return expiration; }

        EOSLIB_SERIALIZE(domain, (name)(domainhash)(is_public)(expiration)(account)
        )
    };

    typedef multi_index<"domains"_n, domain,
            indexed_by<"byaccount"_n, const_mem_fun < domain, uint64_t, &domain::by_account>>,
            indexed_by<"byexpiration"_n, const_mem_fun < domain, uint64_t, &domain::by_expiration>>
                    >
    domains_table;

    // @abi table chains i64
    struct [[eosio::action]] chainList {
        string chainname = nullptr;
        uint32_t id = 0;
        uint64_t chainhash = 0;

        uint64_t primary_key() const { return chainhash; }

        uint64_t by_index() const { return id; }

        EOSLIB_SERIALIZE(chainList, (chainname)(id)(chainhash)
        )
    };

    typedef multi_index<"chains"_n, chainList> chains_table;

    // Maps client wallet generated public keys to EOS user account names.
    // @abi table eosionames i64
    struct [[eosio::action]] eosio_name {

        uint64_t account = 0;
        string clientkey = nullptr;
        uint64_t keyhash = 0;

        uint64_t primary_key() const { return account; }
        uint64_t by_keyhash() const { return keyhash; }

        EOSLIB_SERIALIZE(eosio_name, (account)(clientkey)(keyhash)
        )
    };

    typedef multi_index<"accountmap"_n, eosio_name,
            indexed_by<"bykey"_n, const_mem_fun < eosio_name, uint64_t, &eosio_name::by_keyhash>>
    >
    eosio_names_table;
}
