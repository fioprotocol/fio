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
#define PAYSCHEDTIME    86401                   //seconds per day + 1
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
        treasurystate state;
public:
        using contract::contract;
        FIOTreasury(name s, name code, datastream<const char *> ds) : contract(s, code, ds),
                tpids(TPIDContract, TPIDContract.value),
                fionames(AddressContract, AddressContract.value),
                domains(AddressContract, AddressContract.value),
                bprewards(get_self(), get_self().value),
                clockstate(get_self(), get_self().value),
                voteshares(get_self(), get_self().value),
                producers(SYSTEMACCOUNT, SYSTEMACCOUNT.value),
                global(SYSTEMACCOUNT, SYSTEMACCOUNT.value),
                fdtnrewards(get_self(), get_self().value),
                bucketrewards(get_self(), get_self().value) {
                  state = clockstate.get_or_default();
        }



        //FIOTreasury deconstructor sets the clockstate
        ~FIOTreasury() {
          clockstate.set(state, get_self());
        }

        // @abi action
        [[eosio::action]]
        void tpidclaim(const name &actor) {
                require_auth(actor);

                uint64_t tpids_paid = 0;

                //This contract should only be able to iterate throughout the entire tpids table to
                //to check for rewards once every x blocks.
                fio_400_assert(now() > state.lasttpidpayout + MINUTE, "tpidclaim", "tpidclaim",
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
                                        bprewards.set(bpreward{bprewards.get().rewards + itr.rewards}, get_self());
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
                state.lasttpidpayout = now();
                const string response_string = string("{\"status\": \"OK\",\"tpids_paid\":") +
                                         to_string(tpids_paid) + string("}");

               fio_400_assert(transaction_size() <= MAX_TRX_SIZE, "transaction_size", std::to_string(transaction_size()),
                 "Transaction is too large", ErrorTransactionTooLarge);

                send_response(response_string.c_str());
        } //tpid_claim

        // @abi action
        [[eosio::action]]
        void bpclaim(const string &fio_address, const name &actor) {
                require_auth(actor);

                gstate = global.get();
                check( gstate.total_voted_fio >= MINVOTEDFIO || gstate.thresh_voted_fio_time != time_point() ,
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

                fio_400_assert((now() - proditer->last_bpclaim) > SECONDSBETWEENBPCLAIM, "fio_address", fio_address,
                               "FIO Address not producer or nothing payable", ErrorNoFioAddressProducer);

                //Invoke system contract to update producer last_bpclaim time
                action(permission_level{get_self(), "active"_n},
                       SYSTEMACCOUNT, "updlbpclaim"_n,
                       make_tuple(producer)
                ).send();

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
                if (clockstate.exists() && now() >= state.payschedtimer + PAYSCHEDTIME) { //+ 172801
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
                    int64_t bpcounter = 0;
                    uint64_t activecount = 0;
                    //prototal votes returns active producers sorted beginning at the highest voted to the lowest voted
                    // active producers  then for inactive producers lowest voted to highest voted.
                    auto proditer = producers.get_index<"prototalvote"_n>();
                    auto itr = proditer.begin();

                    int32_t prodcount = std::distance(producers.begin(), producers.end());
                    check(prodcount > 0,"error -- no producers");

                    for (int32_t idx=0;idx < prodcount; idx++) {
                        if (itr->is_active) {
                            voteshares.emplace(actor, [&](auto &p) {
                                p.owner = itr->owner;
                                p.votes = itr->total_votes;
                            });
                            bpcounter++;
                        }

                        itr++;
                        if (bpcounter >= MAXBPS) break;
                    } // &itr : producers table
                    //Move 1/365 of the bucketpool to the bpshare
                        bprewards.set(bpreward{bprewards.get().rewards + static_cast<uint64_t>(bucketrewards.get().rewards / YEARDAYS)}, get_self());
                        bucketrewards.set(bucketpool{bucketrewards.get().rewards - static_cast<uint64_t>(bucketrewards.get().rewards / YEARDAYS)}, get_self());

                        if (state.bpreservetokensminted < BPMAXRESERVE && bprewards.get().rewards < BPMAXTOMINT) {

                          uint64_t bptomint = BPMAXTOMINT - bprewards.get().rewards;
                          const uint64_t bpremainingreserve = BPMAXRESERVE - state.bpreservetokensminted;

                            if (bpremainingreserve < BPMAXTOMINT) {
                                    bptomint = bpremainingreserve;
                            }

                            if (state.bpreservetokensminted + bptomint > BPMAXRESERVE) {
                              bptomint = BPMAXRESERVE - state.bpreservetokensminted;
                            }
                            //Mint new tokens up to 50,000 FIO
                            action(permission_level{get_self(), "active"_n},
                                   TokenContract, "mintfio"_n,
                                   make_tuple(TREASURYACCOUNT,bptomint)
                                   ).send();

                            state.bpreservetokensminted += bptomint;

                            //Include the minted tokens in the reward payout
                            bprewards.set(bpreward{bprewards.get().rewards + bptomint}, get_self());
                            //This new reward amount that has been minted will be appended to the rewards being divied up next
                        }
                        else {
                          print("Block producers reserve minting exhausted");
                        }
                        //!!!rewards is now 0 in the bprewards table and can no longer be referred to. If needed use projectedpay

                        uint64_t fdtntomint = FDTNMAXTOMINT;
                        const uint64_t fdtnremainingreserve = FDTNMAXRESERVE - state.fdtnreservetokensminted;

                        if (fdtnremainingreserve < FDTNMAXTOMINT) {
                                fdtntomint = fdtnremainingreserve;
                        }

                        if (state.fdtnreservetokensminted < FDTNMAXRESERVE) {

                                //Mint new tokens up to 50,000 FIO
                                action(permission_level{get_self(), "active"_n},
                                       TokenContract, "mintfio"_n,
                                       make_tuple(FOUNDATIONACCOUNT,fdtntomint)
                                ).send();

                                    state.fdtnreservetokensminted += fdtntomint;

                        }
                        // All bps are now in pay schedule, calculate the shares
                        int64_t bpcount = std::distance(voteshares.begin(), voteshares.end());
                        int64_t abpcount = MAXACTIVEBPS;

                        if (bpcount <= MAXACTIVEBPS) abpcount = bpcount;
                        auto bprewardstat = bprewards.get();
                        uint64_t tostandbybps = static_cast<uint64_t>(bprewardstat.rewards * .60);
                        uint64_t toactivebps = static_cast<uint64_t>(bprewardstat.rewards * .40);

                        bpcounter = 0;
                        auto votesharesiter = voteshares.get_index<"byvotes"_n>();
                        for (const auto &itr : votesharesiter) {
                            if (bpcounter > (bpcount - abpcount)-1) {
                                voteshares.modify(itr, get_self(), [&](auto &entry) {
                                    entry.abpayshare = static_cast<uint64_t>(toactivebps / abpcount);;
                                });
                            }
                                voteshares.modify(itr, get_self(), [&](auto &entry) {
                                                entry.sbpayshare =  static_cast<uint64_t>((tostandbybps) * (itr.votes / gstate.total_producer_vote_weight));;
                                        });
                                bpcounter++;
                        } // &itr : voteshares

                        //Start 24 track for daily pay schedule
                        if (state.payschedtimer == 0){
                                state.payschedtimer = now();
                        }else {
                                state.payschedtimer += (PAYSCHEDTIME - 1);
                        }

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
                                        bprewards.set(bpreward{bprewards.get().rewards - payout}, get_self());
                                }
                                //Keep track of rewards paid for reserve minting
                                  state.rewardspaid += payout;

                                //Invoke system contract to reset producer last_claim_time and unpaid_blocks
                                action(permission_level{get_self(), "active"_n},
                                   SYSTEMACCOUNT, "resetclaim"_n,
                                   make_tuple(producer)
                                ).send();
                        }


                        // PAY FOUNDATION //
                        auto fdtnstate = fdtnrewards.get();
                        if(fdtnstate.rewards > 0) {
                            action(permission_level{get_self(), "active"_n},
                                   TokenContract, "transfer"_n,
                                   make_tuple(TREASURYACCOUNT, FOUNDATIONACCOUNT, asset(fdtnstate.rewards, FIOSYMBOL),
                                              string("Paying foundation from treasury."))).send();
                        }

                        //Clear the foundation rewards counter
                        fdtnrewards.set(fdtnreward{0}, get_self());
                        //remove the producer from payschedule
                        voteshares.erase(bpiter);
                } //endif now() > bpiter + 172800

                const string response_string = string("{\"status\": \"OK\",\"amount\":") +
                                         to_string(payout) + string("}");

                fio_400_assert(transaction_size() <= MAX_TRX_SIZE, "transaction_size", std::to_string(transaction_size()),
                 "Transaction is too large", ErrorTransactionTooLarge);

                send_response(response_string.c_str());
        } //bpclaim

        // @abi action
        [[eosio::action]]
        void startclock() {
                require_auth(TREASURYACCOUNT);

                clockstate.set(treasurystate{now(), now()}, get_self());
                bucketrewards.set(bucketpool{0}, get_self());
                bprewdupdate(0);
        }

        // @abi action
        [[eosio::action]]
        void bprewdupdate(const uint64_t &amount) {

                eosio_assert((has_auth(AddressContract) || has_auth(TokenContract) || has_auth(TREASURYACCOUNT) ||
                             has_auth(REQOBTACCOUNT) || has_auth(SYSTEMACCOUNT) || has_auth(FeeContract)),
                             "missing required authority of fio.address, fio.treasury, fio.fee, fio.token, eosio or fio.reqobt");

                bprewards.set(bprewards.exists() ? bpreward{bprewards.get().rewards + amount} : bpreward{amount}, get_self());
        }

        // @abi action
        [[eosio::action]]
        void bppoolupdate(const uint64_t &amount) {

                eosio_assert((has_auth(AddressContract) || has_auth(TokenContract) || has_auth(TREASURYACCOUNT) ||
                             has_auth(REQOBTACCOUNT)),
                             "missing required authority of fio.address, fio.treasury, fio.token, or fio.reqobt");
                bucketrewards.set(bucketrewards.exists() ? bucketpool{bucketrewards.get().rewards + amount} : bucketpool{amount}, get_self());
        }

        // @abi action
        [[eosio::action]]
        void fdtnrwdupdat(const uint64_t &amount) {

                eosio_assert((has_auth(AddressContract) || has_auth(TokenContract) || has_auth(TREASURYACCOUNT) ||
                             has_auth(REQOBTACCOUNT) || has_auth(SYSTEMACCOUNT) || has_auth(FeeContract)),
                             "missing required authority of fio.address, fio.token, fio.fee, fio.treasury or fio.reqobt");

                fdtnrewards.set(fdtnrewards.exists() ? fdtnreward{fdtnrewards.get().rewards + amount} : fdtnreward{amount}, get_self());

        }

};     //class FIOTreasury

EOSIO_DISPATCH(FIOTreasury, (tpidclaim)(startclock)(bprewdupdate)(fdtnrwdupdat)(bppoolupdate)
               (bpclaim))
}
