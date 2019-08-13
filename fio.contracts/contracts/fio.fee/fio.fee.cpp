
#include "fio.fee.hpp"
#include "../fio.name/fio.name.hpp"
#include <fio.common/fio.common.hpp>
#include <fio.common/fioerror.hpp>
#include <eosio/native/intrinsics.hpp>
#include <fio.common/json.hpp>
#include <string>

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

            uint32_t nowtime = now();

            //next look in the feevoter table and see if there is already an entry.
            //if there is then update it, if not then emplace it.

            auto voter_iter = feevoters.find(aactor.value);
            fio_400_assert(voter_iter != feevoters.end(), "actor", actor,
                           " does not exist in the feevoters table.",
                           ErrorFioNameNotReg);

            uint32_t lastupdate = voter_iter->lastvotetimestamp;


            //be silent and dont update until 2 minutes goes by since last vote call.
            if (lastupdate > (nowtime - 120)) {
                return;
            }



           for(int i=0; i< fee_ratios.size();i++){
               uint64_t endPointHash = string_to_uint64_hash(fee_ratios[i].end_point.c_str());
               //look for this actor in the feevoters table, if not there error.
               //look for this actor in the feevotes table, it not there add the record.
               //  if there are records loop through the records, find teh matching endpoint and remove this record.

               auto feevotesbybpname = feevotes.get_index<"bybpname"_n>();
               auto vote_iter = feevotesbybpname.lower_bound(aactor.value);

               uint64_t idtoremove;
               bool found = false;
               while(vote_iter != feevotesbybpname.end()) //update if it exists
               {
                   if (vote_iter->block_producer_name.value != aactor.value) {
                       //exit loop if not found.
                       break;
                   }

                   if (vote_iter->end_point_hash == endPointHash) {
                       idtoremove = vote_iter->id;
                       found = true;
                       break;

                   }
                   vote_iter++;

               }

               if (found){
                   auto myiter = feevotes.find(idtoremove);
                   feevotes.erase(myiter);
               }

              feevotes.emplace(_self,[&] (struct feevote &fv){
                   fv.id = feevotes.available_primary_key();
                   fv.block_producer_name = aactor;
                   fv.end_point = fee_ratios[i].end_point;
                   fv.end_point_hash = endPointHash;
                   fv.suf_amount = fee_ratios[i].value;
               });


           }

            feevoters.modify(voter_iter, _self, [&](struct feevoter &a) {
                a.lastvotetimestamp = nowtime;
            });

        }










            /***
            * This action will create a new fee voter record if the specified block producer does not exist,
             * it will verify that the producer making the request is a present block producer, it will update the voter
             * record if a pre-existing voter record exists.
            */
        // @abi action
        [[eosio::action]]
        void setfeemult(
                uint64_t multiplier,      // this fee multiplier to be used for all fee votes, this will
                                          // be converted into a double value with 6 decimal places.
                const string &actor       //this is the block producer account name. and needs to be the account
                                          //associated with the signer of this request.
        ) {

            print("called setfeemult.", "\n");
           // require_auth(actor);

            print("verifying that the actor is a registered producer  ", actor);

            name aactor = name(actor.c_str());
            auto prod_iter = producers.find(aactor.value);
            //check if this actor is a registered producer.
            fio_400_assert(prod_iter != producers.end(), "actor", actor,
                           " Not an active BP",
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

    EOSIO_DISPATCH(FioFee, (setfeevote)(setfeemult)(create)
    )
} // namespace fioio
