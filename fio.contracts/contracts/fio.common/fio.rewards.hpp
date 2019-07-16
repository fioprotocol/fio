#pragma once

#include <vector>
#include <string>
#include <eosiolib/eosio.hpp>

namespace fioio {


    // @abi table tpids i64
    struct [[eosio::action]] bpreward {

      uint64_t rewards;

      uint64_t primary_key() const {return rewards;}

      EOSLIB_SERIALIZE(bpreward, (rewards))


    };

    typedef multi_index<"bprewards"_n, bpreward> bprewards_table;



} // namespace fioio
