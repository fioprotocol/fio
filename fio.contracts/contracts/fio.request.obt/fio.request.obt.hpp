/** FIO Request OBT header file
 *  Description: Smart contract to track requests and OBT.
 *  @author Adam Androulidakis, Casey Gardiner, Ciju John, Ed Rotthoff
 *  @file fio.request.obt.hpp
 *  @copyright Dapix
 */


#pragma once

#include <eosiolib/eosio.hpp>
#include <string>
#include <eosiolib/singleton.hpp>
#include <eosiolib/asset.hpp>

using std::string;

namespace fioio {

    using namespace eosio;

    // Transaction status
    enum class trxstatus {
        requested = 0,
        rejected = 1,
        sent_to_blockchain = 2
    };

    // Structure for "FIO request" context.
    // @abi table fioreqctxts i64
    struct [[eosio::action]] fioreqctxt {
        uint64_t fio_request_id;
        uint128_t payer_fio_address;
        uint128_t payee_fio_address;
        string payer_fio_address_hex_str;
        string payee_fio_address_hex_str;
        string content;
        uint64_t time_stamp;
        string payer_fio_addr;
        string payee_fio_addr;

        uint64_t primary_key() const { return fio_request_id; }
        uint128_t by_receiver() const { return payer_fio_address; }
        uint128_t by_originator() const { return payee_fio_address; }

        EOSLIB_SERIALIZE(fioreqctxt,
        (fio_request_id)(payer_fio_address)(payee_fio_address)(payer_fio_address_hex_str)(payee_fio_address_hex_str)(
                content)(time_stamp)(payer_fio_addr)(payee_fio_addr)
        )
    };

    // FIO requests contexts table
    typedef multi_index<"fioreqctxts"_n, fioreqctxt,
            indexed_by<"byreceiver"_n, const_mem_fun < fioreqctxt, uint128_t, &fioreqctxt::by_receiver>>,
    indexed_by<"byoriginator"_n, const_mem_fun<fioreqctxt, uint128_t, &fioreqctxt::by_originator>
    >>
    fiorequest_contexts_table;

    // Structure for "FIO request status" updates.
    // @abi table fioreqstss i64
    struct [[eosio::action]] fioreqsts {
        uint64_t id;
        uint64_t fio_request_id;
        uint64_t status;
        string metadata;
        uint64_t time_stamp;

        uint64_t primary_key() const { return id; }
        uint64_t by_fioreqid() const { return fio_request_id; }

        EOSLIB_SERIALIZE(fioreqsts, (id)(fio_request_id)(status)(metadata)(time_stamp)
        )
    };

    // FIO requests status table
    typedef multi_index<"fioreqstss"_n, fioreqsts,
            indexed_by<"byfioreqid"_n, const_mem_fun < fioreqsts, uint64_t, &fioreqsts::by_fioreqid> >
    >
    fiorequest_status_table;
}
