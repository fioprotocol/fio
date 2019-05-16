/** Fio Request Obt implementation file
 *  Description: FioRequestObt smart contract supports funds request and other block chain transaction recording.
 *  @author Ed Rotthoff, Casey Gardiner
 *  @file fio.request.obt.cpp
 *  @copyright Dapix
 *
 *  Changes:
 */

#include <eosiolib/asset.hpp>
#include "fio.request.obt.hpp"
#include "../fio.name/fio.name.hpp"
#include "../fio.fee/fio.fee.hpp"
#include <fio.common/fio.common.hpp>
#include <fio.common/json.hpp>
#include <eosio/chain/fioio/fioerror.hpp>
#include <eosio/chain/fioio/fio_common_validator.hpp>
#include <eosio/chain/fioio/chain_control.hpp>

namespace fioio {

    class FioRequestObt : public contract {

    private:
        fiorequest_contexts_table fiorequestContextsTable;
        fiorequest_status_table fiorequestStatusTable;
        fionames_table fionames;
        fiofee_table fiofees;
        config appConfig;


        const account_name TokenContract = eosio::string_to_name(TOKEN_CONTRACT);
    public:
        explicit FioRequestObt(account_name self)
                : contract(self), fiorequestContextsTable(self, self),
                fiorequestStatusTable(self, self),
                fionames(SystemContract,SystemContract),
                fiofees(FeeContract,FeeContract){
            configs_singleton configsSingleton(FeeContract, FeeContract);
            appConfig = configsSingleton.get_or_default(config());
        }


