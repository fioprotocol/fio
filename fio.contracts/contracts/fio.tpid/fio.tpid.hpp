/** FioTPID implementation file
 *  Description:
 *  @author Adam Androulidakis
 *  @modifedby
 *  @file fio.tpid.cpp
 *  @license FIO Foundation ( https://github.com/fioprotocol/fio/blob/master/LICENSE ) Dapix
 */

#pragma once

#include <fio.common/fio.common.hpp>
#include <fio.address/fio.address.hpp>
#include <string>
#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>

namespace fioio {
    using namespace eosio;

    // @abi table tpids i64
    struct [[eosio::action]] tpid {

        uint64_t id;
        uint128_t fioaddhash;
        string fioaddress;
        uint64_t rewards;

        uint64_t primary_key() const { return id; }
        uint128_t by_name() const { return fioaddhash; }

        EOSLIB_SERIALIZE(tpid, (id)(fioaddhash)(fioaddress)(rewards)
        )
    };

    typedef multi_index<"tpids"_n, tpid,
            indexed_by<"byname"_n, const_mem_fun < tpid, uint128_t, &tpid::by_name>>>
    tpids_table;
}
