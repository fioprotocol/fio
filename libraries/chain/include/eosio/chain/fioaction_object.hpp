/**
 *  @file
 *  @copyright defined in fio/LICENSE
 */
#pragma once

#include <eosio/chain/database_utils.hpp>
#include <eosio/chain/authority.hpp>
#include <eosio/chain/code_object.hpp>
#include <eosio/chain/block_timestamp.hpp>
#include <eosio/chain/abi_def.hpp>

#include "multi_index_includes.hpp"

namespace eosio {
    namespace chain {


        class fioaction_object : public chainbase::object<fioaction_object_type, fioaction_object> {
            OBJECT_CTOR(fioaction_object)

            id_type id;
            action_name actionname; //< name should not be changed within a chainbase modifier lambda
            name contractname;
            uint64_t blocktimestamp;
        };

        using fioaction_id_type = fioaction_object::id_type;

        struct by_actionname;
        using fioaction_index = chainbase::shared_multi_index_container<
        fioaction_object,
        indexed_by<
                ordered_unique<tag<by_id>, member<fioaction_object, fioaction_object::id_type, &fioaction_object::id>>,
        ordered_unique<tag<by_actionname>, member<fioaction_object, action_name, &fioaction_object::actionname>>
        >
        >;

    }
} // eosio::chain


CHAINBASE_SET_INDEX_TYPE(eosio::chain::fioaction_object, eosio::chain::fioaction_index)


FC_REFLECT(eosio::chain::fioaction_object, (actionname)(contractname)(blocktimestamp))
