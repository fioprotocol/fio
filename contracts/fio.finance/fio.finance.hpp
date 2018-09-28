/** Fio Finance header file
 *  Description: FioFinance smart contract supports funds request and approval.
 *  @author Ciju John
 *  @file fio.finance.hpp
 *  @copyright Dapix
 *
 *  Changes:
 */


#pragma once

#include <eosiolib/eosio.hpp>
#include <string>
#include <eosiolib/singleton.hpp>
#include <eosiolib/asset.hpp>


using std::string;

namespace fioio {

    using namespace eosio;

    // structure for storing funds request
    // @abi table fundsrequest i64
    struct fundsrequest {
        uint32_t requestid; // user supplied request id, mainly for user to track requests
        uint64_t obtid; // FIO Other Blockchain Transaction (OBT) Transaction ID
        name from; // request originator
        name to; // requestee
        string chain;
        eosio::asset quantity; // requested fund quantity
        string memo; // user generated memo
        time request_time; // request received timestamp
        time aproval_time; // request approved time stamp

        uint64_t primary_key() const { return obtid; }

        uint64_t by_requestid() const { return requestid; }

        EOSLIB_SERIALIZE(fundsrequest, (requestid)(obtid)(from)(to)(chain)(quantity)(memo)(request_time)(aproval_time))
    };

    typedef multi_index<N(pendrequests), fundsrequest,
            indexed_by<N(
                    byrequestid), const_mem_fun<fundsrequest, uint64_t, &fundsrequest::by_requestid> > > pending_requests_table;
    typedef multi_index<N(aprvrequests), fundsrequest> approved_requests_table;

    struct config {
        name tokencontr; // owner of the token contract

        EOSLIB_SERIALIZE(config, (tokencontr))
    };
    typedef singleton<N(configs), config> configs;

    struct contr_state {
        uint64_t current_obt = 0; // obt generator

        EOSLIB_SERIALIZE(contr_state, (current_obt))
    };
    typedef singleton<N(state), contr_state> statecontainer;
}
