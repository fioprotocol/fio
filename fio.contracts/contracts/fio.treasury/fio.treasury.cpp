/** FioTreasury implementation file
 *  Description: FioTreasury smart contract controls block producer and tpid payments.
 *  @author Adam Androulidakis, Ed Rotthoff, Casey Gardiner
 *  @modifedby
 *  @file fio.treasury.cpp
 *  @license FIO Foundation ( https://github.com/fioprotocol/fio/blob/master/LICENSE ) Dapix
 */

#define REWARDMAX       100000000000            // 100 FIO
#define FDTNMAXTOMINT   150000000000000         // 150,000 FIO
#define BPMAXTOMINT     50000000000000          // 50,000  FIO
#define FDTNMAXRESERVE  181253654000000000      // 181,253,654 FIO
#define BPMAXRESERVE    10000000000000000       // 10,000,000 FIO
#define PAYSCHEDTIME    172801                  // 1 day  ( block time )
#define PAYABLETPIDS    100

#include "fio.treasury.hpp"

namespace fioio {

class [[eosio::contract("FIOTreasury")]]  FIOTreasury: public eosio::contract {

private:
        tpids_table tpids;
        fionames_table fionames;
        domains_table domains;
        rewards_table clockstate;
        bprewards_table bprewards;
        bpbucketpool_table bucketrewards;
        fdtnrewards_table fdtnrewards;
        voteshares_table voteshares;
        eosiosystem::eosio_global_state gstate;
        eosiosystem::global_state_singleton global;
        eosiosystem::producers_table producers;
        bool rewardspaid;
        uint64_t lasttpidpayout;

public:
        using contract::contract;

        FIOTreasury(name s, name code, datastream<const char *> ds) : contract(s, code, ds),
                tpids(TPIDContract, TPIDContract.value),
                fionames(AddressContract, AddressContract.value),
                domains(AddressContract, AddressContract.value),
                bprewards(_self, _self.value),
                clockstate(_self, _self.value),
                voteshares(_self, _self.value),
                producers(SYSTEMACCOUNT, SYSTEMACCOUNT.value),
                global(SYSTEMACCOUNT, SYSTEMACCOUNT.value),
                fdtnrewards(_self, _self.value),
                bucketrewards(_self, _self.value) {
        }

        // @abi action
        [[eosio::action]]
        void tpidclaim(const name &actor) {
                require_auth(actor);

                uint64_t tpids_paid = 0;
                auto clockiter = clockstate.begin();

                //This contract should only be able to iterate throughout the entire tpids table to
                //to check for rewards once every x blocks.
                fio_400_assert(now() > clockiter->lasttpidpayout + MINUTE, "tpidclaim", "tpidclaim",
                               "No work.", ErrorNoWork);

                for (const auto &itr : tpids) {
                        if (itr.rewards >= REWARDMAX) { //100 FIO (100,000,000,000 SUF)
                                auto namesbyname = fionames.get_index<"byname"_n>();
                                auto itrfio = namesbyname.find(string_to_uint128_hash(itr.fioaddress.c_str()));

                                // If the fioaddress exists (address could have been burned)
                                if (itrfio != namesbyname.end()) {
                                        action(permission_level{get_self(), "active"_n},
                                               TokenContract, "transfer"_n,
                                               make_tuple(TREASURYACCOUNT, name(itrfio->owner_account),
                                                          asset(itr.rewards, FIOSYMBOL),
                                                          string("Paying TPID from treasury."))
                                        ).send();
                                } else { //Allocate to BP buckets instead
                                        bprewards.set(bpreward{bprewards.get().rewards + itr.rewards}, _self);
                                }
                                action(permission_level{get_self(), "active"_n},
                                       "fio.tpid"_n, "rewardspaid"_n,
                                       make_tuple(itr.fioaddress)
                                ).send();
                                tpids_paid++;
                                if (tpids_paid >= PAYABLETPIDS) break; //only paying 100 tpids
                        } // endif itr.rewards >=
                } // for (const auto &itr : tpids)

                fio_400_assert(tpids_paid > 0, "tpidclaim", "tpidclaim","No work.", ErrorNoWork);

                //update the clock but only if there has been a tpid paid out.
                action(permission_level{get_self(), "active"_n},
                       get_self(), "updateclock"_n,
                       make_tuple()
                ).send();

                const string response_string = string("{\"status\": \"OK\",\"tpids_paid\":") +
                                         to_string(tpids_paid) + string("}");

               fio_400_assert(transaction_size() < MAX_TPIDCLAIM_TRANSACTION_SIZE, "transaction_size", std::to_string(transaction_size()),
                 "Transaction is too large", ErrorTransaction);

                send_response(response_string.c_str());
        } //tpid_claim

