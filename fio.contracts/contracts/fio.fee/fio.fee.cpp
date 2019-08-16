
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
     * This contract maintains fee related actions and data. Most importantly its the host of the transaction fee
     * structure that is used by sister contracts to 
     */
    class [[eosio::contract("FioFee")]]  FioFee : public eosio::contract {

    private:
        fiofee_table fiofees;
        feevoters_table  feevoters;
        feevotes_table   feevotes;
        eosiosystem::producers_table producers;


    public:


        using contract::contract;

        FioFee(name s, name code, datastream<const char *> ds)
                : contract(s, code, ds),
                  fiofees(_self, _self.value),
                  feevoters(_self, _self.value),
                  feevotes(_self, _self.value),
                  producers("eosio"_n, name("eosio").value) {
        }



        /***
         * this action will record a send using Obt. the input json will be verified, if verification fails an exception will be thrown.
         * if verification succeeds then status tables will be updated...
         */
        // @abi action
        [[eosio::action]]
        void setfeevote(const vector<feevalue>& fee_ratios, const string &actor) {


            name aactor = name(actor.c_str());
            auto prod_iter = producers.find(aactor.value);

            //check if this actor is a registered producer.
            fio_400_assert(prod_iter != producers.end(), "actor", actor,
                           " Not an active BP",
                           ErrorFioNameNotReg);

            //add logic to check is active.
            bool is_active = prod_iter->is_active;
            fio_400_assert(is_active, "actor", actor,
                           " Not an active BP",
                           ErrorFioNameNotReg);


            uint32_t nowtime = now();

           for(int i=0; i< fee_ratios.size();i++){
               uint64_t endPointHash = string_to_uint64_hash(fee_ratios[i].end_point.c_str());
               //look for this actor in the feevoters table, if not there error.
               //look for this actor in the feevotes table, it not there add the record.
               //  if there are records loop through the records, find teh matching endpoint and remove this record.

               auto feevotesbybpname = feevotes.get_index<"bybpname"_n>();
               auto votebyname_iter = feevotesbybpname.lower_bound(aactor.value);

               uint64_t idtoremove;
               bool found = false;
               bool timeviolation = false;
               while(votebyname_iter != feevotesbybpname.end()) //update if it exists
               {
                   if (votebyname_iter->block_producer_name.value != aactor.value) {
                       //exit loop if not found.
                       break;
                   }

                   if (votebyname_iter->end_point_hash == endPointHash) {
                       //check the time, if the time is good then add to remove list
                       uint32_t lastupdate = votebyname_iter->lastvotetimestamp;

                       //be silent and dont update until 2 minutes goes by since last vote call.
                       if (lastupdate > (nowtime - 120)) {
                           timeviolation = true;
                           break;
                       }
                       else {
                           idtoremove = votebyname_iter->id;
                           found = true;
                           break;
                       }

                   }
                   votebyname_iter++;

               }

               if (found){
                   auto myiter = feevotes.find(idtoremove);
                   feevotes.erase(myiter);
               }

               if (!timeviolation) {
                   feevotes.emplace(_self, [&](struct feevote &fv) {
                       fv.id = feevotes.available_primary_key();
                       fv.block_producer_name = aactor;
                       fv.end_point = fee_ratios[i].end_point;
                       fv.end_point_hash = endPointHash;
                       fv.suf_amount = fee_ratios[i].value;
                       fv.lastvotetimestamp = nowtime;
                   });
               }
           }
           updatefees();

            nlohmann::json json = {{"status", "OK"}};

            send_response(json.dump().c_str());
        }

    void compute_median_and_update_fees(vector<uint64_t> feevalues, string fee_endpoint, uint64_t fee_endpoint_hash) {
            bool dbgout = false;
            //one more time
            if (feevalues.size() > 0) {
                uint64_t median_fee = 0;
                //sort it.
                sort(feevalues.begin(), feevalues.end());
                //if the number of values is odd use the middle one.
                if ((feevalues.size() % 2) == 1) {
                    int useIdx = (feevalues.size() / 2);
                    if(dbgout) {
                        print(" odd size is ", feevalues.size(), " using index for median ", useIdx, "\n");
                    }
                    median_fee = feevalues[useIdx];
                } else {//even number in the list. use the middle 2
                    int useIdx = (feevalues.size() / 2) - 1;
                    if(dbgout) {
                        print(" even size is ", feevalues.size(), " using index for median ", useIdx, "\n");
                    }
                    median_fee = (feevalues[useIdx] + feevalues[useIdx + 1]) / 2;
                }
                //update the fee.
                auto feesbyendpoint = fiofees.get_index<"byendpoint"_n>();
                auto fee_iter = feesbyendpoint.find(fee_endpoint_hash);
                if (fee_iter != feesbyendpoint.end()) {
                    if(dbgout) {
                        print(" updating ", fee_iter->end_point, " to have fee ", median_fee, "\n");
                    }
                    feesbyendpoint.modify(fee_iter, _self, [&](struct fiofee &ff) {
                        ff.suf_amount = median_fee;
                    });
                } else {
                    if(dbgout) {
                        print(" fee endpoint does not exist in fiofees for endpoint ", fee_endpoint,
                              " computed median is ", median_fee, "\n");
                    }
                }
            }
        }

        [[eosio::action]]
       void updatefees(){

           print("called update fees.","\n");
            map<string,double> producer_fee_multipliers_map;

            bool dbgout = false;

           //get the producers from the producers table, only use producers with is_active set true.
           //for the active producers. make a map for each producer and its associated multiplier
           // for use in performing the multiplications later,
           auto prod_iter = producers.lower_bound(0);
           while (prod_iter != producers.end()){
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
                       producer_fee_multipliers_map.insert(make_pair(v1,voters_iter->fee_multiplier));
                   }
               }
               prod_iter++;
           }

           auto feevotesbyendpoint = feevotes.get_index<"byendpoint"_n>();
           string lastvalUsed = "";
           uint64_t lastusedHash;
           vector<uint64_t> feevalues;
           for( const auto& vote_item : feevotesbyendpoint ) {
               if (vote_item.end_point.compare(lastvalUsed) != 0){

                   if (dbgout) {
                       print(" saw new endpoint name ", vote_item.end_point, "\n");
                   }
                   //we have just started a new endpoint.
                   //compute the median if the vector has elements.
                   compute_median_and_update_fees(feevalues,lastvalUsed,lastusedHash);

                   if (dbgout) {
                       print(" clearing the fee values", "\n");
                   }
                   //empty the vector.set the last used.
                   feevalues.clear();

               }
               lastvalUsed = vote_item.end_point;
               lastusedHash = vote_item.end_point_hash;
               //process the fee vote into the vector of fees.
               //check if this bp name is in the map, if so process the fee and add it to the vector.
               if(producer_fee_multipliers_map.find(vote_item.block_producer_name.to_string()) != producer_fee_multipliers_map.end()){
                   if (dbgout) {
                       print(" found multiplier ",
                             producer_fee_multipliers_map[vote_item.block_producer_name.to_string()],
                             " for ", vote_item.block_producer_name, "\n");
                   }

                   uint64_t voted_fee = (uint64_t)(producer_fee_multipliers_map[vote_item.block_producer_name.to_string()] *
                           (double)vote_item.suf_amount);
                   if(dbgout) {
                       print(" fee suf amount is ",
                             producer_fee_multipliers_map[vote_item.block_producer_name.to_string()],
                             " for ", vote_item.block_producer_name, "\n");
                   }
                   feevalues.push_back(voted_fee);
               }

           }

           //one more time to process the last set
           compute_median_and_update_fees(feevalues,lastvalUsed,lastusedHash);

            //get the fee votes sorted by endpoint hash,
            // traverse the sorted list, so we will traverse the list of all votes grouped by endpoint,
            // check each entry if it has a key in the multiplier map (IE is it an active producer),
            // if not move on to the next, if so then multiply the fee by the associated multiplier
            // and then add it to the vector of values to be averaged.
            //sort the vector of values, pick the middle one if its an odd number of values, pick the average of the
            //middle and middle+1 if its even number of values.
            // the result is a median value for the endpoint in question, update this value into the fees table.
            //do this process for each fee that has been voted upon.
        }







            /***
            * This action will create a new fee voter record if the specified block producer does not exist,
             * it will verify that the producer making the request is a present block producer, it will update the voter
             * record if a pre-existing voter record exists.
            */
        // @abi action
        [[eosio::action]]
        void setfeemult(
                double multiplier,      // this fee multiplier to be used for all fee votes, t
                const string &actor       //this is the block producer account name. and needs to be the account
                                          //associated with the signer of this request.
        ) {

            print("called setfeemult.", "\n");
           // require_auth(actor);

            name aactor = name(actor.c_str());
            auto prod_iter = producers.find(aactor.value);
            //check if this actor is a registered producer.
            fio_400_assert(prod_iter != producers.end(), "actor", actor,
                           " Not an active BP",
                           ErrorFioNameNotReg);

            fio_400_assert(multiplier > 0, "multiplier", to_string(multiplier),
                           " Must be positive",
                           ErrorFioNameNotReg);

           uint32_t nowtime = now();

            //next look in the feevoter table and see if there is already an entry.
            //if there is then update it, if not then emplace it.

            auto voter_iter = feevoters.find(aactor.value);
           if (voter_iter != feevoters.end()) //update if it exists
           {
               uint32_t lastupdate = voter_iter->lastvotetimestamp;
               if (lastupdate <= (nowtime - 120)) {
                   feevoters.modify(voter_iter, _self, [&](struct feevoter &a) {
                       a.block_producer_name = aactor;
                       a.fee_multiplier = multiplier;
                       a.lastvotetimestamp = nowtime;
                   });
               }

           }else{ //emplace it if its not there
               //emplace the values into the table
               feevoters.emplace(_self, [&](struct feevoter &f) {
                   f.block_producer_name = aactor;
                   f.fee_multiplier = multiplier;
                   f.lastvotetimestamp = nowtime;
               });
           }

           updatefees();

            nlohmann::json json = {{"status", "OK"}};

            send_response(json.dump().c_str());
            print("done setfeemult.", "\n");
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
        ) {

            print("called create fee.", "\n");
            require_auth(_self);

            print("creating name for end point ", end_point.c_str());

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

    EOSIO_DISPATCH(FioFee, (setfeevote)(setfeemult)(updatefees)(create)
    )
} // namespace fioio