        struct decrementcounter {
            string fio_address;
        };
        /***
         * this action will record a send using Obt. the input json will be verified, if verification fails an exception will be thrown.
         * if verification succeeds then status tables will be updated...
         */
        // @abi action
        void recordsend(const string &payer_fio_address, const string &payee_fio_address,
                        const string &payer_public_address, const string &payee_public_address, const string &amount,
                        const string &token_code, const string status, const string obt_id, const string &metadata,
                        const string fio_request_id, uint64_t max_fee, const string &actor) {
            //check that names were found in the json.
            fio_400_assert(payer_fio_address.length() > 0, "payer_fio_address", payer_fio_address,
                           "from fio address not found in obt json blob", ErrorInvalidJsonInput);
            fio_400_assert(payee_fio_address.length() > 0, "payee_fio_address", payee_fio_address,
                           "to fio address not found in obt json blob", ErrorInvalidJsonInput);

            //if the request id is specified in the json then look to see if it is present
            //in the table, if so then add the associated update into the status tables.
            //if the id is present in the json and not in the table error.
            if (fio_request_id.length() > 0) {
                uint64_t currentTime = current_time();
                uint64_t requestId;

                requestId = std::atoi(fio_request_id.c_str());

                auto fioreqctx_iter = fiorequestContextsTable.find(requestId);
                fio_400_assert(fioreqctx_iter != fiorequestContextsTable.end(), "fio_request_id", fio_request_id,
                               "No such FIO Request ", ErrorRequestContextNotFound);
                //insert a send record into the status table using this id.
                fiorequestStatusTable.emplace(_self, [&](struct fioreqsts &fr) {
                    fr.id = fiorequestStatusTable.available_primary_key();;
                    fr.fio_request_id = requestId;
                    fr.status = static_cast<int64_t >(trxstatus::sent_to_blockchain);
                    fr.metadata = "";
                    fr.time_stamp = currentTime;
                });
            }

            //check the from address, see that its a valid fio name
            uint64_t nameHash = ::eosio::string_to_uint64_t(payer_fio_address.c_str());
            auto fioname_iter = fionames.find(nameHash);
            fio_400_assert(fioname_iter != fionames.end(), "payer_fio_address", payer_fio_address,
                           "No such FIO Address",
                           ErrorFioNameNotReg);
            uint64_t account = fioname_iter->account;

            //check the to address, see that its a valid fio name
            nameHash = ::eosio::string_to_uint64_t(payee_fio_address.c_str());
            fioname_iter = fionames.find(nameHash);

            fio_400_assert(fioname_iter != fionames.end(), "payee_fio_address", payee_fio_address,
                           "No such FIO Address",
                           ErrorFioNameNotReg);

            account_name aactor = eosio::string_to_name(actor.c_str());
            print("account: ", account, " actor: ", aactor, "\n");
            fio_403_assert(account == aactor, ErrorSignature);


            //begin new fees, bundle eligible fee logic
            uint64_t endpoint_hash = string_to_uint64_t("record_send");

            auto fees_by_endpoint = fiofees.get_index<N(byendpoint)>();
            auto fee_iter = fees_by_endpoint.find(endpoint_hash);
            //if the fee isnt found for the endpoint, then 400 error.
            fio_400_assert(fee_iter != fees_by_endpoint.end(),"endpoint_name", "record_send",
                           "FIO fee not found for endpoint", ErrorNoEndpoint);

            uint64_t reg_amount = fee_iter->suf_amount;
            uint64_t fee_type = fee_iter->type;

            //if its not a bundleeligible fee then this is an error.
            fio_400_assert(fee_type == 1,"fee_type", to_string(fee_type),
                           "record_send unexpected fee type for endpoint record_send, expected 1", ErrorNoEndpoint);


            uint64_t bundleeligiblecountdown = fioname_iter->bundleeligiblecountdown;

            uint64_t fee_amount = 0;

            if (bundleeligiblecountdown >0)
            {
                //fee is zero, and decrement the counter.
                fee_amount = 0;
                action {
                        permission_level{_self, N(active)},
                        N(fio.system),
                        N(decrcounter),
                        decrementcounter {
                                .fio_address = payer_fio_address
                        }
                }.send();

            } else {
                fee_amount = fee_iter->suf_amount;
                fio_400_assert(max_fee >= fee_amount, "max_fee", to_string(max_fee), "Fee exceeds supplied maximum.",ErrorMaxFeeExceeded );

                //NOTE -- question here, should we always record the transfer for the fees, even when its zero,
                //or should we do as this code does and not do a transaction when the fees are 0.
                asset reg_fee_asset = asset(fee_amount);

                fio_fees(aactor, reg_fee_asset);
            }
            //end new fees, bundle eligible fee logic

            nlohmann::json json = {{"status", status},
                                   {"fee_collected",fee_amount}};

            send_response(json.dump().c_str());
        }

        inline void fio_fees(const account_name &actor, const asset &fee) const {
            if (appConfig.pmtson) {
                account_name fiosystem = eosio::string_to_name("fio.system");
                // check for funds is implicitly done as part of the funds transfer.
                print("Collecting FIO API fees: ", fee);
                action(permission_level{actor, N(active)},
                       TokenContract, N(transfer),
                       make_tuple(actor, fiosystem, fee,
                                  string("FIO API fees. Thank you."))
                ).send();
            } else {
                print("Payments currently disabled.");
            }
        }

