/** FIO Request OBT header file
 *  Description: Smart contract to track requests and OBT.
 *  @author Ciju John, Ed Rotthoff, Casey Gardiner
 *  @file fio.request.obt.hpp
 *  @copyright Dapix
 */


#pragma once

#include <eosiolib/eosio.hpp>
#include <string>
#include <eosiolib/singleton.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/fio/types.hpp>

using std::string;

namespace fioio {

    using namespace eosio;

    // Transaction status
    enum class trxstatus {
        requested = 0,  // request sent
        rejected = 1,  // request rejected
        sent_to_blockchain = 2   // payment sent to blockchain
    };

    // Structure for "FIO request" context.
    // @abi table fioreqctxts i64
    struct fioreqctxt {        // FIO funds request context; specific to requests native to FIO platform
        uint64_t fio_request_id;     // one up index starting at 0
        uint64_t payer_fio_address;  // requestee fio address of fio request
        uint64_t payee_fio_address;    // requestor fio address of the fio request
        string payee_public_address;      // chain specific receiver public address e.g 0xC8a5bA5868A5E9849962167B2F99B2040Cee2031
        string amount;         // token quantity
        string token_code;      // token type e.g. BLU
        string metadata;       // JSON formatted meta data e.g. {"memo":"utility payment"}
        uint64_t time_stamp;      // FIO blockchain request received timestamp

        uint64_t primary_key() const { return fio_request_id; }

        uint64_t by_receiver() const { return payer_fio_address; }

        uint64_t by_originator() const { return payee_fio_address; }

        EOSLIB_SERIALIZE(fioreqctxt,
        (fio_request_id)(payer_fio_address)(payee_fio_address)(payee_public_address)(amount)(token_code)(metadata)(
                time_stamp)
        )
    };

    // FIO requests contexts table
    typedef multi_index<N(fioreqctxts), fioreqctxt,
            indexed_by<N(byreceiver), const_mem_fun<fioreqctxt, uint64_t, &fioreqctxt::by_receiver>>,
            indexed_by<N(byoriginator), const_mem_fun<fioreqctxt, uint64_t, &fioreqctxt::by_originator>
            >> fiorequest_contexts_table;

    // Structure for "FIO request status" updates.
    // @abi table fioreqstss i64
    struct fioreqsts {
        uint64_t id;             // primary key, auto-increment
        uint64_t fio_request_id;       // FIO request {fioreqctxt.fioreqid} this request status update is related to
        uint64_t status;         // request status
        string metadata;       // JSON formatted meta data e.g. {"obt_hash": "962167B2F99B20"}
        uint64_t time_stamp;        // FIO blockchain status update received timestamp

        uint64_t primary_key() const { return id; }

        uint64_t by_fioreqid() const { return fio_request_id; }

        EOSLIB_SERIALIZE(fioreqsts, (id)(fio_request_id)(status)(metadata)(time_stamp)
        )
    };

    // FIO requests status table
    typedef multi_index<N(fioreqstss), fioreqsts,
            indexed_by<N(byfioreqid), const_mem_fun<fioreqsts, uint64_t, &fioreqsts::by_fioreqid> >
    > fiorequest_status_table;

}
