/** FioTPID implementation file
 *  Description:
 *  @author Adam Androulidakis, Ed Rotthoff
 *  @modifedby
 *  @file fio.tpid.cpp
 *  @license FIO Foundation ( https://github.com/fioprotocol/fio/blob/master/LICENSE ) Dapix
 */

#include "fio.tpid.hpp"

namespace fioio {

class [[eosio::contract("TPIDController")]]  TPIDController: public eosio::contract {

private:
        tpids_table tpids;
        fionames_table fionames;
        eosiosystem::voters_table voters;
        bounties_table bounties;
        bool debugout = false;

public:
        using contract::contract;

        TPIDController(name s, name code, datastream<const char *> ds) :
                contract(s, code, ds), tpids(_self, _self.value), bounties(_self, _self.value),
                fionames(AddressContract, AddressContract.value),
                voters(AddressContract, AddressContract.value) {
        }

        // this action will perform the logic of checking the voter_info,
        // and setting the proxy and auto proxy for auto proxy.
        inline void autoproxy(name proxy_name, name owner_name) {

                //check the voter_info table for a record matching owner_name.
                auto votersbyowner = voters.get_index<"byowner"_n>();
                const auto viter = votersbyowner.find(owner_name.value);
                if (viter == votersbyowner.end()) {
                        //if the record is not there then send inline action to crautoprx (a new action in the system contract).
                        //note this action will set the auto proxy and is_aut_proxy, so return after.
                        INLINE_ACTION_SENDER(eosiosystem::system_contract, crautoproxy)(
                                "eosio"_n, {{get_self(), "active"_n}},
                                {proxy_name, owner_name});
                        return;
                } else {
                        //check if the record has auto proxy and proxy matching proxy_name, set has_proxy. if so return.
                        if (viter->is_auto_proxy) {
                                if (proxy_name == viter->proxy) {
                                        return;
                                }
                        } else if ((viter->proxy) || (viter->producers.size() > 0) || viter->is_proxy) {
                                //check if the record has another proxy or producers. if so return.
                                return;
                        }

                        //invoke the fio.address contract action to set auto proxy and proxy name.
                        action(
                                permission_level{get_self(), "active"_n},
                                "eosio"_n,
                                "setautoproxy"_n,
                                std::make_tuple(proxy_name, owner_name)
                                ).send();
                }
        }

        inline void process_auto_proxy(const string &tpid, name owner) {
                uint128_t hashname = string_to_uint128_hash(tpid.c_str());
                const auto tpidsbyname = tpids.get_index<"byname"_n>();
                auto iter = tpidsbyname.find(hashname);
                if (iter != tpidsbyname.end()) {
                    if (debugout) {
                        print("process auto proxy found tpid ", tpid, "\n");
                    }
                        //tpid exists, use the info to find the owner of the tpid
                        auto namesbyname = fionames.get_index<"byname"_n>();
                        auto iternm = namesbyname.find(iter->fioaddhash);
                        if (iternm != namesbyname.end()) {
                                name proxy_name = name(iternm->owner_account);
                                //do the auto proxy
                                autoproxy(proxy_name, owner);
                        }
                }
                else{
                    if (debugout) {
                        print("process auto proxy did not find tpid ", tpid, "\n");
                    }
                }
        }

        //Condition: check if tpid exists in fionames before executing.
        //This call should only be made by processrewards and processbucketrewards in fio.rewards.hpp
        //which should also only be called by contracts that collect fees
        //@abi action
        [[eosio::action]]
        void updatetpid(const string &tpid, const name owner, const uint64_t &amount) {

                eosio_assert(has_auth(AddressContract) || has_auth(TokenContract) || has_auth(TREASURYACCOUNT) ||
                             has_auth("fio.reqobt"_n) || has_auth("eosio"_n),
                             "missing required authority of fio.address, fio.treasury, fio.token, eosio or fio.reqobt");
            if (debugout) {
                print("update tpid calling updatetpid with tpid ", tpid, " owner ", owner, "\n");
            }
                const auto tpidhash = string_to_uint128_hash(tpid.c_str());
                auto tpidsbyname = tpids.get_index<"byname"_n>();
                if (tpidsbyname.find(tpidhash) == tpidsbyname.end()) {

                        const auto id = tpids.available_primary_key();
                        tpids.emplace(owner, [&](struct tpid &f) {
                                        f.id = id;
                                        f.fioaddress = tpid;
                                        f.fioaddhash = tpidhash;
                                        f.rewards = 0;
                                });

                }
                process_auto_proxy(tpid, owner);
                //Update existing tpid amount or amount of tpid that was just created before
                tpidsbyname.modify(tpidsbyname.find(tpidhash), get_self(), [&](struct tpid &f) {
                                f.rewards += amount;
                        });

        } //updatetpid

        //This action can only be called by fio.treasury after successful rewards payment to tpid
        //@abi action
        [[eosio::action]]
        void rewardspaid(const string &tpid) {
                require_auth(TREASURYACCOUNT);
                auto tpidsbyname = tpids.get_index<"byname"_n>();
                const auto tpidfound = tpidsbyname.find(string_to_uint128_hash(tpid.c_str()));

                if (tpidfound != tpidsbyname.end()) {
                        tpidsbyname.modify(tpidfound, _self, [&](struct tpid &f) {
                                        f.rewards = 0;
                                });
                }
        }

        //Must be called at least once at genesis for tokensminted check in fio.rewards.hpp
        //@abi action
        [[eosio::action]]
        void updatebounty(const uint64_t &amount) {
                eosio_assert((has_auth(TPIDContract) || has_auth(TREASURYACCOUNT)),
                             "missing required authority of fio.tpid, or fio.treasury");
                bounties.set(bounties.exists() ? bounty{bounties.get().tokensminted + amount} : bounty{amount}, _self);
        }
};     //class TPIDController

EOSIO_DISPATCH(TPIDController, (updatetpid)(rewardspaid)
               (updatebounty))
}
