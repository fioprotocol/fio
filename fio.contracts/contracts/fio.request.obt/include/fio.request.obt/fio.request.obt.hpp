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
        cancelled = 3,
        obt_action = 4,
        other = 5 //Future Use
    };

    struct ledgerItems {
        std::vector <uint64_t> pending_action_ids;
        std::vector <uint64_t> sent_action_ids;
        std::vector <uint64_t> cancelled_action_ids;
        std::vector <uint64_t> obt_action_ids;

        std::vector <uint64_t> other_ids; //Future Use
    };

    // The request context table holds the requests for funds that have been requested, it provides
    // searching by id, payer and payee.
    // @abi table fioreqctxts i64
    struct [[eosio::action]] fiotrxt {
        uint64_t id;
        uint64_t fio_request_id = 0;
        uint128_t payer_fio_addr_hex;
        uint128_t payee_fio_addr_hex;
        uint8_t fio_data_type; //trxstatus ids
        uint64_t init_time;
        string payer_fio_addr;
        string payee_fio_addr;
        string payer_key = nullptr;
        string payee_key = nullptr;
        uint128_t payer_key_hex;
        uint128_t payee_key_hex;

        string content = "";
        uint64_t update_time = 0;

        //Future use
        string futureuse_string1 = "";
        string futureuse_string2 = "";
        uint64_t futureuse_uint64 = 0;
        uint128_t futureuse_uint128 = 0;

        uint64_t primary_key() const { return id; }
        uint64_t by_requestid() const { return fio_request_id; }
        uint128_t by_receiver() const { return payer_fio_addr_hex; }
        uint128_t by_originator() const { return payee_fio_addr_hex; }
        uint128_t by_payerkey() const { return payer_key_hex; }
        uint128_t by_payeekey() const { return payee_key_hex; }
        uint128_t by_stduint128() const { return futureuse_uint128; }

        EOSLIB_SERIALIZE(fiotrxt,
        (id)(fio_request_id)(payer_fio_addr_hex)(payee_fio_addr_hex)(fio_data_type)(init_time)
                (payer_fio_addr)(payee_fio_addr)(payer_key)(payee_key)(payer_key_hex)(payee_key_hex)
                (content)(update_time)(futureuse_string1)(futureuse_string2)(futureuse_uint64)(futureuse_uint128)
        )
    };

    typedef multi_index<"fiotrxts"_n, fiotrxt,
            indexed_by<"byrequestid"_n, const_mem_fun < fiotrxt, uint64_t, &fiotrxt::by_requestid>>,
    indexed_by<"byreceiver"_n, const_mem_fun<fiotrxt, uint128_t, &fiotrxt::by_receiver>>,
    indexed_by<"byoriginator"_n, const_mem_fun<fiotrxt, uint128_t, &fiotrxt::by_originator>>,
    indexed_by<"bypayerkey"_n, const_mem_fun<fiotrxt, uint128_t, &fiotrxt::by_payerkey>>,
    indexed_by<"bypayeekey"_n, const_mem_fun<fiotrxt, uint128_t, &fiotrxt::by_payeekey>>,
    indexed_by<"bystduint"_n, const_mem_fun<fiotrxt, uint128_t, &fiotrxt::by_stduint128>
    >>
    fiotrxt_contexts_table;

    struct [[eosio::action]] reqledger {

        uint64_t account;
        ledgerItems transactions;

        uint64_t primary_key() const { return account; }

        EOSLIB_SERIALIZE(reqledger, (account)(transactions))
    };

    typedef multi_index<"reqledgers"_n, reqledger> reqledgers_table;
}
