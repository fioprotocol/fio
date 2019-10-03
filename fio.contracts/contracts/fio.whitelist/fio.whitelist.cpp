/** FioWhitelist implementation file
 *  Description: FioWhitelist smart contract controls whitelisting on the FIO Blockchain.
 *  @author Ed Rotthoff
 *  @modifedby
 *  @file fio.whitelist.cpp
 *  @copyright Dapix
 */

#include <fio.fee/fio.fee.hpp>
#include "fio.whitelist.hpp"

namespace fioio {

    class [[eosio::contract("FIOWhitelist")]]  FIOWhitelist : public eosio::contract {

    private:

        whitelist_table whitelist;
        fiofee_table fiofees;
        config appConfig;
        fionames_table fionames;
        domains_table domains;
        eosio_names_table accountmap;

    public:
        using contract::contract;

        FIOWhitelist(name s, name code, datastream<const char *> ds) : contract(s, code, ds),
                                                                       whitelist(_self, _self.value),
                                                                       fiofees(FeeContract, FeeContract.value),
                                                                       fionames(SystemContract, SystemContract.value),
                                                                       domains(SystemContract, SystemContract.value),
                                                                       accountmap(fioio::SystemContract,
                                                                                  fioio::SystemContract.value) {
            configs_singleton configsSingleton(FeeContract, FeeContract.value);
            appConfig = configsSingleton.get_or_default(config());
        }

        inline void fio_fees(const name &actor, const asset &fee) const {
            if (appConfig.pmtson) {
                // check for funds is implicitly done as part of the funds transfer.
                print("Collecting FIO API fees: ", fee, "\n");
                action(permission_level{actor, "active"_n},
                       TokenContract, "transfer"_n,
                       make_tuple(actor, "fio.treasury"_n, fee,
                                  string("FIO API fees. Thank you."))
                ).send();
            } else {
                print("Payments currently disabled.", "\n");
            }
        }

        struct decrementcounter {
            string fio_address;
        };

