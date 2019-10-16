/** Fio Request Obt implementation file
 *  Description: The FIO request obt contract supports request for funds and also may record other
 *  block chain transactions (such as send of funds from one FIO address to another).
 *  @author Adam Androulidakis, Casey Gardiner, Ed Rotthoff
 *  @file fio.request.obt.cpp
 *  @copyright Dapix
 *
 *  Changes:
 */

#include <eosiolib/asset.hpp>
#include "fio.request.obt.hpp"
#include <fio.address/fio.address.hpp>
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

        struct decrementcounter {
            string fio_address;
        };

        int64_t stoi(const char *s)
        {
            int64_t i;
            i = 0;
            while (*s >= '0' && *s <= '9') {
                i = i * 10 + (*s - '0');
                s++;
            }
            return i;
        }

        inline void fio_fees(const name &actor, const asset &fee) const {
            if (appConfig.pmtson) {
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

    public:
        explicit FioRequestObt(name s, name code, datastream<const char *> ds)
                : contract(s, code, ds),
                  fiorequestContextsTable(_self, _self.value),
                  fiorequestStatusTable(_self, _self.value),
                  fionames(AddressContract, AddressContract.value),
                  domains(AddressContract, AddressContract.value),
                  fiofees(FeeContract, FeeContract.value),
                  tpids(AddressContract, AddressContract.value) {
            configs_singleton configsSingleton(FeeContract, FeeContract.value);
            appConfig = configsSingleton.get_or_default(config());
        }


         /*******
          * This action will record the send of funds from one FIO address to another, either
          * in response to a request for funds or as a result of a direct send of funds from
          * one user to another
          * @param fio_request_id   This is the one up id of the fio request
          * @param payer_fio_address The payer of the request
          * @param payee_fio_address  The payee (recieve of funds) of the request.
          * @param content  this is the encrypted blob of content containing details of the request.
          * @param max_fee  this is maximum fee the user is willing to pay as a result of this transaction.
          * @param actor  this is the actor (the account which has signed this transaction)
          * @param tpid  this is the tpid for the owner of the domain (this is optional)
          */
        // @abi action
        [[eosio::action]]
        void recordsend(
                const string &fio_request_id,
                const string &payer_fio_address,
                const string &payee_fio_address,
                const string &content,
                const int64_t &max_fee,
                const string &actor,
                const string &tpid) {

            name aactor = name(actor.c_str());
            require_auth(aactor);

            fio_400_assert(max_fee >= 0, "max_fee", to_string(max_fee), "Invalid fee value",
                           ErrorMaxFeeInvalid);

            fio_400_assert(payer_fio_address.length() > 0, "payer_fio_address", payer_fio_address,
                           "from fio address not found", ErrorInvalidJsonInput);
            fio_400_assert(payee_fio_address.length() > 0, "payee_fio_address", payee_fio_address,
                           "to fio address not found", ErrorInvalidJsonInput);

            fio_400_assert(content.size() >= 64 && content.size() <= 432, "content", content,
                           "Requires min 64 max 432 size", ErrorContentLimit);

            FioAddress payerfa;
            getFioAddressStruct(payer_fio_address, payerfa);

            uint32_t present_time = now();

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

            uint128_t domHash = string_to_uint128_hash(payerfa.fiodomain.c_str());

            auto domainsbyname = domains.get_index<"byname"_n>();
            auto iterdom = domainsbyname.find(domHash);

            fio_400_assert(iterdom != domainsbyname.end(), "payer_fio_address", payer_fio_address,
                           "No such domain",
                           ErrorDomainNotRegistered);
            uint64_t domexp = iterdom->expiration;
            fio_400_assert(present_time <= domexp, "payer_fio_address", payer_fio_address,
                           "FIO Domain expired", ErrorFioNameExpired);

            nameHash = string_to_uint128_hash(payee_fio_address.c_str());
            namesbyname = fionames.get_index<"byname"_n>();
            fioname_iter = namesbyname.find(nameHash);

            fio_400_assert(fioname_iter != namesbyname.end(), "payee_fio_address", payee_fio_address,
                           "No such FIO Address",
                           ErrorFioNameNotReg);


            print("account: ", account, " actor: ", aactor, "\n");
            fio_403_assert(account == aactor.value, ErrorSignature);

            //begin fees, bundle eligible fee logic
            uint128_t endpoint_hash = string_to_uint128_hash("record_send");

            auto fees_by_endpoint = fiofees.get_index<"byendpoint"_n>();
            auto fee_iter = fees_by_endpoint.find(endpoint_hash);

            fio_400_assert(fee_iter != fees_by_endpoint.end(), "endpoint_name", "record_send",
                           "FIO fee not found for endpoint", ErrorNoEndpoint);

            uint64_t reg_amount = fee_iter->suf_amount;
            uint64_t fee_type = fee_iter->type;

            fio_400_assert(fee_type == 1, "fee_type", to_string(fee_type),
                           "record_send unexpected fee type for endpoint record_send, expected 1", ErrorNoEndpoint);

            uint64_t bundleeligiblecountdown = fioname_iter->bundleeligiblecountdown;
            uint64_t fee_amount = 0;

            if (bundleeligiblecountdown > 0) {
                fee_amount = 0;
                action{
                        permission_level{_self, "active"_n},
                        AddressContract,
                        "decrcounter"_n,
                        decrementcounter{
                                .fio_address = payer_fio_address
                        }
                }.send();
            } else {
                fee_amount = fee_iter->suf_amount;
                fio_400_assert(max_fee >= (int64_t)fee_amount, "max_fee", to_string(max_fee), "Fee exceeds supplied maximum.",
                               ErrorMaxFeeExceeded);

                asset reg_fee_asset = asset();
                reg_fee_asset.amount = fee_amount;
                reg_fee_asset.symbol = symbol("FIO", 9);

                fio_fees(aactor, reg_fee_asset);
                process_rewards(tpid, fee_amount, get_self());

                if (fee_amount > 0) {
                    INLINE_ACTION_SENDER(eosiosystem::system_contract, updatepower)
                            ("eosio"_n, {{_self, "active"_n}},
                             {aactor, true}
                            );
                }
            }
            //end fees, bundle eligible fee logic


            if (fio_request_id.length() > 0) {
                uint64_t currentTime = current_time();
                uint64_t requestId;

                requestId = std::atoi(fio_request_id.c_str());

                auto fioreqctx_iter = fiorequestContextsTable.find(requestId);
                fio_400_assert(fioreqctx_iter != fiorequestContextsTable.end(), "fio_request_id", fio_request_id,
                               "No such FIO Request ", ErrorRequestContextNotFound);

                fiorequestStatusTable.emplace(_self, [&](struct fioreqsts &fr) {
                    fr.id = fiorequestStatusTable.available_primary_key();
                    fr.fio_request_id = requestId;
                    fr.status = static_cast<int64_t >(trxstatus::sent_to_blockchain);
                    fr.metadata = "";
                    fr.time_stamp = currentTime;
                });
            }

            nlohmann::json json = {{"status",        "sent_to_blockchain"},
                                   {"fee_collected", fee_amount}};

            send_response(json.dump().c_str());
        }



       /*********
        * This action will record a request for funds into the FIO protocol.
        * @param payer_fio_address this is the fio address of the payer of the request for funds.
        * @param payee_fio_address this is the requestor of the funds (or the payee) for this request for funds.
        * @param content  this is the blob of encrypted data associated with this request for funds.
        * @param max_fee  this is the maximum fee that the sender of this transaction is willing to pay for this tx.
        * @param actor this is the string representation of the fio account that has signed this transaction
        * @param tpid
        */
        // @abi action
        [[eosio::action]]
        void newfundsreq(
                const string &payer_fio_address,
                const string &payee_fio_address,
                const string &content,
                const int64_t &max_fee,
                const string &actor,
                const string &tpid) {

            name aActor = name(actor.c_str());
            require_auth(aActor);

            fio_400_assert(max_fee >= 0, "max_fee", to_string(max_fee), "Invalid fee value",
                           ErrorMaxFeeInvalid);

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

            uint128_t nameHash = string_to_uint128_hash(payer_fio_address.c_str());
            auto namesbyname = fionames.get_index<"byname"_n>();
            auto fioname_iter = namesbyname.find(nameHash);
            fio_400_assert(fioname_iter != namesbyname.end(), "payer_fio_address", payer_fio_address,
                           "No such FIO Address",
                           ErrorFioNameNotReg);

            nameHash = string_to_uint128_hash(payee_fio_address.c_str());
            namesbyname = fionames.get_index<"byname"_n>();
            fioname_iter = namesbyname.find(nameHash);

            fio_400_assert(fioname_iter != namesbyname.end(), "payee_fio_address", payee_fio_address,
                           "No such FIO Address",
                           ErrorFioNameNotReg);

            uint64_t payeenameexp = fioname_iter->expiration;

            fio_400_assert(present_time <= payeenameexp, "payee_fio_address", payee_fio_address,
                           "FIO Address expired", ErrorFioNameExpired);

            uint128_t domHash = string_to_uint128_hash(payeefa.fiodomain.c_str());
            auto domainsbyname = domains.get_index<"byname"_n>();
            auto iterdom = domainsbyname.find(domHash);

            fio_400_assert(iterdom != domainsbyname.end(), "payee_fio_address", payee_fio_address,
                           "No such domain",
                           ErrorDomainNotRegistered);
            uint64_t domexp = iterdom->expiration;
            fio_400_assert(present_time <= domexp, "payee_fio_address", payee_fio_address,
                           "FIO Domain expired", ErrorFioNameExpired);

            uint64_t account = fioname_iter->owner_account;


            print("account: ", account, " actor: ", aActor, "\n");
            fio_403_assert(account == aActor.value, ErrorSignature);

            //begin fees, bundle eligible fee logic
            uint128_t endpoint_hash = string_to_uint128_hash("new_funds_request");
            auto fees_by_endpoint = fiofees.get_index<"byendpoint"_n>();
            auto fee_iter = fees_by_endpoint.find(endpoint_hash);

            fio_400_assert(fee_iter != fees_by_endpoint.end(), "endpoint_name", "new_funds_request",
                           "FIO fee not found for endpoint", ErrorNoEndpoint);

            uint64_t reg_amount = fee_iter->suf_amount;
            uint64_t fee_type = fee_iter->type;

            fio_400_assert(fee_type == 1, "fee_type", to_string(fee_type),
                           "new_funds_request unexpected fee type for endpoint new_funds_request, expected 1",
                           ErrorNoEndpoint);

            uint64_t bundleeligiblecountdown = fioname_iter->bundleeligiblecountdown;
            uint64_t fee_amount = 0;

            if (bundleeligiblecountdown > 0) {
                fee_amount = 0;
                action{
                        permission_level{_self, "active"_n},
                        AddressContract,
                        "decrcounter"_n,
                        decrementcounter{
                                .fio_address = payee_fio_address
                        }
                }.send();
            } else {
                fee_amount = fee_iter->suf_amount;
                fio_400_assert(max_fee >= (int64_t)fee_amount, "max_fee", to_string(max_fee), "Fee exceeds supplied maximum.",
                               ErrorMaxFeeExceeded);

                asset reg_fee_asset = asset();
                reg_fee_asset.symbol = symbol("FIO", 9);
                reg_fee_asset.amount = fee_amount;

                fio_fees(aActor, reg_fee_asset);
                process_rewards(tpid, fee_amount, get_self());

                if (fee_amount > 0) {
                    INLINE_ACTION_SENDER(eosiosystem::system_contract, updatepower)
                            ("eosio"_n, {{_self, "active"_n}},
                             {aActor, true}
                            );
                }
            }
            //end fees, bundle eligible fee logic

            uint64_t id = fiorequestContextsTable.available_primary_key();
            uint64_t currentTime = now();
            uint128_t toHash = string_to_uint128_hash(payee_fio_address.c_str());
            uint128_t fromHash = string_to_uint128_hash(payer_fio_address.c_str());
            string toHashStr = "0x" + to_hex((char *) &toHash, sizeof(toHash));
            string fromHashStr = "0x" + to_hex((char *) &fromHash, sizeof(fromHash));

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

            nlohmann::json json = {{"fio_request_id", id},
                                   {"status",         "requested"},
                                   {"fee_collected",  fee_amount}};

            send_response(json.dump().c_str());
        }


         /********
          * this action will add a rejection status to the request for funds with the specified request id.
          * the input fiorequest id will be verified to ensure there is a request in the contexts table matching this id
          * before the status record is added to the index tables.
          * @param fio_request_id this is the id of the request in the fio request contexts table.
          * @param max_fee  this is the maximum fee that the sender of this transaction is willing to pay.
          * @param actor  this is the string representation of the FIO account that is associated with the signer of this tx.
          * @param tpid  this is the fio address of the domain owner associated with this request.
          */
        // @abi action
        [[eosio::action]]
        void rejectfndreq(
                const string &fio_request_id,
                const int64_t &max_fee,
                const string &actor,
                const string &tpid) {

            print("call reject funds request\n");
            name aactor = name(actor.c_str());
            require_auth(aactor);

            fio_400_assert(max_fee >= 0, "max_fee", to_string(max_fee), "Invalid fee value",
                           ErrorMaxFeeInvalid);

            fio_400_assert(fio_request_id.length() > 0, "fio_request_id", fio_request_id, "No value specified",
                           ErrorRequestContextNotFound);

            uint64_t currentTime = current_time();
            uint64_t requestId;

            requestId = std::atoi(fio_request_id.c_str());

            auto fioreqctx_iter = fiorequestContextsTable.find(requestId);
            fio_400_assert(fioreqctx_iter != fiorequestContextsTable.end(), "fio_request_id", fio_request_id,
                           "No such FIO Request", ErrorRequestContextNotFound);

            uint128_t payer128FioAddHashed = fioreqctx_iter->payer_fio_address;
            uint32_t present_time = now();

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


            print("account: ", account, " actor: ", aactor, "\n");
            fio_403_assert(account == aactor.value, ErrorSignature);

            //begin fees, bundle eligible fee logic
            uint128_t endpoint_hash = string_to_uint128_hash("reject_funds_request");

            auto fees_by_endpoint = fiofees.get_index<"byendpoint"_n>();
            auto fee_iter = fees_by_endpoint.find(endpoint_hash);

            fio_400_assert(fee_iter != fees_by_endpoint.end(), "endpoint_name", "reject_funds_request",
                           "FIO fee not found for endpoint", ErrorNoEndpoint);

            uint64_t reg_amount = fee_iter->suf_amount;
            uint64_t fee_type = fee_iter->type;

            fio_400_assert(fee_type == 1, "fee_type", to_string(fee_type),
                           "reject_funds_request unexpected fee type for endpoint reject_funds_request, expected 1",
                           ErrorNoEndpoint);

            uint64_t bundleeligiblecountdown = fioname_iter->bundleeligiblecountdown;
            uint64_t fee_amount = 0;

            if (bundleeligiblecountdown > 0) {
                fee_amount = 0;
                action{
                        permission_level{_self, "active"_n},
                        AddressContract,
                        "decrcounter"_n,
                        decrementcounter{
                                .fio_address = payer_fio_address
                        }
                }.send();
            } else {
                fee_amount = fee_iter->suf_amount;
                fio_400_assert(max_fee >= (uint64_t)fee_amount, "max_fee", to_string(max_fee),
                               "Fee exceeds supplied maximum.",
                               ErrorMaxFeeExceeded);

                asset reg_fee_asset = asset();
                reg_fee_asset.amount = fee_amount;
                reg_fee_asset.symbol = symbol("FIO", 9);

                fio_fees(aactor, reg_fee_asset);
                process_rewards(tpid, fee_amount, get_self());

                if (fee_amount > 0) {
                    INLINE_ACTION_SENDER(eosiosystem::system_contract, updatepower)
                            ("eosio"_n, {{_self, "active"_n}},
                             {aactor, true}
                            );
                }
            }
            //end fees, bundle eligible fee logic

            fiorequestStatusTable.emplace(_self, [&](struct fioreqsts &fr) {
                fr.id = fiorequestStatusTable.available_primary_key();;
                fr.fio_request_id = requestId;
                fr.status = static_cast<int64_t >(trxstatus::rejected);
                fr.metadata = "";
                fr.time_stamp = currentTime;
            });


            nlohmann::json json = {{"status",        "request_rejected"},
                                   {"fee_collected", fee_amount}};
            send_response(json.dump().c_str());
        }
    };

    EOSIO_DISPATCH(FioRequestObt, (recordsend)(newfundsreq)
    (rejectfndreq))
}
