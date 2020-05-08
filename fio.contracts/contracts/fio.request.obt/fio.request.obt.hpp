/** FIO Request OBT header file
 *  Description: The FIO request other blockchain transaction (or obt) contract contains the
 *  tables that hold the requests for funds that have been made along with the status of any
 *  response to each request for funds.
 *  @author Adam Androulidakis, Casey Gardiner, Ciju John, Ed Rotthoff
 *  @file fio.request.obt.hpp
 *  @license FIO Foundation ( https://github.com/fioprotocol/fio/blob/master/LICENSE ) Dapix
 */


#pragma once

#include <eosiolib/eosio.hpp>
#include <string>
#include <eosiolib/singleton.hpp>
#include <eosiolib/asset.hpp>

using std::string;

namespace fioio {

    using namespace eosio;

    // The status of a transaction progresses from requested, to either rejected or sent to blockchain (if funds
    // are sent in response to the request.
    enum class trxstatus {
        requested = 0,
        rejected = 1,
        sent_to_blockchain = 2,
        cancelled = 3
    };

    // The request context table holds the requests for funds that have been requested, it provides
    // searching by id, payer and payee.
    // @abi table fioreqctxts i64
    struct [[eosio::action]] fioreqctxt {
        uint64_t fio_request_id;
        uint128_t payer_fio_addr_hex;
        uint128_t payee_fio_addr_hex;
        string content;  //this content is a encrypted blob containing the details of the request.
        uint64_t time_stamp;
        string payer_fio_addr;
        string payee_fio_addr;
        string payer_key = nullptr;
        string payee_key = nullptr;
        uint128_t payer_key_hex;
        uint128_t payee_key_hex;

        //Future use
        string futureuse_string1 = nullptr;
        string futureuse_string2 = nullptr;
        uint64_t futureuse_uint64 = 0;
        uint128_t futureuse_uint128 = 0;

        uint64_t primary_key() const { return fio_request_id; }
        uint128_t by_receiver() const { return payer_fio_addr_hex; }
        uint128_t by_originator() const { return payee_fio_addr_hex; }
        uint128_t by_payerkey() const { return payer_key_hex; }
        uint128_t by_payeekey() const { return payee_key_hex; }
        uint128_t by_stduint128() const { return futureuse_uint128; }

        EOSLIB_SERIALIZE(fioreqctxt,
        (fio_request_id)(payer_fio_addr_hex)(payee_fio_addr_hex)(content)(time_stamp)
                (payer_fio_addr)(payee_fio_addr)(payer_key)(payee_key)(payer_key_hex)(payee_key_hex)(futureuse_string1)
                (futureuse_string2)(futureuse_uint64)(futureuse_uint128)
        )
    };

    typedef multi_index<"fioreqctxts"_n, fioreqctxt,
            indexed_by<"byreceiver"_n, const_mem_fun < fioreqctxt, uint128_t, &fioreqctxt::by_receiver>>,
    indexed_by<"byoriginator"_n, const_mem_fun<fioreqctxt, uint128_t, &fioreqctxt::by_originator>>,
    indexed_by<"bypayerkey"_n, const_mem_fun<fioreqctxt, uint128_t, &fioreqctxt::by_payerkey>>,
    indexed_by<"bypayeekey"_n, const_mem_fun<fioreqctxt, uint128_t, &fioreqctxt::by_payeekey>>,
    indexed_by<"bystduint"_n, const_mem_fun<fioreqctxt, uint128_t, &fioreqctxt::by_stduint128>
    >>
    fiorequest_contexts_table;

    // TEMP MIGRATION TABLE
    struct [[eosio::action]] fioreqctxt2 {
        uint64_t fio_request_id;
        uint128_t payer_fio_addr_hex;
        uint128_t payee_fio_addr_hex;
        string content;  //this content is a encrypted blob containing the details of the request.
        uint64_t time_stamp;
        string payer_fio_addr;
        string payee_fio_addr;
        string payer_key = nullptr;
        string payee_key = nullptr;
        uint128_t payer_key_hex;
        uint128_t payee_key_hex;

        //Future use
        string futureuse_string1 = nullptr;
        string futureuse_string2 = nullptr;
        uint64_t futureuse_uint64 = 0;
        uint128_t futureuse_uint128 = 0;

        uint64_t primary_key() const { return fio_request_id; }
        uint128_t by_receiver() const { return payer_fio_addr_hex; }
        uint128_t by_originator() const { return payee_fio_addr_hex; }
        uint128_t by_payerkey() const { return payer_key_hex; }
        uint128_t by_payeekey() const { return payee_key_hex; }
        uint128_t by_stduint128() const { return futureuse_uint128; }

        EOSLIB_SERIALIZE(fioreqctxt2,
        (fio_request_id)(payer_fio_addr_hex)(payee_fio_addr_hex)(content)(time_stamp)
                (payer_fio_addr)(payee_fio_addr)(payer_key)(payee_key)(payer_key_hex)(payee_key_hex)(futureuse_string1)
                (futureuse_string2)(futureuse_uint64)(futureuse_uint128)
        )
    };

    typedef multi_index<"fioreqctxts2"_n, fioreqctxt2,
            indexed_by<"byreceiver"_n, const_mem_fun < fioreqctxt2, uint128_t, &fioreqctxt2::by_receiver>>,
    indexed_by<"byoriginator"_n, const_mem_fun<fioreqctxt2, uint128_t, &fioreqctxt2::by_originator>>,
    indexed_by<"bypayerkey"_n, const_mem_fun<fioreqctxt2, uint128_t, &fioreqctxt2::by_payerkey>>,
    indexed_by<"bypayeekey"_n, const_mem_fun<fioreqctxt2, uint128_t, &fioreqctxt2::by_payeekey>>,
    indexed_by<"bystduint"_n, const_mem_fun<fioreqctxt2, uint128_t, &fioreqctxt2::by_stduint128>
    >>
    fiorequest2_contexts_table;

