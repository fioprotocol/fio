
#pragma once

#include <eosiolib/eosio.hpp>
#include <string>
#include <eosiolib/singleton.hpp>
//#include <map>

using namespace eosio;
using std::string;

namespace dapix {

    // @abi table accounts i64
    struct account {

        string name;
        uint64_t namehash;
        string domain;
        uint64_t domainhash;
        // chain specific keys
        vector<string> keys;
//        std::map<string, string> accounts;

        uint64_t primary_key() const { return namehash; }
        uint64_t by_domain() const { return domainhash; }
        EOSLIB_SERIALIZE(account, (name)(namehash)(domain)(domainhash)(keys))
    };

    typedef multi_index<N(accounts), account,
     indexed_by<N(bydomain), const_mem_fun<account, uint64_t, &account::by_domain> > > accounts_table;

    // @abi table domains i64
    struct domain {
        string name;
        uint64_t domainhash;

        uint64_t primary_key() const { return domainhash; }
        EOSLIB_SERIALIZE(domain, (name)(domainhash))
    };
    typedef multi_index<N(domains), domain> domains_table;

    struct config {
        name tokencontr; // owner of the token contract
        EOSLIB_SERIALIZE(config, (tokencontr))
    };
    typedef singleton<N(configs), config> configs;
}
