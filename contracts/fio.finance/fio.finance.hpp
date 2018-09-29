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

    const static uint8_t REQUEST_APPROVED = 0;
    const static uint8_t REQUEST_PENDING = 1;
    const static uint8_t REQUEST_REJECTED = 2;

    // structure for storing funds request
    // @abi table pendreqsts i64
    // @abi table finalreqsts i64
    struct fundsrequest {
        uint32_t requestid; // user supplied request id, mainly for user to track requests
        uint64_t obtid; // FIO Other Blockchain Transaction (OBT) Transaction ID
        name requestor; // request originator
        name requestee; // requestee
        string chain;
        eosio::asset quantity; // requested fund quantity
        string memo; // user generated memo, can be encrypted

        time request_time; // request received timestamp
        time final_time; // request approved time stamp
        uint64_t state = REQUEST_PENDING; // tracks request state
        string final_detail; // free form finalization details

        uint64_t primary_key() const { return obtid; }
        uint64_t by_requestid() const { return requestid; }
        uint64_t by_requestee() const { return requestee; }

        EOSLIB_SERIALIZE(fundsrequest, (requestid)(obtid)(requestor)(requestee)(chain)(quantity)(memo)(request_time)(final_time)(state)(final_detail))
    };

    // requests in this table will have state = REQUEST_PENDING
    typedef multi_index<N(pendreqsts), fundsrequest,
            indexed_by<N(byrequestid), const_mem_fun<fundsrequest, uint64_t, &fundsrequest::by_requestid> >,
            indexed_by<N(byrequestee), const_mem_fun<fundsrequest, uint64_t, &fundsrequest::by_requestee> >
                    > pending_requests_table;

    // requests in this table will have state REQUEST_REJECTED or REQUEST_APPROVED
    typedef multi_index<N(finalreqsts), fundsrequest,
            indexed_by<N(byrequestid), const_mem_fun<fundsrequest, uint64_t, &fundsrequest::by_requestid> >,
            indexed_by<N(byrequestee), const_mem_fun<fundsrequest, uint64_t, &fundsrequest::by_requestee> >
                    > finalized_requests_table;

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
