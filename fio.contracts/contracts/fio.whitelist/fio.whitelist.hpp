/** FioWhitelist implementation file
 *  Description: FioWhitelist smart contract controls whitelisting on the FIO Blockchain.
 *  @author Casey Gardiner, Ed Rotthoff
 *  @modifedby
 *  @file fio.whitelist.cpp
 *  @copyright Dapix
 */

#include <fio.common/fio.common.hpp>
#include <fio.name/fio.name.hpp>
#include <fio.tpid/fio.tpid.hpp>
#include <string>
#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>

namespace fioio {
    using namespace eosio;

    // @abi table whitelist i64
    struct [[eosio::table]]  whitelist_info {
        uint64_t id; //this is the id of the record
        uint64_t owner;     //this is the owner of this whitelist.
        string lookupindex;
        uint64_t lookupindex_hash;  //this is the hashed lookup index of whom is being whitelisted
        string content;   //this holds the encrypted json representation of the fio address and public key of
        //of whom is being whitelisted

        uint64_t primary_key() const { return id; }
        uint64_t by_owner() const { return owner; }
        uint64_t by_lookupindex() const { return lookupindex_hash; }

        EOSLIB_SERIALIZE(whitelist_info, (id)(owner)(lookupindex)(lookupindex_hash)(content)
        )
    };

    typedef eosio::multi_index<"whitelist"_n, whitelist_info,
            indexed_by<"byowner"_n, const_mem_fun < whitelist_info, uint64_t, &whitelist_info::by_owner>>,
    indexed_by<"bylookupidx"_n, const_mem_fun<whitelist_info, uint64_t, &whitelist_info::by_lookupindex>>
    >
    whitelist_table;


}