        /***
        * this action will record a send using Obt. the input json will be verified, if verification fails an exception will be thrown.
        * if verification succeeds then status tables will be updated...
        */
        // @abi action
        void newfundsreq(const string &payer_fio_address, const string &payee_fio_address,
                         const string &payee_public_address, const string &amount,
                         const string &token_code, const string &metadata, uint64_t max_fee, const string &actor) {

            //check that names were found in the json.
            fio_400_assert(payer_fio_address.length() > 0, "payer_fio_address", payer_fio_address,
                           "from fio address not specified",
                           ErrorInvalidJsonInput);
            fio_400_assert(payee_fio_address.length() > 0, "payee_fio_address", payee_fio_address,
                           "to fio address not specified",
                           ErrorInvalidJsonInput);

            //check the from address, see that its a valid fio name
            uint64_t nameHash = ::eosio::string_to_uint64_t(payer_fio_address.c_str());
            auto fioname_iter = fionames.find(nameHash);
            fio_400_assert(fioname_iter != fionames.end(), "payer_fio_address", payer_fio_address,
                           "No such FIO Address",
                           Error400FioNameNotRegistered);

            //check the to address, see that its a valid fio name
            nameHash = ::eosio::string_to_uint64_t(payee_fio_address.c_str());
            fioname_iter = fionames.find(nameHash);
            fio_400_assert(fioname_iter != fionames.end(), "payee_fio_address", payee_fio_address,
                           "No such FIO Address",
                           Error400FioNameNotRegistered);
            uint64_t account = fioname_iter->account;

            account_name aActor = eosio::string_to_name(actor.c_str());
            print("account: ", account, " actor: ", aActor, "\n");
            fio_403_assert(account == aActor, ErrorSignature);

            //put the thing into the table get the index.
            uint64_t id = fiorequestContextsTable.available_primary_key();
            uint64_t currentTime = now();
            uint64_t toHash = ::eosio::string_to_uint64_t(payee_fio_address.c_str());
            uint64_t fromHash = ::eosio::string_to_uint64_t(payer_fio_address.c_str());

            //insert a send record into the status table using this id.
            fiorequestContextsTable.emplace(_self, [&](struct fioreqctxt &frc) {
                frc.fio_request_id = id;
                frc.payer_fio_address = fromHash;
                frc.payee_fio_address = toHash;
                frc.payee_public_address = payee_public_address;
                frc.amount = amount;
                frc.token_code = token_code;
                frc.metadata = metadata;
                frc.time_stamp = currentTime;
            });



            //begin new fees, bundle eligible fee logic
            uint64_t endpoint_hash = string_to_uint64_t("new_funds_request");

            auto fees_by_endpoint = fiofees.get_index<N(byendpoint)>();
            auto fee_iter = fees_by_endpoint.find(endpoint_hash);
            //if the fee isnt found for the endpoint, then 400 error.
            fio_400_assert(fee_iter != fees_by_endpoint.end(),"endpoint_name", "new_funds_request",
                           "FIO fee not found for endpoint", ErrorNoEndpoint);

            uint64_t reg_amount = fee_iter->suf_amount;
            uint64_t fee_type = fee_iter->type;

            //if its not a bundleeligible fee then this is an error.
            fio_400_assert(fee_type == 1,"fee_type", to_string(fee_type),
                           "new_funds_request unexpected fee type for endpoint new_funds_request, expected 1", ErrorNoEndpoint);


            uint64_t bundleeligiblecountdown = fioname_iter->bundleeligiblecountdown;

            uint64_t fee_amount = 0;

            if (bundleeligiblecountdown >0)
            {
                //fee is zero, and decrement the counter.
                fee_amount = 0;
                action {
                        permission_level{_self, N(active)},
                        N(fio.system),
                        N(decrcounter),
                        decrementcounter {
                                .fio_address = payee_fio_address
                        }
                }.send();

            } else {
                fee_amount = fee_iter->suf_amount;
                fio_400_assert(max_fee >= fee_amount, "max_fee", to_string(max_fee), "Fee exceeds supplied maximum.",ErrorMaxFeeExceeded );

                //NOTE -- question here, should we always record the transfer for the fees, even when its zero,
                //or should we do as this code does and not do a transaction when the fees are 0.
                asset reg_fee_asset = asset(fee_amount);

                fio_fees(aActor, reg_fee_asset);
            }
            //end new fees, bundle eligible fee logic

            nlohmann::json json = {{"fio_request_id", id},
                                   {"status",         "requested"},
                                   {"fee_collected",fee_amount}};

            send_response(json.dump().c_str());
        }