        // @abi action
        [[eosio::action]]
        void addwhitelist(string fio_public_key_hash,
                          const string &content,
                          uint64_t max_fee,
                          const string &tpid,
                          const name &actor) {

            bool dbgout = false;

            print("called addwhitelist.", "\n");

            require_auth(actor);

            uint64_t hashed_str = string_to_uint64_hash(fio_public_key_hash.c_str());
            auto whitelistbylookup = whitelist.get_index<"bylookupidx"_n>();
            auto key_iter = whitelistbylookup.find(hashed_str);

            if (dbgout) {
                print("looking for pub key hash in whitelist.", "\n");
            }

            fio_400_assert(key_iter == whitelistbylookup.end(), "fio_public_key_hash", fio_public_key_hash,
                           "FIO public key already in whitelist", ErrorPublicKeyExists);
            if (dbgout) {
                print("pub key hash not found in whitelist, adding info to whitelist.", "\n");
            }

            uint64_t id = whitelist.available_primary_key();
            //add it.
            whitelist.emplace(_self, [&](struct whitelist_info &wi) {
                wi.id = id;
                wi.owner = actor.value;
                wi.lookupindex = fio_public_key_hash;
                wi.lookupindex_hash = hashed_str;
                wi.content = content;
            });

            //begin new fees, bundle eligible fee logic
            uint128_t endpoint_hash = string_to_uint128_hash("add_to_whitelist");

            if (dbgout) {
                print("processing add to whitelist fee.", "\n");
            }
            auto fees_by_endpoint = fiofees.get_index<"byendpoint"_n>();
            auto fee_iter = fees_by_endpoint.find(endpoint_hash);
            //if the fee isnt found for the endpoint, then 400 error.
            fio_400_assert(fee_iter != fees_by_endpoint.end(), "endpoint_name", "add_to_whitelist",
                           "FIO fee not found for endpoint", ErrorNoEndpoint);

            uint64_t reg_amount = fee_iter->suf_amount;
            uint64_t fee_type = fee_iter->type;

            //if its not a bundleeligible fee then this is an error.
            fio_400_assert(fee_type == 1, "fee_type", to_string(fee_type),
                           "record_send unexpected fee type for endpoint, expected 1", ErrorNoEndpoint);

            if (dbgout) {
                print("whitelist fee is. ", to_string(reg_amount), "\n");
            }

            auto fionames_byowner = fionames.get_index<"byowner"_n>();
            auto fioname_iter = fionames_byowner.find(actor.value);
            fio_404_assert(fioname_iter != fionames_byowner.end(), "FIO Address owner not found",
                           ErrorFioNameNotRegistered);

            uint64_t bundleeligiblecountdown = fioname_iter->bundleeligiblecountdown;
            string fio_address = fioname_iter->name;

            uint64_t fee_amount = 0;

            if (bundleeligiblecountdown > 0) {
                //fee is zero, and decrement the counter.
                fee_amount = 0;
                if (dbgout) {
                    print("calling decrcounter.", "\n");
                }
                action{
                        permission_level{_self, "active"_n},
                        "fio.system"_n,
                        "decrcounter"_n,
                        decrementcounter{
                                .fio_address = fio_address
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

                fio_fees(actor, reg_fee_asset);

                process_rewards(tpid, fee_amount, get_self());

                //MAS-522 remove staking from voting
                if (fee_amount > 0) {
                    //MAS-522 remove staking from voting.
                    INLINE_ACTION_SENDER(eosiosystem::system_contract, updatepower)
                            ("eosio"_n, {{_self, "active"_n}},
                             {actor, true}
                            );
                }
            }
            //end new fees, bundle eligible fee logic

            nlohmann::json json = {{"status",        "OK"},
                                   {"fee_collected", fee_amount}};
            send_response(json.dump().c_str());
        }

        // @abi action
        [[eosio::action]]
        void remwhitelist(string fio_public_key_hash,
                          uint64_t max_fee,
                          const string &tpid,
                          const name &actor) {


            bool dbgout = true;

            require_auth(actor);

            uint64_t hashed_str = string_to_uint64_hash(fio_public_key_hash.c_str());
            auto whitelistbylookup = whitelist.get_index<"bylookupidx"_n>();
            auto key_iter = whitelistbylookup.find(hashed_str);
            if (dbgout) {
                print("looking for pub key hash in whitelist.", "\n");
            }


            fio_400_assert(key_iter != whitelistbylookup.end(), "fio_public_key_hash", fio_public_key_hash,
                           "FIO public key not in whitelist", ErrorPublicKeyExists);

            if (dbgout) {
                print("pub key hash found in whitelist, removing info from whitelist.", "\n");
            }

            //remove it.
            whitelistbylookup.erase(key_iter);

            //begin new fees, bundle eligible fee logic
            uint128_t endpoint_hash = string_to_uint128_hash("remove_from_whitelist");

            if (dbgout) {
                print("processing remove from whitelist fee.", "\n");
            }
            auto fees_by_endpoint = fiofees.get_index<"byendpoint"_n>();
            auto fee_iter = fees_by_endpoint.find(endpoint_hash);
            //if the fee isnt found for the endpoint, then 400 error.
            fio_400_assert(fee_iter != fees_by_endpoint.end(), "endpoint_name", "remove_from_whitelist",
                           "FIO fee not found for endpoint", ErrorNoEndpoint);

            uint64_t reg_amount = fee_iter->suf_amount;
            uint64_t fee_type = fee_iter->type;

            //if its not a bundleeligible fee then this is an error.
            fio_400_assert(fee_type == 1, "fee_type", to_string(fee_type),
                           " unexpected fee type for endpoint, expected 1", ErrorNoEndpoint);

            if (dbgout) {
                print("whitelist fee is. ", to_string(reg_amount), "\n");
            }

            auto fionames_byowner = fionames.get_index<"byowner"_n>();
            auto fioname_iter = fionames_byowner.find(actor.value);
            fio_404_assert(fioname_iter != fionames_byowner.end(), "FIO Address owner not found",
                           ErrorFioNameNotRegistered);

            uint64_t bundleeligiblecountdown = fioname_iter->bundleeligiblecountdown;
            string fio_address = fioname_iter->name;

            uint64_t fee_amount = 0;

            if (bundleeligiblecountdown > 0) {
                //fee is zero, and decrement the counter.
                fee_amount = 0;
                if (dbgout) {
                    print("calling decrcounter.", "\n");
                }
                action{
                        permission_level{_self, "active"_n},
                        "fio.system"_n,
                        "decrcounter"_n,
                        decrementcounter{
                                .fio_address = fio_address
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

                fio_fees(actor, reg_fee_asset);

                process_rewards(tpid, fee_amount, get_self());

                //MAS-522 remove staking from voting
                if (fee_amount > 0) {
                    //MAS-522 remove staking from voting.
                    INLINE_ACTION_SENDER(eosiosystem::system_contract, updatepower)
                            ("eosio"_n, {{_self, "active"_n}},
                             {actor, true}
                            );
                }
            }
            //end new fees, bundle eligible fee logic

            nlohmann::json json = {{"status",        "OK"},
                                   {"fee_collected", fee_amount}};
            send_response(json.dump().c_str());
        }
    };

    EOSIO_DISPATCH(FIOWhitelist, (addwhitelist)(remwhitelist)
    )
}