    //this struct holds records relating to the items sent via record obt, we call them recordobt_info items.
    struct [[eosio::action]] recordobt_info {
        uint64_t id;
        uint128_t payer_fio_addr_hex;
        uint128_t payee_fio_addr_hex;
        string content;  //this content is a encrypted blob containing the details of the request.
        uint64_t time_stamp;
        string payer_fio_addr;
        string payee_fio_addr;
        string payer_key = nullptr;
        string payee_key = nullptr;
        uint128_t payer_key_hex;
        uint128_t payee_key_hex;

        //Future use
        string futureuse_string1 = nullptr;
        string futureuse_string2 = nullptr;
        uint64_t futureuse_uint64 = 0;
        uint128_t futureuse_uint128 = 0;

        uint64_t primary_key() const { return id; }
        uint128_t by_receiver() const { return payer_fio_addr_hex; }
        uint128_t by_originator() const { return payee_fio_addr_hex; }
        uint128_t by_payerkey() const { return payer_key_hex; }
        uint128_t by_payeekey() const { return payee_key_hex; }
        uint128_t by_stduint128() const { return futureuse_uint128; }

        EOSLIB_SERIALIZE(recordobt_info,
        (id)(payer_fio_addr_hex)(payee_fio_addr_hex)(content)(time_stamp)
                (payer_fio_addr)(payee_fio_addr)(payer_key)(payee_key)(payer_key_hex)(payee_key_hex)(futureuse_string1)
                (futureuse_string2)(futureuse_uint64)(futureuse_uint128)
        )
    };

    typedef multi_index<"recordobts"_n, recordobt_info,
            indexed_by<"byreceiver"_n, const_mem_fun < recordobt_info, uint128_t, &recordobt_info::by_receiver>>,
    indexed_by<"byoriginator"_n, const_mem_fun<recordobt_info, uint128_t, &recordobt_info::by_originator>>,
    indexed_by<"bypayerkey"_n, const_mem_fun<recordobt_info, uint128_t, &recordobt_info::by_payerkey>>,
    indexed_by<"bypayeekey"_n, const_mem_fun<recordobt_info, uint128_t, &recordobt_info::by_payeekey>>,
    indexed_by<"bystduint"_n, const_mem_fun<recordobt_info, uint128_t, &recordobt_info::by_stduint128>
    >>
    recordobt_table;

    // TEMP MIGRATION TABLE
    struct [[eosio::action]] recobt_info2 {
        uint64_t id;
        uint128_t payer_fio_addr_hex;
        uint128_t payee_fio_addr_hex;
        string content;  //this content is a encrypted blob containing the details of the request.
        uint64_t time_stamp;
        string payer_fio_addr;
        string payee_fio_addr;
        string payer_key = nullptr;
        string payee_key = nullptr;
        uint128_t payer_key_hex;
        uint128_t payee_key_hex;

        //Future use
        string futureuse_string1 = nullptr;
        string futureuse_string2 = nullptr;
        uint64_t futureuse_uint64 = 0;
        uint128_t futureuse_uint128 = 0;

        uint64_t primary_key() const { return id; }
        uint128_t by_receiver() const { return payer_fio_addr_hex; }
        uint128_t by_originator() const { return payee_fio_addr_hex; }
        uint128_t by_payerkey() const { return payer_key_hex; }
        uint128_t by_payeekey() const { return payee_key_hex; }
        uint128_t by_stduint128() const { return futureuse_uint128; }

        EOSLIB_SERIALIZE(recobt_info2,
        (id)(payer_fio_addr_hex)(payee_fio_addr_hex)(content)(time_stamp)
                (payer_fio_addr)(payee_fio_addr)(payer_key)(payee_key)(payer_key_hex)(payee_key_hex)(futureuse_string1)
                (futureuse_string2)(futureuse_uint64)(futureuse_uint128)
        )
    };

    typedef multi_index<"recordobts2"_n, recobt_info2,
            indexed_by<"byreceiver"_n, const_mem_fun < recobt_info2, uint128_t, &recobt_info2::by_receiver>>,
    indexed_by<"byoriginator"_n, const_mem_fun<recobt_info2, uint128_t, &recobt_info2::by_originator>>,
    indexed_by<"bypayerkey"_n, const_mem_fun<recobt_info2, uint128_t, &recobt_info2::by_payerkey>>,
    indexed_by<"bypayeekey"_n, const_mem_fun<recobt_info2, uint128_t, &recobt_info2::by_payeekey>>,
    indexed_by<"bystduint"_n, const_mem_fun<recobt_info2, uint128_t, &recobt_info2::by_stduint128>
    >>
    recobt2_table;

    // The FIO request status table references FIO requests that have had funds sent, or have been rejected.
    // the table provides a means to find the status of a request.
    // @abi table fioreqstss i64
    struct [[eosio::action]] fioreqsts {
        uint64_t id;
        uint64_t fio_request_id;
        uint64_t status;
        string metadata;   //this contains a json string with meta data regarding the response to a request.
        uint64_t time_stamp;

        uint64_t primary_key() const { return id; }
        uint64_t by_fioreqid() const { return fio_request_id; }

        EOSLIB_SERIALIZE(fioreqsts, (id)(fio_request_id)(status)(metadata)(time_stamp)
        )
    };

    typedef multi_index<"fioreqstss"_n, fioreqsts,
            indexed_by<"byfioreqid"_n, const_mem_fun < fioreqsts, uint64_t, &fioreqsts::by_fioreqid> >
    >
    fiorequest_status_table;
}
