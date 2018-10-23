/** FioName Token header file
 *  Description: FioName smart contract allows issuance of unique domains and names for easy public address resolution
 *  @author Adam Androulidakis
 *  @file fio.name.hpp
 *  @copyright Dapix
 *
 *  Changes:
 *  Adam Androulidakis 9-18-2018
 *  Adam Androulidakis 9-6-2018
 *  Ciju John 9-5-2018
 *  Adam Androulidakis  8-31-2018
 *  Ciju John  8-30-2018
 *  Adam Androulidakis  8-29-2019
 */
 

#pragma once

#include <eosiolib/eosio.hpp>
#include <string>
#include <eosiolib/singleton.hpp>
//#include <map>


using std::string;

namespace fioio {
    
        using namespace eosio;

    // @abi table fionames i64
    struct fioname {

        string name = nullptr;
        uint64_t namehash = 0;
        string domain = nullptr;
        uint64_t domainhash = 0;
                //EXPIRATION this is the expiration for this fio name, this is a number of seconds since 1970
                uint32_t expiration;
                
                
        // Chain specific keys
        vector<string> addresses;
                // std::map<string, string> fionames;
                
                // primary_key is required to store structure in multi_index table
        uint64_t primary_key() const { return namehash; }
        uint64_t by_domain() const { return domainhash; }
                
        EOSLIB_SERIALIZE(fioname, (name)(namehash)(domain)(domainhash)(addresses))
    };

        //Where fioname tokens are stored
    typedef multi_index<N(fionames), fioname,
     indexed_by<N(bydomain), const_mem_fun<fioname, uint64_t, &fioname::by_domain> > > fionames_table;

    // @abi table domains i64
    struct domain {
        string name;
        uint64_t domainhash;
                //EXPIRATION this is the expiration for this fio domain, this is a number of seconds since 1970
                uint32_t expiration;

        uint64_t primary_key() const { return domainhash; }
        EOSLIB_SERIALIZE(domain, (name)(domainhash))
    };
    typedef multi_index<N(domains), domain> domains_table;

    // Structures/table for mapping chain key to FIO name
    // @abi table keynames i64
    struct key_name {
        uint64_t id;
        string key = nullptr;       // user key on a chain
        uint64_t keyhash = 0;       // chainkey hash
        uint64_t chaintype;         // maps to ${FioNameLookup::chain_type}
        string name = nullptr;      // FIO name

        uint64_t primary_key() const { return id; }
        uint64_t by_key() const { return keyhash; }
        EOSLIB_SERIALIZE(key_name, (id)(key)(keyhash)(chaintype)(name))
    };
    typedef multi_index<N(keynames), key_name,
            indexed_by<N(bykey), const_mem_fun<key_name, uint64_t, &key_name::by_key> > > keynames_table;

    struct config {
        name tokencontr; // owner of the token contract
        EOSLIB_SERIALIZE(config, (tokencontr))
    };
        
    typedef singleton<N(configs), config> configs;
}