        // @abi action
        [[eosio::action]]
        void bpclaim(const string &fio_address, const name &actor) {
                require_auth(actor);

                gstate = global.get();
                check( gstate.total_voted_fio >= MINVOTEDFIO,
                       "cannot claim rewards until the chain voting threshold is exceeded" );

                auto namesbyname = fionames.get_index<"byname"_n>();
                auto fioiter = namesbyname.find(string_to_uint128_hash(fio_address.c_str()));

                fio_400_assert(fioiter != namesbyname.end(), "fio_address", fio_address,
                               "Invalid FIO Address", ErrorNoFioAddressProducer);

                const uint64_t producer = fioiter->owner_account;
                auto prodbyowner = producers.get_index<"byowner"_n>();
                auto proditer = prodbyowner.find(producer);

                fio_400_assert(proditer != prodbyowner.end(), "fio_address", fio_address,
                               "FIO Address not producer or nothing payable", ErrorNoFioAddressProducer);

                auto domainsbyname = domains.get_index<"byname"_n>();
                auto domiter = domainsbyname.find(fioiter->domainhash);

                //add 30 days to the domain expiration, this call will work until 30 days past expire.
                uint32_t expiration = domiter->expiration;
                expiration = get_time_plus_seconds(expiration,SECONDS30DAYS);

                fio_400_assert(now() < expiration, "domain", domiter->name,
                               "FIO Domain expired", ErrorDomainExpired);
                fio_400_assert(now() < fioiter->expiration, "fio_address", fio_address,
                               "FIO Address expired", ErrorFioNameExpired);

                /***************  Pay schedule expiration *******************/
                //if it has been 24 hours, transfer remaining producer vote_shares to the foundation and record the rewards back into bprewards,
                // then erase the pay schedule so a new one can be created in a subsequent call to bpclaim.
                auto clockiter = clockstate.begin();
                if (clockiter != clockstate.end() && now() >= clockiter->payschedtimer + PAYSCHEDTIME) { //+ 172801
                        if (std::distance(voteshares.begin(), voteshares.end()) > 0) {
                                auto iter = voteshares.begin();
                                while (iter != voteshares.end()) {
                                        iter = voteshares.erase(iter);
                                }
                        }
                }

                //*********** CREATE PAYSCHEDULE **************
                // If there is no pay schedule then create a new one
                if (std::distance(voteshares.begin(), voteshares.end()) == 0) { //if new payschedule
                        //Create the payment schedule
                        uint64_t bpcounter = 0;
                        auto proditer = producers.get_index<"prototalvote"_n>();
                        for (const auto &itr : proditer) {
                                if (itr.is_active) {
                                        voteshares.emplace(get_self(), [&](auto &p) {
                                                        p.owner = itr.owner;
                                                        p.votes = itr.total_votes;
                                                });
                                }
                                bpcounter++;
                                if (bpcounter > MAXBPS) break;
                        } // &itr : producers table

                        //Move 1/365 of the bucketpool to the bpshare
                        bprewards.set(bpreward{bprewards.get().rewards + static_cast<uint64_t>(bucketrewards.get().rewards / YEARDAYS)}, _self);
                        bucketrewards.set(bucketpool{bucketrewards.get().rewards - static_cast<uint64_t>(bucketrewards.get().rewards / YEARDAYS)}, _self);

                        if (clockiter->bpreservetokensminted < BPMAXRESERVE && bprewards.get().rewards < BPMAXTOMINT) {

                          uint64_t bptomint = BPMAXTOMINT;
                          const uint64_t bpremainingreserve = BPMAXRESERVE - clockiter->bpreservetokensminted;

                            if (bpremainingreserve < BPMAXTOMINT) {
                                    bptomint = bpremainingreserve;
                            }

                            if (clockiter->bpreservetokensminted + bptomint > BPMAXRESERVE) {
                              bptomint = BPMAXRESERVE - clockiter->bpreservetokensminted;
                            }
                            //Mint new tokens up to 50,000 FIO
                            action(permission_level{get_self(), "active"_n},
                                   TokenContract, "mintfio"_n,
                                   make_tuple(TREASURYACCOUNT,bptomint)
                                   ).send();

                            clockstate.modify(clockiter, get_self(), [&](auto &entry) {
                                            entry.bpreservetokensminted += bptomint;
                                    });

                            //Include the minted tokens in the reward payout
                            bprewards.set(bpreward{bprewards.get().rewards + bptomint}, _self);
                            //This new reward amount that has been minted will be appended to the rewards being divied up next
                        }
                        else
                        {
                          print("Block producers reserve minting exhausted");
                        }
                        //!!!rewards is now 0 in the bprewards table and can no longer be referred to. If needed use projectedpay

                        uint64_t fdtntomint = FDTNMAXTOMINT;
                        const uint64_t fdtnremainingreserve = FDTNMAXRESERVE - clockiter->fdtnreservetokensminted;

                        if (fdtnremainingreserve < FDTNMAXTOMINT) {
                                fdtntomint = fdtnremainingreserve;
                        }

                        if (clockiter->fdtnreservetokensminted < FDTNMAXRESERVE) {

                                //Mint new tokens up to 50,000 FIO
                                action(permission_level{get_self(), "active"_n},
                                       TokenContract, "mintfio"_n,
                                       make_tuple(FOUNDATIONACCOUNT,fdtntomint)
                                ).send();

                                clockstate.modify(clockiter, get_self(), [&](auto &entry) {
                                    entry.fdtnreservetokensminted += fdtntomint;
                                });

                        }
                        // All bps are now in pay schedule, calculate the shares
                        uint64_t bpcount = std::distance(voteshares.begin(), voteshares.end());
                        uint64_t abpcount = MAXACTIVEBPS;

                        if (bpcount > MAXBPS) bpcount = MAXBPS; //limit to 42 producers in voteshares
                        if (bpcount <= MAXACTIVEBPS) abpcount = bpcount;
                        auto bprewardstat = bprewards.get();
                        uint64_t tostandbybps = static_cast<uint64_t>(bprewardstat.rewards * .60);
                        uint64_t toactivebps = static_cast<uint64_t>(bprewardstat.rewards * .40);

                        bpcounter = 0;
                        for (const auto &itr : voteshares) {
                                if (bpcounter <= abpcount) {
                                        voteshares.modify(itr, get_self(), [&](auto &entry) {
                                                        entry.abpayshare = static_cast<uint64_t>(toactivebps / abpcount);;
                                                });
                                }
                                voteshares.modify(itr, get_self(), [&](auto &entry) {
                                                entry.sbpayshare =  static_cast<uint64_t>((tostandbybps) * (itr.votes / gstate.total_producer_vote_weight));;
                                        });
                                bpcounter++;
                        } // &itr : voteshares
                        //Start 24 track for daily pay
                        clockstate.modify(clockiter, get_self(), [&](auto &entry) {
                                        entry.payschedtimer = now();
                                });
                } //if new payschedule
                  //*********** END OF CREATE PAYSCHEDULE **************
                auto bpiter = voteshares.find(producer);
                prodbyowner = producers.get_index<"byowner"_n>();
                proditer = prodbyowner.find(producer);
                check(proditer != prodbyowner.end(),"producer not found");
                /******* Payouts *******/
                //This contract should only allow the producer to be able to claim rewards once every 172800 blocks (1 day).
                uint64_t payout = 0;

                if (bpiter != voteshares.end()) {
                        payout = static_cast<uint64_t>(bpiter->abpayshare + bpiter->sbpayshare);
                        check(proditer->is_active, "producer does not have an active key");

                        if (payout > 0) {
                                action(permission_level{get_self(), "active"_n},
                                       TokenContract, "transfer"_n,
                                       make_tuple(TREASURYACCOUNT, name(bpiter->owner), asset(payout, FIOSYMBOL),
                                                  string("Paying producer from treasury."))
                                       ).send();

                                // Reduce the producer's share of daily rewards and bucketrewards
                                if (bpiter->abpayshare > 0) {
                                        bprewards.set(bpreward{bprewards.get().rewards - payout}, _self);
                                }
                                //Keep track of rewards paid for reserve minting
                                clockstate.modify(clockiter, get_self(), [&](auto &entry) {
                                                entry.rewardspaid += payout;
                                        });
                                //Invoke system contract to reset producer last_claim_time and unpaid_blocks
                                action(permission_level{get_self(), "active"_n},
                                       AddressContract, "resetclaim"_n,
                                       make_tuple(producer)
                                       ).send();
                        }

                        // PAY FOUNDATION //
                        auto fdtnstate = fdtnrewards.get();
                        action(permission_level{get_self(), "active"_n},
                                TokenContract, "transfer"_n,
                                make_tuple(TREASURYACCOUNT, FOUNDATIONACCOUNT, asset(fdtnstate.rewards, FIOSYMBOL),
                                        string("Paying foundation from treasury."))).send();

                        //Clear the foundation rewards counter
                        fdtnrewards.set(fdtnreward{0}, _self);
                        //remove the producer from payschedule
                        voteshares.erase(bpiter);
                } //endif now() > bpiter + 172800

                const string response_string = string("{\"status\": \"OK\",\"amount\":") +
                                         to_string(payout) + string("}");

               fio_400_assert(transaction_size() < MAX_BPCLAIM_TRANSACTION_SIZE, "transaction_size", std::to_string(transaction_size()),
                 "Transaction is too large", ErrorTransaction);

                send_response(response_string.c_str());
        } //bpclaim

