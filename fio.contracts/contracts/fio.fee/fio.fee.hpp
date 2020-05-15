/** FioFee implementation file
 *  Description: FioFee is the smart contract that manages fees. see fio.fee.cpp
 *  @author Adam Androulidakis, Casey Gardiner, Ed Rotthoff
 *  @modifedby
 *  @file fio.fee.cpp
 *  @license FIO Foundation ( https://github.com/fioprotocol/fio/blob/master/LICENSE ) Dapix
 */

#pragma once

#include <eosiolib/eosio.hpp>

using std::string;

namespace fioio {

    using namespace eosio;

    enum class feetype {
        mandatory = 0,  // these fees are applied each time an operation takes place.
        bundleeligible = 1  // these fees are free until a predetermined number of operations takes place,
        // then a fee is applied. the number of free transactions is determined by votes of the block producers.
    };

    struct feevalue {
        string end_point; //this is the name of the endpoint, which is by convention the same as the
                          //url to which the signed transaction is sent.
        uint64_t value;   //this it the value of the fee in FIO SUFs (Smallest unit of FIO).

        EOSLIB_SERIALIZE( feevalue, (end_point)(value))
    };

    //this is the amount of time that must elapse for votes to be recorded into the FIO protocol for fees.
    const uint32_t TIME_BETWEEN_VOTES_SECONDS = 120;
    const uint32_t TIME_BETWEEN_FEE_VOTES_SECONDS = 3600;

    // This table contains the data attributes associated with a fee.
    // @abi table fiofee i64
    struct [[eosio::action]] fiofee {
        uint64_t fee_id;     // one up index starting at 0
        string end_point;
        uint128_t end_point_hash;
        uint64_t type;      // this is the fee type from the feetype enumeration.
        uint64_t suf_amount;

        uint64_t primary_key() const { return fee_id; }
        uint128_t by_endpoint() const { return end_point_hash; }
        uint64_t by_type() const { return type; }

        EOSLIB_SERIALIZE(fiofee, (fee_id)(end_point)(end_point_hash)(type)(suf_amount)
        )
    };

    typedef multi_index<"fiofees"_n, fiofee,
            indexed_by<"byendpoint"_n, const_mem_fun < fiofee, uint128_t, &fiofee::by_endpoint>>,
            indexed_by<"bytype"_n, const_mem_fun<fiofee, uint64_t, &fiofee::by_type>
    >>
    fiofee_table;


    // this is the feevoter table, it holds the votes made for fees, a fee vote has producer name and
    // a multiplier that will be applied to the vote to arrive at the final fee amount used.
    // @abi table feevoter i64
    struct [[eosio::action]] feevoter {
        name block_producer_name;
        double fee_multiplier;
        uint64_t lastvotetimestamp;

        uint64_t primary_key() const { return block_producer_name.value; }

        EOSLIB_SERIALIZE(feevoter, (block_producer_name)(fee_multiplier)(lastvotetimestamp)
        )
    };

    typedef multi_index<"feevoters"_n, feevoter> feevoters_table;

    // the bundlevoter table holds the block producer votes for the number of transactions that will be
    // free before the user is charged a fee.
    // @abi table bundlevoter i64
    struct [[eosio::action]] bundlevoter {
        name block_producer_name;
        int64_t bundledbvotenumber;
        uint64_t lastvotetimestamp;

        uint64_t primary_key() const { return block_producer_name.value; }

        EOSLIB_SERIALIZE(bundlevoter, (block_producer_name)(bundledbvotenumber)(lastvotetimestamp)
        )
    };

    typedef multi_index<"bundlevoters"_n, bundlevoter> bundlevoters_table;

    // This table holds block producer votes for fees. this table holds the vote for each fee for each block producer
    // in SUFs. The votes here will be multiplied by the multiplier in the feevoters table.
    // @abi table feevote i64
    struct [[eosio::action]] feevote {
        uint64_t id;       //unique one up id
        name block_producer_name;
        string end_point;
        uint128_t end_point_hash;
        uint64_t suf_amount;
        uint64_t lastvotetimestamp;

        uint64_t primary_key() const { return id; }
        uint128_t by_endpoint() const { return end_point_hash; }
        uint64_t by_bpname() const { return block_producer_name.value; }

        EOSLIB_SERIALIZE(feevote, (id)(block_producer_name)(end_point)(end_point_hash)(suf_amount)(lastvotetimestamp)
        )
    };

    typedef multi_index<"feevotes"_n, feevote,
            indexed_by<"byendpoint"_n, const_mem_fun < feevote, uint128_t, &feevote::by_endpoint>>,
            indexed_by<"bybpname"_n, const_mem_fun<feevote, uint64_t, &feevote::by_bpname>>
    >
    feevotes_table;
} // namespace fioio