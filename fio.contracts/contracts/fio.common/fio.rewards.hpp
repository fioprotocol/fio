#pragma once

#include <vector>
#include <string>
#include <eosiolib/eosio.hpp>

namespace fioio {


    // @abi table bpreward i64
    struct [[eosio::action]] bpreward {

      uint64_t rewards;

      uint64_t primary_key() const {return rewards;}

      EOSLIB_SERIALIZE(bpreward, (rewards))


    };

    typedef multi_index<"bprewards"_n, bpreward> bprewards_table;

    // @abi table bpreward i64
    struct [[eosio::action]] fdtnreward {

      uint64_t rewards;

      uint64_t primary_key() const {return rewards;}

      EOSLIB_SERIALIZE(fdtnreward, (rewards))


    };

    typedef multi_index<"fdtnrewards"_n, fdtnreward> fdtnrewards_table;



} // namespace fioio