        // @abi action
        [[eosio::action]]
        void updateclock() {
                require_auth(TREASURYACCOUNT);

                auto clockiter = clockstate.begin();
                if (clockiter != clockstate.end()) {
                        clockstate.erase(clockiter);
                        clockstate.emplace(_self, [&](struct treasurystate &entry) {
                                        entry.lasttpidpayout = now();
                                });
                }
        }

        // @abi action
        [[eosio::action]]
        void startclock() {
                require_auth(TREASURYACCOUNT);

                if (std::distance(clockstate.begin(), clockstate.end()) == 0) {
                        clockstate.emplace(_self, [&](struct treasurystate &entry) {
                                        entry.lasttpidpayout = now();
                                        entry.payschedtimer = now();
                                });
                }

                bucketrewards.set(bucketpool{0}, _self);
                bprewdupdate(0);
        }

        // @abi action
        [[eosio::action]]
        void bprewdupdate(const uint64_t &amount) {

                eosio_assert((has_auth(AddressContract) || has_auth(TokenContract) || has_auth(TREASURYACCOUNT) ||
                             has_auth(REQOBTACCOUNT) || has_auth(SYSTEMACCOUNT) || has_auth(FeeContract)),
                             "missing required authority of fio.address, fio.treasury, fio.fee, fio.token, eosio or fio.reqobt");

                bprewards.set(bprewards.exists() ? bpreward{bprewards.get().rewards + amount} : bpreward{amount}, _self);
        }

