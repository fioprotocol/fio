/** FioFee implementation file
 *  Description: FioFee is the smart contract that manages fees.
 *  @author Adam Androulidakis, Casey Gardiner, Ed Rotthoff
 *  @modifedby
 *  @file fio.fee.cpp
 *  @copyright Dapix
 */

#include "fio.fee.hpp"
#include "../fio.name/fio.name.hpp"
#include <fio.common/fio.common.hpp>
#include <fio.common/fioerror.hpp>
#include <eosio/native/intrinsics.hpp>
#include <fio.common/json.hpp>
#include <string>
#include <map>

using std::string;

namespace fioio {

    /***
     * This contract maintains fee related actions and data. It provides voting actions which allow the
     * setting of fee amounts for mandatory fees, and the setting of bundle counts for bundled fees
     * by the active block producers. It provides datga structures representing the fee construct within
     * the FIO protocol. It provides the computation of fees from the voting
     * results of block producers.
     */
    class [[eosio::contract("FioFee")]]  FioFee : public eosio::contract {

    private:
        fiofee_table fiofees;
        feevoters_table feevoters;
        bundlevoters_table bundlevoters;
        feevotes_table feevotes;
        eosiosystem::producers_table producers;

        /*******
        * This method will compute the median fee from the fee votes that are cast.
        *
        * @param feevalues
        * @param fee_endpoint
        * @param fee_endpoint_hash
        */
        void
        compute_median_and_update_fees(vector <uint64_t> feevalues, string fee_endpoint, uint128_t fee_endpoint_hash) {
            bool dbgout = false;
            //one more time
            if (feevalues.size() > 0) {
                uint64_t median_fee = 0;
                //sort it.
                sort(feevalues.begin(), feevalues.end());
                //if the number of values is odd use the middle one.
                if ((feevalues.size() % 2) == 1) {
                    int useIdx = (feevalues.size() / 2);
                    if (dbgout) {
                        print(" odd size is ", feevalues.size(), " using index for median ", useIdx, "\n");
                    }
                    median_fee = feevalues[useIdx];
                } else {//even number in the list. use the middle 2
                    int useIdx = (feevalues.size() / 2) - 1;
                    if (dbgout) {
                        print(" even size is ", feevalues.size(), " using index for median ", useIdx, "\n");
                    }
                    median_fee = (feevalues[useIdx] + feevalues[useIdx + 1]) / 2;
                }
                //update the fee.
                auto feesbyendpoint = fiofees.get_index<"byendpoint"_n>();
                auto fee_iter = feesbyendpoint.find(fee_endpoint_hash);
                if (fee_iter != feesbyendpoint.end()) {
                    if (dbgout) {
                        print(" updating ", fee_iter->end_point, " to have fee ", median_fee, "\n");
                    }
                    feesbyendpoint.modify(fee_iter, _self, [&](struct fiofee &ff) {
                        ff.suf_amount = median_fee;
                    });
                } else {
                    if (dbgout) {
                        print(" fee endpoint does not exist in fiofees for endpoint ", fee_endpoint,
                              " computed median is ", median_fee, "\n");
                    }
                }
            }
        }




    public:
        using contract::contract;

        FioFee(name s, name code, datastream<const char *> ds)
                : contract(s, code, ds),
                  fiofees(_self, _self.value),
                  bundlevoters(_self, _self.value),
                  feevoters(_self, _self.value),
                  feevotes(_self, _self.value),
                  producers("eosio"_n, name("eosio").value) {
        }

