
#include "fio.fee.hpp"
#include "../fio.name/fio.name.hpp"
#include <fio.common/fio.common.hpp>
#include <fio.common/fioerror.hpp>
#include <eosio/native/intrinsics.hpp>
#include <fio.common/json.hpp>

namespace fioio {

    /***
     * This contract maintains fee related actions and data. Most importantly its the host of the transaction fee
     * structure that is used by sister contracts to 
     */
    class [[eosio::contract("FioFee")]]  FioFee : public eosio::contract {

    private:
        fiofee_table fiofees;
        // TODO: timer for driving elections and updating fees.

    public:


        using contract::contract;
        FioFee(name s, name code, datastream<const char*> ds)
                : contract(s,code,ds), fiofees(_self,_self.value) {
        }

        /***
        * This action will create a new fee on the FIO protocol.
        */
        // @abi action
        [[eosio::action]]
        void create(
                string end_point,   // the blockchain endpoint to which the fee relates.
                uint64_t type,      // this is the fee type from the feetype enumeration.
                uint64_t suf_amount // this is the amount of the fee in small units of FIO.
                                    // 1 billion per fio as of 04/23/2019.
        )
        {

            print("called create fee.","\n");
            require_auth( _self );

            print("creating name for end point ",end_point.c_str());

            uint64_t endPointHash = string_to_uint64_hash(end_point.c_str());
            uint64_t fee_id = fiofees.available_primary_key();
            //emplace the values into the table
            fiofees.emplace(_self, [&](struct fiofee &f) {
                f.fee_id = fee_id;
                f.end_point = end_point;
                f.end_point_hash = endPointHash;
                f.type = type;
                f.suf_amount = suf_amount;
            });
            print("done create fee.", "\n");
        }



    }; // class FioFee

    EOSIO_DISPATCH(FioFee,(create))
} // namespace fioio
