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
#include <eosiolib/asset.hpp>


using std::string;

namespace fioio {

        using namespace eosio;

    // @abi table fionames i64
    struct fioname {

        string name = nullptr;
        uint64_t namehash = 0;
        string domain = nullptr;
        uint64_t domainhash = 0;


        // Chain specific keys
        vector<string> addresses;
		// std::map<string, string> fionames;

		// primary_key is required to store structure in multi_index table
        uint64_t primary_key() const { return namehash; }
        uint64_t by_domain() const { return domainhash; }

        EOSLIB_SERIALIZE(fioname, (name)(namehash)(domain)(domainhash)(addresses))
    };

    // structure for storing funds request
    struct fundsrequest {
        uint32_t requestid; // user supplied request id, mainly for user to track requests
        uint64_t obtid; // FIO Other Blockchain Transaction (OBT) Transaction ID
        name from; // request originator
        name to; // requestee
        string chain;
        eosio::asset quantity; // requested fund quantity
        string memo; // user generated memo
        time  request_time; // request received timestamp
        time  approval_time; // request approved time stamp

        uint64_t primary_key() const { return obtid; }
        uint64_t by_requestid() const { return requestid; }
        EOSLIB_SERIALIZE(fundsrequest, (requestid)(obtid)(from)(to)(chain)(quantity)(memo)(request_time)(approval_time))
    };
    typedef multi_index<N(fundrequests), fundsrequest,
            indexed_by<N(byrequestid), const_mem_fun<fundsrequest, uint64_t, &fundsrequest::by_requestid> > > requests_table;

    //Where fioname tokens are stored
    typedef multi_index<N(fionames), fioname,
     indexed_by<N(bydomain), const_mem_fun<fioname, uint64_t, &fioname::by_domain> > > fionames_table;

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
