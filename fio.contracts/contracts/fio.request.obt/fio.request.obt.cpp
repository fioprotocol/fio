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
#include <fio.name/fio.name.hpp>
#include <fio.fee/fio.fee.hpp>
#include <fio.common/fio.common.hpp>
#include <fio.common/json.hpp>
#include <fio.common/fioerror.hpp>
#include <fio.tpid/fio.tpid.hpp>


namespace fioio {


    class [[eosio::contract("FioRequestObt")]]  FioRequestObt : public eosio::contract {

    private:
        fiorequest_contexts_table fiorequestContextsTable;
        fiorequest_status_table fiorequestStatusTable;
        fionames_table fionames;
        domains_table domains;
        fiofee_table fiofees;
        config appConfig;
        tpids_table tpids;


    public:
        explicit FioRequestObt(name s, name code, datastream<const char *> ds)
                : contract(s, code, ds),
                  fiorequestContextsTable(_self, _self.value),
                  fiorequestStatusTable(_self, _self.value),
                  fionames(SystemContract, SystemContract.value),
                  domains(SystemContract, SystemContract.value),
                  fiofees(FeeContract, FeeContract.value),
                  tpids(SystemContract, SystemContract.value) {
            configs_singleton configsSingleton(FeeContract, FeeContract.value);
            appConfig = configsSingleton.get_or_default(config());
        }


        struct decrementcounter {
            string fio_address;
        };