        /*********
         * This action provides the ability to set a fee vote by a block producer.
         * the user submits a list of feevalue objects each of which contains a suf amount
         * (to which a multiplier will later be applied, and an endpoint name which must
         * match one of the endpoint names in the fees table.
         * @param fee_values this is a vector of feevalue objects each has the
         *                     endpoint name and desired fee in FIO SUF
         * @param actor this is the string rep of the fio account for the user making the call, a block producer.
         */
        // @abi action
        [[eosio::action]]
        void setfeevote(const vector <feevalue> &fee_values, const string &actor) {

            name aactor = name(actor.c_str());
            require_auth(aactor);

            bool dbgout = true;


            //validate the actor.
            //check that the actor is in the block producers.
            auto prod_iter = producers.find(aactor.value);
            fio_400_assert(prod_iter != producers.end(), "actor", actor,
                           " Not an active BP",
                           ErrorFioNameNotReg);

            //check that the producer is active
            bool is_active = prod_iter->is_active;
            fio_400_assert(is_active, "actor", actor,
                           " Not an active BP",
                           ErrorFioNameNotReg);

            uint32_t nowtime = now();

            //check the submitted fee values.
            for (auto &feeval : fee_values) {
                //check the endpoint exists for this fee
                uint128_t endPointHash = string_to_uint128_hash(feeval.end_point.c_str());
                if (dbgout) {
                    print("\nendPointHash: ", endPointHash, "\n");
                }
                auto feesbyendpoint = fiofees.get_index<"byendpoint"_n>();
                fio_400_assert(feesbyendpoint.find(endPointHash) != feesbyendpoint.end(), "end_point", feeval.end_point,
                               "invalid end_point", ErrorEndpointNotFound);

                fio_400_assert(feeval.value >= 0, "fee_value", feeval.end_point,
                               "invalid fee value", ErrorFeeInvalid);

                //get all the votes made by this actor. go through the list
                //and find the fee vote to update.
                auto feevotesbybpname = feevotes.get_index<"bybpname"_n>();
                auto votebyname_iter = feevotesbybpname.lower_bound(aactor.value);

                uint64_t idtoremove;
                bool found = false;
                bool timeviolation = false;
                while (votebyname_iter != feevotesbybpname.end())
                {
                    if (votebyname_iter->block_producer_name.value != aactor.value) {
                        //if the bp name changes we have exited the items of interest, so quit.
                        break;
                    }

                    if (votebyname_iter->end_point_hash == endPointHash) {
                        //check the time of the last update, remove and replace if
                        //its been longer than the time between votes
                        uint32_t lastupdate = votebyname_iter->lastvotetimestamp;

                        //be silent if the time between votes has not yet elapsed.
                        if (lastupdate > (nowtime - TIME_BETWEEN_VOTES_SECONDS)) {
                            timeviolation = true;
                            break;
                        } else {
                            idtoremove = votebyname_iter->id;
                            found = true;
                            break;
                        }
                    }
                    votebyname_iter++;
                }

                if (found) {
                    auto myiter = feevotes.find(idtoremove);
                    feevotes.erase(myiter);
                }

                if (!timeviolation) {
                    feevotes.emplace(_self, [&](struct feevote &fv) {
                        fv.id = feevotes.available_primary_key();
                        fv.block_producer_name = aactor;
                        fv.end_point = feeval.end_point;
                        fv.end_point_hash = endPointHash;
                        fv.suf_amount = feeval.value;
                        fv.lastvotetimestamp = nowtime;
                    });
                }
            }
            updatefees();

            nlohmann::json json = {{"status", "OK"}};
            send_response(json.dump().c_str());
        }

        /**********
      * This method will update the fees based upon the present votes made by producers.
      */
        [[eosio::action]]
        void updatefees() {

            print("called update fees.", "\n");
            map<string, double> producer_fee_multipliers_map;

            bool dbgout = false;

            //get the producers from the producers table, only use producers with is_active set true.
            //for the active producers. make a map for each producer and its associated multiplier
            // for use in performing the multiplications later,
            auto prod_iter = producers.lower_bound(0);
            while (prod_iter != producers.end()) {
                if (prod_iter->is_active) {
                    if (dbgout) {
                        print(" found active producer", prod_iter->owner, "\n");
                    }
                    auto voters_iter = feevoters.find(prod_iter->owner.value);
                    string v1 = prod_iter->owner.to_string();

                    if (voters_iter != feevoters.end()) {
                        if (dbgout) {
                            print(" adding producer to multiplier map", prod_iter->owner, "\n");
                        }
                        producer_fee_multipliers_map.insert(make_pair(v1, voters_iter->fee_multiplier));
                    }
                }
                prod_iter++;
            }

            auto feevotesbyendpoint = feevotes.get_index<"byendpoint"_n>();
            string lastvalUsed = "";
            uint64_t lastusedHash;
            vector <uint64_t> feevalues;
            //traverse all of the fee votes grouped by endpoint.
            for (const auto &vote_item : feevotesbyendpoint) {
                //if we have changed the endpoint name then we are in the next endpoints grouping,
                // so compute median fee for this endpoint and then clear the list.
                if (vote_item.end_point.compare(lastvalUsed) != 0) {
                    compute_median_and_update_fees(feevalues, lastvalUsed, lastusedHash);

                    feevalues.clear();
                }
                lastvalUsed = vote_item.end_point;
                lastusedHash = vote_item.end_point_hash;

                //if the vote item block producer name is in the multiplier map, then multiply
                //the suf amount by the multiplier and add it to the list of feevalues to be
                //averaged
                if (producer_fee_multipliers_map.find(vote_item.block_producer_name.to_string()) !=
                    producer_fee_multipliers_map.end()) {

                    //note -- we protect against both overflow and negative values here, an
                    //overflow error should result computing the dresult,and we check if the
                    //result is negative.
                    double dresult =  producer_fee_multipliers_map[vote_item.block_producer_name.to_string()] *
                                      (double) vote_item.suf_amount;
                    fio_403_assert(dresult > 0, ErrorInvalidMultiplier);

                    uint64_t voted_fee = (uint64_t) (dresult);
                    feevalues.push_back(voted_fee);
                }
            }

            //compute the median on the remaining feevalues, this remains to be
            //processed after we have gone through the loop.
            compute_median_and_update_fees(feevalues, lastvalUsed, lastusedHash);
        }