        /***
         * this action will add a rejection status to the specified request id.
         * the input fiorequest id will be verified to ensure there is a request in the contexts table matching this id
         * before the status record is added to the index tables.
         f*/
        // @abi action
        void rejectfndreq(const string &fio_request_id, uint64_t max_fee, const string &actor) {
            print("call new funds request\n");

            fio_400_assert(fio_request_id.length() > 0, "fio_request_id", fio_request_id, "No value specified",
                           ErrorRequestContextNotFound);
            //if the request id is specified in the json then look to see if it is present
            //in the table, if so then add the associated update into the status tables.
            //if the id is present in the json and not inthe table error.
            uint64_t currentTime = current_time();
            uint64_t requestId;

            requestId = std::atoi(fio_request_id.c_str());

            auto fioreqctx_iter = fiorequestContextsTable.find(requestId);
            fio_400_assert(fioreqctx_iter != fiorequestContextsTable.end(), "fio_request_id", fio_request_id,
                           "No such FIO Request", ErrorRequestContextNotFound);

            uint64_t fromFioAdd = fioreqctx_iter->payer_fio_address;

            //check the from address, see that its a valid fio name
            auto fioname_iter = fionames.find(fromFioAdd);


            fio_403_assert(fioname_iter != fionames.end(), ErrorSignature);

            uint64_t account = fioname_iter->account;
            string payer_fio_address = fioname_iter->name;

            account_name aactor = eosio::string_to_name(actor.c_str());
            print("account: ", account, " actor: ", aactor, "\n");
            fio_403_assert(account == aactor, ErrorSignature);

            //insert a send record into the status table using this id.
            fiorequestStatusTable.emplace(_self, [&](struct fioreqsts &fr) {
                fr.id = fiorequestStatusTable.available_primary_key();;
                fr.fio_request_id = requestId;
                fr.status = static_cast<int64_t >(trxstatus::rejected);
                fr.metadata = "";
                fr.time_stamp = currentTime;
            });

            //begin new fees, bundle eligible fee logic

            uint64_t endpoint_hash = string_to_uint64_t("reject_funds_request");

            auto fees_by_endpoint = fiofees.get_index<N(byendpoint)>();
            auto fee_iter = fees_by_endpoint.find(endpoint_hash);
            //if the fee isnt found for the endpoint, then 400 error.
            fio_400_assert(fee_iter != fees_by_endpoint.end(),"endpoint_name", "reject_funds_request",
                           "FIO fee not found for endpoint", ErrorNoEndpoint);

            uint64_t reg_amount = fee_iter->suf_amount;
            uint64_t fee_type = fee_iter->type;

            //if its not a bundleeligible fee then this is an error.
            fio_400_assert(fee_type == 1,"fee_type", to_string(fee_type),
                           "reject_funds_request unexpected fee type for endpoint reject_funds_request, expected 1", ErrorNoEndpoint);


            uint64_t bundleeligiblecountdown = fioname_iter->bundleeligiblecountdown;

            uint64_t fee_amount = 0;

            if (bundleeligiblecountdown >0)
            {
                //fee is zero, and decrement the counter.
                fee_amount = 0;
                action {
                        permission_level{_self, N(active)},
                        N(fio.system),
                        N(decrcounter),
                        decrementcounter {
                                .fio_address = payer_fio_address
                        }
                }.send();

            } else {
                fee_amount = fee_iter->suf_amount;
                fio_400_assert(max_fee >= fee_amount, "max_fee", to_string(max_fee), "Fee exceeds supplied maximum.",ErrorMaxFeeExceeded );

                //NOTE -- question here, should we always record the transfer for the fees, even when its zero,
                //or should we do as this code does and not do a transaction when the fees are 0.
                asset reg_fee_asset = asset(fee_amount);

                fio_fees(aactor, reg_fee_asset);
            }
            //end new fees, bundle eligible fee logic

            nlohmann::json json = {{"status", "request_rejected"},
                                   {"fee_collected",fee_amount}};
            send_response(json.dump().c_str());
        }
    };

    EOSIO_ABI(FioRequestObt, (recordsend)(newfundsreq)(rejectfndreq))
}