        int64_t stoi(const char *s) //1000000000000
        {
            int64_t i;
            i = 0;
            while (*s >= '0' && *s <= '9') {
                i = i * 10 + (*s - '0');
                s++;
            }

            return i;
        }
        /***
         * this action will record a send using Obt. the input json will be verified, if verification fails an exception will be thrown.
         * if verification succeeds then status tables will be updated...
         */
        // @abi action
        [[eosio::action]]
        void recordsend(
                const string fio_request_id,
                const string &payer_fio_address,
                const string &payee_fio_address,
                const string &content,
                uint64_t max_fee,
                const string &actor,
                const string &tpid) {


            //check that names were found in the json.
            fio_400_assert(payer_fio_address.length() > 0, "payer_fio_address", payer_fio_address,
                           "from fio address not found in obt json blob", ErrorInvalidJsonInput);
            fio_400_assert(payee_fio_address.length() > 0, "payee_fio_address", payee_fio_address,
                           "to fio address not found in obt json blob", ErrorInvalidJsonInput);

            FioAddress payerfa;
            getFioAddressStruct(payer_fio_address, payerfa);

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
                    fr.id = fiorequestStatusTable.available_primary_key();
                    fr.fio_request_id = requestId;
                    fr.status = static_cast<int64_t >(trxstatus::sent_to_blockchain);
                    fr.metadata = "";
                    fr.time_stamp = currentTime;
                });
            }

            uint32_t present_time = now();

            //check the payer address, see that its a valid fio name
            uint128_t nameHash = string_to_uint128_hash(payer_fio_address.c_str());
            auto namesbyname = fionames.get_index<"byname"_n>();
            auto fioname_iter = namesbyname.find(nameHash);
            fio_400_assert(fioname_iter != namesbyname.end(), "payer_fio_address", payer_fio_address,
                           "No such FIO Address",
                           ErrorFioNameNotReg);
            uint64_t account = fioname_iter->owner_account;
            uint64_t payernameexp = fioname_iter->expiration;

            fio_400_assert(present_time <= payernameexp, "payer_fio_address", payer_fio_address,
                           "FIO Address expired", ErrorFioNameExpired);

            //check domain.
            uint128_t domHash = string_to_uint128_hash(payerfa.fiodomain.c_str());

            auto domainsbyname = domains.get_index<"byname"_n>();
            auto iterdom = domainsbyname.find(domHash);

            fio_400_assert(iterdom != domainsbyname.end(), "payer_fio_address", payer_fio_address,
                           "No such domain",
                           ErrorDomainNotRegistered);
            uint64_t domexp = iterdom->expiration;
            fio_400_assert(present_time <= domexp, "payer_fio_address", payer_fio_address,
                           "FIO Domain expired", ErrorFioNameExpired);





            //check the payee address, see that its a valid fio name
            nameHash = string_to_uint128_hash(payee_fio_address.c_str());
            namesbyname = fionames.get_index<"byname"_n>();
            fioname_iter = namesbyname.find(nameHash);

            fio_400_assert(fioname_iter != namesbyname.end(), "payee_fio_address", payee_fio_address,
                           "No such FIO Address",
                           ErrorFioNameNotReg);

            name aactor = name(actor.c_str());
            print("account: ", account, " actor: ", aactor, "\n");
            fio_403_assert(account == aactor.value, ErrorSignature);


            //begin new fees, bundle eligible fee logic
            uint128_t endpoint_hash = string_to_uint128_hash("record_send");

            auto fees_by_endpoint = fiofees.get_index<"byendpoint"_n>();
            auto fee_iter = fees_by_endpoint.find(endpoint_hash);
            //if the fee isnt found for the endpoint, then 400 error.
            fio_400_assert(fee_iter != fees_by_endpoint.end(), "endpoint_name", "record_send",
                           "FIO fee not found for endpoint", ErrorNoEndpoint);

            uint64_t reg_amount = fee_iter->suf_amount;
            uint64_t fee_type = fee_iter->type;

            //if its not a bundleeligible fee then this is an error.
            fio_400_assert(fee_type == 1, "fee_type", to_string(fee_type),
                           "record_send unexpected fee type for endpoint record_send, expected 1", ErrorNoEndpoint);


            uint64_t bundleeligiblecountdown = fioname_iter->bundleeligiblecountdown;

            uint64_t fee_amount = 0;

            if (bundleeligiblecountdown > 0) {
                //fee is zero, and decrement the counter.
                fee_amount = 0;
                action{
                        permission_level{_self, "active"_n},
                        "fio.system"_n,
                        "decrcounter"_n,
                        decrementcounter{
                                .fio_address = payer_fio_address
                        }
                }.send();

            } else {
                fee_amount = fee_iter->suf_amount;
                fio_400_assert(max_fee >= fee_amount, "max_fee", to_string(max_fee), "Fee exceeds supplied maximum.",
                               ErrorMaxFeeExceeded);

                //NOTE -- question here, should we always record the transfer for the fees, even when its zero,
                //or should we do as this code does and not do a transaction when the fees are 0.
                asset reg_fee_asset = asset();
                reg_fee_asset.amount = fee_amount;
                reg_fee_asset.symbol = symbol("FIO", 9);

                fio_fees(aactor, reg_fee_asset);
                process_rewards(tpid, fee_amount, get_self());

                //MAS-522 remove staking from voting
                if (fee_amount > 0) {
                    //MAS-522 remove staking from voting.
                    INLINE_ACTION_SENDER(eosiosystem::system_contract, updatepower)
                            ("eosio"_n, {{_self, "active"_n}},
                             {aactor, true}
                            );
                }


            }
            //end new fees, bundle eligible fee logic

            nlohmann::json json = {{"status",        "sent_to_blockchain"},
                                   {"fee_collected", fee_amount}};

            send_response(json.dump().c_str());
        }

        inline void fio_fees(const name &actor, const asset &fee) const {
            if (appConfig.pmtson) {
                // check for funds is implicitly done as part of the funds transfer.
                print("Collecting FIO API fees: ", fee);
                action(permission_level{actor, "active"_n},
                       TokenContract, "transfer"_n,
                       make_tuple(actor, "fio.treasury"_n, fee,
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
        [[eosio::action]]
        void newfundsreq(
                const string &payer_fio_address,
                const string &payee_fio_address,
                const string &content,
                uint64_t max_fee,
                const string &actor,
                const string &tpid) {


            //check that names were found in the json.
            fio_400_assert(payer_fio_address.length() > 0, "payer_fio_address", payer_fio_address,
                           "from fio address not specified",
                           ErrorInvalidJsonInput);
            fio_400_assert(payee_fio_address.length() > 0, "payee_fio_address", payee_fio_address,
                           "to fio address not specified",
                           ErrorInvalidJsonInput);
            FioAddress payerfa;
            getFioAddressStruct(payer_fio_address, payerfa);

            FioAddress payeefa;
            getFioAddressStruct(payee_fio_address, payeefa);

            uint32_t present_time = now();

            //check the payer address, see that its a valid fio name
            uint128_t nameHash = string_to_uint128_hash(payer_fio_address.c_str());
            auto namesbyname = fionames.get_index<"byname"_n>();
            auto fioname_iter = namesbyname.find(nameHash);
            fio_400_assert(fioname_iter != namesbyname.end(), "payer_fio_address", payer_fio_address,
                           "No such FIO Address",
                           ErrorFioNameNotReg);

            //check the payee address, see that its a valid fio name
            nameHash = string_to_uint128_hash(payee_fio_address.c_str());
            namesbyname = fionames.get_index<"byname"_n>();
            fioname_iter = namesbyname.find(nameHash);

            fio_400_assert(fioname_iter != namesbyname.end(), "payee_fio_address", payee_fio_address,
                           "No such FIO Address",
                           ErrorFioNameNotReg);

            uint64_t payeenameexp = fioname_iter->expiration;

            fio_400_assert(present_time <= payeenameexp, "payee_fio_address", payee_fio_address,
                           "FIO Address expired", ErrorFioNameExpired);

            //check domain.
            uint128_t domHash = string_to_uint128_hash(payeefa.fiodomain.c_str());
            auto domainsbyname = domains.get_index<"byname"_n>();
            auto iterdom = domainsbyname.find(domHash);

            fio_400_assert(iterdom != domainsbyname.end(), "payee_fio_address", payee_fio_address,
                           "No such domain",
                           ErrorDomainNotRegistered);
            uint64_t domexp = iterdom->expiration;
            fio_400_assert(present_time <= domexp, "payee_fio_address", payee_fio_address,
                           "FIO Domain expired", ErrorFioNameExpired);


            //payee must be the actor.
            uint64_t account = fioname_iter->owner_account;

            name aActor = name(actor.c_str());
            print("account: ", account, " actor: ", aActor, "\n");
            fio_403_assert(account == aActor.value, ErrorSignature);

            //put the thing into the table get the index.
            uint64_t id = fiorequestContextsTable.available_primary_key();
            uint64_t currentTime = now();
            uint128_t toHash = string_to_uint128_hash(payee_fio_address.c_str());
            uint128_t fromHash = string_to_uint128_hash(payer_fio_address.c_str());
            string toHashStr = "0x"+ to_hex((char *)&toHash,sizeof(toHash));
            string fromHashStr = "0x"+ to_hex((char *)&fromHash,sizeof(fromHash));


            //insert a send record into the status table using this id.
            fiorequestContextsTable.emplace(_self, [&](struct fioreqctxt &frc) {
                frc.fio_request_id = id;
                frc.payer_fio_address = fromHash;
                frc.payee_fio_address = toHash;
                frc.payer_fio_address_hex_str = fromHashStr;
                frc.payee_fio_address_hex_str = toHashStr;
                frc.content = content;
                frc.time_stamp = currentTime;
                frc.payer_fio_addr = payer_fio_address;
                frc.payee_fio_addr = payee_fio_address;
            });

            //begin new fees, bundle eligible fee logic
            uint128_t endpoint_hash = string_to_uint128_hash("new_funds_request");
            string mystr = "0x"+ to_hex((char *)&endpoint_hash,sizeof(endpoint_hash));
            print ("EDEDEDEDEDEDEDEDEDEDEDED endpoint hash new_funds_request is ",mystr);

            auto fees_by_endpoint = fiofees.get_index<"byendpoint"_n>();
            auto fee_iter = fees_by_endpoint.find(endpoint_hash);
            //if the fee isnt found for the endpoint, then 400 error.
            fio_400_assert(fee_iter != fees_by_endpoint.end(), "endpoint_name", "new_funds_request",
                           "FIO fee not found for endpoint", ErrorNoEndpoint);

            uint64_t reg_amount = fee_iter->suf_amount;
            uint64_t fee_type = fee_iter->type;

            //if its not a bundleeligible fee then this is an error.
            fio_400_assert(fee_type == 1, "fee_type", to_string(fee_type),
                           "new_funds_request unexpected fee type for endpoint new_funds_request, expected 1",
                           ErrorNoEndpoint);


            uint64_t bundleeligiblecountdown = fioname_iter->bundleeligiblecountdown;

            uint64_t fee_amount = 0;

            if (bundleeligiblecountdown > 0) {
                //fee is zero, and decrement the counter.
                fee_amount = 0;
                action{
                        permission_level{_self, "active"_n},
                        "fio.system"_n,
                        "decrcounter"_n,
                        decrementcounter{
                                .fio_address = payee_fio_address
                        }
                }.send();

            } else {
                fee_amount = fee_iter->suf_amount;
                fio_400_assert(max_fee >= fee_amount, "max_fee", to_string(max_fee), "Fee exceeds supplied maximum.",
                               ErrorMaxFeeExceeded);

                //NOTE -- question here, should we always record the transfer for the fees, even when its zero,
                //or should we do as this code does and not do a transaction when the fees are 0.
                asset reg_fee_asset = asset();
                reg_fee_asset.symbol = symbol("FIO", 9);
                reg_fee_asset.amount = fee_amount;

                fio_fees(aActor, reg_fee_asset);
                process_rewards(tpid, fee_amount, get_self());

                //MAS-522 remove staking from voting
                if (fee_amount > 0) {
                    //MAS-522 remove staking from voting.
                    INLINE_ACTION_SENDER(eosiosystem::system_contract, updatepower)
                            ("eosio"_n, {{_self, "active"_n}},
                             {aActor, true}
                            );
                }

            }
            //end new fees, bundle eligible fee logic

            nlohmann::json json = {{"fio_request_id", id},
                                   {"status",         "requested"},
                                   {"fee_collected",  fee_amount}};

            send_response(json.dump().c_str());
        }

        /***
         * this action will add a rejection status to the specified request id.
         * the input fiorequest id will be verified to ensure there is a request in the contexts table matching this id
         * before the status record is added to the index tables.
         f*/
        // @abi action
        [[eosio::action]]
        void rejectfndreq(const string &fio_request_id, uint64_t max_fee, const string &actor, const string &tpid) {

            print("call reject funds request\n");

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

            uint128_t payer128FioAddHashed =fioreqctx_iter->payer_fio_address;

            uint32_t present_time = now();

            //check the payer address, see that its a valid fio name

            auto namesbyname = fionames.get_index<"byname"_n>();
            auto fioname_iter = namesbyname.find(payer128FioAddHashed);

            fio_403_assert(fioname_iter != namesbyname.end(), ErrorSignature);
            uint64_t account = fioname_iter->owner_account;
            uint64_t payernameexp = fioname_iter->expiration;
            string payerFioAddress = fioname_iter->name;
            FioAddress payerfa;
            getFioAddressStruct(payerFioAddress, payerfa);


            fio_400_assert(present_time <= payernameexp, "payer_fio_address", payerFioAddress,
                           "FIO Address expired", ErrorFioNameExpired);

            //check domain.
            uint128_t domHash = string_to_uint128_hash(payerfa.fiodomain.c_str());
            auto domainsbyname = domains.get_index<"byname"_n>();
            auto iterdom = domainsbyname.find(domHash);

            fio_400_assert(iterdom != domainsbyname.end(), "payer_fio_address", payerFioAddress,
                           "No such domain",
                           ErrorDomainNotRegistered);
            uint64_t domexp = iterdom->expiration;
            fio_400_assert(present_time <= domexp, "payer_fio_address", payerFioAddress,
                           "FIO Domain expired", ErrorFioNameExpired);


            string payer_fio_address = fioname_iter->name;

            name aactor = name(actor.c_str());
            print("account: ", account, " actor: ", aactor, "\n");
            fio_403_assert(account == aactor.value, ErrorSignature);

            //insert a send record into the status table using this id.
            fiorequestStatusTable.emplace(_self, [&](struct fioreqsts &fr) {
                fr.id = fiorequestStatusTable.available_primary_key();;
                fr.fio_request_id = requestId;
                fr.status = static_cast<int64_t >(trxstatus::rejected);
                fr.metadata = "";
                fr.time_stamp = currentTime;
            });

            //begin new fees, bundle eligible fee logic

            uint128_t endpoint_hash = string_to_uint128_hash("reject_funds_request");

            auto fees_by_endpoint = fiofees.get_index<"byendpoint"_n>();
            auto fee_iter = fees_by_endpoint.find(endpoint_hash);
            //if the fee isnt found for the endpoint, then 400 error.
            fio_400_assert(fee_iter != fees_by_endpoint.end(), "endpoint_name", "reject_funds_request",
                           "FIO fee not found for endpoint", ErrorNoEndpoint);

            uint64_t reg_amount = fee_iter->suf_amount;
            uint64_t fee_type = fee_iter->type;

            //if its not a bundleeligible fee then this is an error.
            fio_400_assert(fee_type == 1, "fee_type", to_string(fee_type),
                           "reject_funds_request unexpected fee type for endpoint reject_funds_request, expected 1",
                           ErrorNoEndpoint);


            uint64_t bundleeligiblecountdown = fioname_iter->bundleeligiblecountdown;

            uint64_t fee_amount = 0;

            if (bundleeligiblecountdown > 0) {
                //fee is zero, and decrement the counter.
                fee_amount = 0;
                action{
                        permission_level{_self, "active"_n},
                        "fio.system"_n,
                        "decrcounter"_n,
                        decrementcounter{
                                .fio_address = payer_fio_address
                        }
                }.send();

            } else {
                fee_amount = fee_iter->suf_amount;
                fio_400_assert(max_fee >= fee_amount, "max_fee", to_string(max_fee),
                               "Fee exceeds supplied maximum.",
                               ErrorMaxFeeExceeded);

                //NOTE -- question here, should we always record the transfer for the fees, even when its zero,
                //or should we do as this code does and not do a transaction when the fees are 0.
                asset reg_fee_asset = asset();
                reg_fee_asset.amount = fee_amount;
                reg_fee_asset.symbol = symbol("FIO", 9);

                fio_fees(aactor, reg_fee_asset);
                process_rewards(tpid, fee_amount, get_self());

                //MAS-522 remove staking from voting
                if (fee_amount > 0) {
                    //MAS-522 remove staking from voting.
                    INLINE_ACTION_SENDER(eosiosystem::system_contract, updatepower)
                            ("eosio"_n, {{_self, "active"_n}},
                             {aactor, true}
                            );
                }

            }
            //end new fees, bundle eligible fee logic

            nlohmann::json json = {{"status",        "request_rejected"},
                                   {"fee_collected", fee_amount}};
            send_response(json.dump().c_str());
        }
    };

    EOSIO_DISPATCH(FioRequestObt, (recordsend)(newfundsreq)
    (rejectfndreq))
}