       /********
        * This action allows block producers to vote for the number of transactions that will be permitted
        * for free in the FIO bundled transaction model.
        * @param bundled_transactions the number of bundled transactions per FIO user.
        * @param actor the block producer actor that is presently voting, the signer of this tx.
        */
        // @abi action
        [[eosio::action]]
        void bundlevote(
                int64_t bundled_transactions,
                const string &actor
        ) {
            name aactor = name(actor.c_str());
            require_auth(aactor);


            auto prod_iter = producers.find(aactor.value);

            fio_400_assert(prod_iter != producers.end(), "actor", actor,
                           " Not an active BP",
                           ErrorFioNameNotReg);

            bool is_active = prod_iter->is_active;
            fio_400_assert(is_active, "actor", actor,
                           " Not an active BP",
                           ErrorFioNameNotReg);

            fio_400_assert(bundled_transactions > 0, "bundled_transactions", to_string(bundled_transactions),
                           " Must be positive",
                           ErrorFioNameNotReg);

            uint32_t nowtime = now();

            auto voter_iter = bundlevoters.find(aactor.value);
            if (voter_iter != bundlevoters.end()) //update if it exists
            {
                uint32_t lastupdate = voter_iter->lastvotetimestamp;
                if (lastupdate <= (nowtime - TIME_BETWEEN_VOTES_SECONDS)) {
                    bundlevoters.modify(voter_iter, _self, [&](struct bundlevoter &a) {
                        a.block_producer_name = aactor;
                        a.bundledbvotenumber = bundled_transactions;
                        a.lastvotetimestamp = nowtime;
                    });
                }
            } else {
                bundlevoters.emplace(_self, [&](struct bundlevoter &f) {
                    f.block_producer_name = aactor;
                    f.bundledbvotenumber = bundled_transactions;
                    f.lastvotetimestamp = nowtime;
                });
            }

            nlohmann::json json = {{"status", "OK"}};

            send_response(json.dump().c_str());
            print("done bundlevote.", "\n");
        }


        /**********
         * This action will create a new feevoters record if the specified block producer does not yet exist in the
         * feevoters table,
         * it will verify that the producer making the request is a present active block producer, it will update the
         * feevoters record if a pre-existing feevoters record exists.
         * @param multiplier this is the multiplier that will be applied to all fee votes for this producer before
         * computing the median fee.
         * @param actor this is the block producer voter.
         */
        // @abi action
        [[eosio::action]]
        void setfeemult(
                double multiplier,
                const string &actor
        ) {

            name aactor = name(actor.c_str());
            require_auth(aactor);

            auto prod_iter = producers.find(aactor.value);

            fio_400_assert(prod_iter != producers.end(), "actor", actor,
                           " Not an active BP",
                           ErrorFioNameNotReg);

            bool is_active = prod_iter->is_active;
            fio_400_assert(is_active, "actor", actor,
                           " Not an active BP",
                           ErrorFioNameNotReg);

            fio_400_assert(multiplier > 0, "multiplier", to_string(multiplier),
                           " Must be positive",
                           ErrorFioNameNotReg);

            uint32_t nowtime = now();

            auto voter_iter = feevoters.find(aactor.value);
            if (voter_iter != feevoters.end())
            {
                uint32_t lastupdate = voter_iter->lastvotetimestamp;
                if (lastupdate <= (nowtime - 120)) {
                    feevoters.modify(voter_iter, _self, [&](struct feevoter &a) {
                        a.block_producer_name = aactor;
                        a.fee_multiplier = multiplier;
                        a.lastvotetimestamp = nowtime;
                    });
                }
            } else {
                feevoters.emplace(_self, [&](struct feevoter &f) {
                    f.block_producer_name = aactor;
                    f.fee_multiplier = multiplier;
                    f.lastvotetimestamp = nowtime;
                });
            }

            updatefees();

            nlohmann::json json = {{"status", "OK"}};
            send_response(json.dump().c_str());
        }


        /*******
         * This action will create a new fee on the FIO protocol.
         * @param end_point  this is the api endpoint name associated with the fee
         * @param type consult feetype, mandatory is 0 bundle eligible is 1
         * @param suf_amount this is the fee amount in FIO SUFs
         */
        // @abi action
        [[eosio::action]]
        void create(
                string end_point,
                int64_t type,
                int64_t suf_amount
        ) {
            require_auth(_self);

            fio_400_assert(suf_amount >= 0, "suf_amount", to_string(suf_amount),
                           " invalid suf amount",
                           ErrorFeeInvalid);

            uint128_t endPointHash = string_to_uint128_hash(end_point.c_str());
            uint64_t fee_id = fiofees.available_primary_key();

            fiofees.emplace(_self, [&](struct fiofee &f) {
                f.fee_id = fee_id;
                f.end_point = end_point;
                f.end_point_hash = endPointHash;
                f.type = type;
                f.suf_amount = suf_amount;
            });
        }
    };

    EOSIO_DISPATCH(FioFee, (setfeevote)(bundlevote)(setfeemult)(updatefees)
    (create)
    )
}
