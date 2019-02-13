/** Fio Request Obt implementation file
 *  Description: FioRequestObt smart contract supports funds request and other block chain transaction recording.
 *  @author Ed Rotthoff
 *  @file fio.request.obt.cpp
 *  @copyright Dapix
 *
 *  Changes:
 */

#include <eosiolib/asset.hpp>
#include "fio.request.obt.hpp"
#include <fio.common/fio.common.hpp>
#include <fio.common/json.hpp>
#include <eosio/chain/fioio/fioerror.hpp>
#include <eosio/chain/fioio/fio_common_validator.hpp>
#include <climits>


namespace fioio {


    class FioRequestObt : public contract {


    public:
        explicit FioRequestObt(account_name self)
                : contract(self) {}

        /***
         * this action will record a send using Obt. the input json will be verified, if verification fails an exception will be thrown.
         * if verification succeeds then status tables will be updated...
         */
        // @abi action
        void recordsend(const string &obtjson) {


            print("FioRequestObt.recordsend input obtjson:  ", obtjson, "\n");
            //TBD implement the necessary logic...Ed will do this.


            //get the sender and receiver and the fio_funds_requestid if it is set.
            /*nlohmann::json jsonobt = obtjson;
            string fromFioAddress = jsonobt["from_fio_address"];
            string toFioAddress = jsonobt["to_fio_address"];


            FioAddress fa = getFioAddressStruct(fromFioAddress);
            FioAddress ta = getFioAddressStruct(toFioAddress);

            print("FioRequestObt.recordsend input json processing: from address: ", fa.fioname, ", to address: ", ta.fioname, "\n");*/
        }


    };
EOSIO_ABI(FioRequestObt, (recordsend))
}