        // @abi action
        [[eosio::action]]
        void bppoolupdate(const uint64_t &amount) {

                eosio_assert((has_auth(AddressContract) || has_auth(TokenContract) || has_auth(TREASURYACCOUNT) ||
                             has_auth(REQOBTACCOUNT)),
                             "missing required authority of fio.address, fio.treasury, fio.token, or fio.reqobt");
                bucketrewards.set(bucketrewards.exists() ? bucketpool{bucketrewards.get().rewards + amount} : bucketpool{amount}, _self);
        }

        // @abi action
        [[eosio::action]]
        void fdtnrwdupdat(const uint64_t &amount) {

                eosio_assert((has_auth(AddressContract) || has_auth(TokenContract) || has_auth(TREASURYACCOUNT) ||
                             has_auth(REQOBTACCOUNT) || has_auth(SYSTEMACCOUNT) || has_auth(FeeContract)),
                             "missing required authority of fio.address, fio.token, fio.fee, fio.treasury or fio.reqobt");

                fdtnrewards.set(fdtnrewards.exists() ? fdtnreward{fdtnrewards.get().rewards + amount} : fdtnreward{amount}, _self);

        }
};     //class FIOTreasury

EOSIO_DISPATCH(FIOTreasury, (tpidclaim)(updateclock)(startclock)(bprewdupdate)(fdtnrwdupdat)(bppoolupdate)
               (bpclaim))
}
