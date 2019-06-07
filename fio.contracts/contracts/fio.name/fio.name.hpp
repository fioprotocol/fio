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
        uint64_t account;
        // Chain specific keys
        std::vector <string> addresses;
        uint64_t bundleeligiblecountdown = 0;

        // primary_key is required to store structure in multi_index table
        uint64_t primary_key() const { return namehash; }

        uint64_t by_domain() const { return domainhash; }

        uint64_t by_expiration() const { return expiration; }

        EOSLIB_SERIALIZE(fioname, (name)(namehash)(domain)(domainhash)(expiration)(account)(addresses)(
                bundleeligiblecountdown)
        )
    };

    //Where fioname tokens are stored
    typedef multi_index<"fionames"_n, fioname,
            indexed_by<"bydomain"_n, const_mem_fun < fioname, uint64_t, &fioname::by_domain>>,
            indexed_by<"byexpiration"_n, const_mem_fun < fioname, uint64_t, &fioname::by_expiration>>
    >
    fionames_table;

    // @abi table domains i64
    struct [[eosio::action]] domain {
        string name;
        uint64_t domainhash;
        uint64_t expiration;
        uint64_t account;

        uint64_t primary_key() const { return domainhash; }

        uint64_t by_account() const { return account; }

        uint64_t by_expiration() const { return expiration; }

        EOSLIB_SERIALIZE(domain, (name)(domainhash)(expiration)(account)
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

    // Structures/table for mapping chain key to FIO name
    // @abi table keynames i64
    struct [[eosio::action]] key_name {
        uint64_t id;
        string key = nullptr;       // user key on a chain
        uint64_t keyhash = 0;       // chainkey hash
        uint64_t chaintype;         // maps to chain_control vector position
        string name = nullptr;      // FIO name
        uint64_t namehash;          //use this index for searches when burning or whenever useful.
        uint32_t expiration;        //expiration of the fioname.

        uint64_t primary_key() const { return id; }
        uint64_t by_keyhash() const { return keyhash; }
        uint64_t by_namehash() const { return namehash; }

        EOSLIB_SERIALIZE(key_name, (id)(key)(keyhash)(chaintype)(name)(namehash)(expiration)
        )
    };

    typedef multi_index<"keynames"_n, key_name,
            indexed_by<"bykey"_n, const_mem_fun < key_name, uint64_t, &key_name::by_keyhash> >,
            indexed_by<"bynamehash"_n, const_mem_fun < key_name, uint64_t, &key_name::by_namehash> > >
    keynames_table;

    // Maps client wallet generated public keys to EOS user account names.
    // @abi table eosionames i64
    struct [[eosio::action]] eosio_name {

        uint64_t account = 0;
        uint64_t accounthash = 0;
        string clientkey = nullptr;
        uint64_t keyhash = 0;

        uint64_t primary_key() const { return account; }

        uint64_t by_keyhash() const { return keyhash; }

        uint64_t by_accounthash() const { return accounthash; }

        EOSLIB_SERIALIZE(eosio_name, (account)(accounthash)(clientkey)(keyhash)
        )
    };

    typedef multi_index<"eosionames"_n, eosio_name,
            indexed_by<"bykey"_n, const_mem_fun < eosio_name, uint64_t, &eosio_name::by_keyhash>>,
    indexed_by<"byaccount"_n, const_mem_fun<eosio_name, uint64_t, &eosio_name::by_accounthash>
    >>
    eosio_names_table;
}
