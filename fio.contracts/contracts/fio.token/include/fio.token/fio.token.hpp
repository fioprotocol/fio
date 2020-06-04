/** FioToken implementation file
 *  Description: FioToken is the smart contract that help manage the FIO Token.
 *  @author Adam Androulidakis, Casey Gardiner, Ed Rotthoff
 *  @modifedby
 *  @file fio.token.cpp
 *  @license FIO Foundation ( https://github.com/fioprotocol/fio/blob/master/LICENSE ) Dapix
 */

#pragma once

#include <fio.common/fio.common.hpp>
#include <fio.address/fio.address.hpp>
#include <fio.system/include/fio.system/fio.system.hpp>
#include <fio.fee/fio.fee.hpp>
#include <fio.tpid/fio.tpid.hpp>

namespace eosiosystem {
    class system_contract;
}

namespace eosio {
    using namespace fioio;

    using std::string;

    class [[eosio::contract("fio.token")]] token : public contract {
    private:
        fioio::eosio_names_table eosionames;
        fioio::fiofee_table fiofees;
        fioio::config appConfig;
        fioio::tpids_table tpids;
        fioio::fionames_table fionames;
        eosiosystem::locked_tokens_table lockedTokensTable;
        eosiosystem::general_locks_table generalLockTokensTable;

    public:
        token(name s, name code, datastream<const char *> ds) : contract(s, code, ds),
                                                                eosionames(fioio::AddressContract,
                                                                           fioio::AddressContract.value),
                                                                fionames(fioio::AddressContract,
                                                                         fioio::AddressContract.value),
                                                                fiofees(fioio::FeeContract, fioio::FeeContract.value),
                                                                tpids(TPIDContract, TPIDContract.value),
                                                                lockedTokensTable(SYSTEMACCOUNT, SYSTEMACCOUNT.value),
                                                                generalLockTokensTable(SYSTEMACCOUNT, SYSTEMACCOUNT.value){
            fioio::configs_singleton configsSingleton(fioio::FeeContract, fioio::FeeContract.value);
            appConfig = configsSingleton.get_or_default(fioio::config());
        }

        [[eosio::action]]
        void create(asset maximum_supply);

        [[eosio::action]]
        void issue(name to, asset quantity, string memo);

        [[eosio::action]]
        void mintfio(const name &to, const uint64_t &amount);

        [[eosio::action]]
        void retire(asset quantity, string memo);

        [[eosio::action]]
        void transfer(name from,
                      name to,
                      asset quantity,
                      string memo);

        [[eosio::action]]
        void trnsfiopubky(const string &payee_public_key,
                          const int64_t &amount,
                          const int64_t &max_fee,
                          const name &actor,
                          const string &tpid);

        [[eosio::action]]
        void trnsloctoks(const string &payee_public_key,
                                const int32_t &can_vote,
                                const vector<eosiosystem::lockperiods> periods,
                                const int64_t &amount,
                                const int64_t &max_fee,
                                const name &actor,
                                const string &tpid);

        static asset get_supply(name token_contract_account, symbol_code sym_code) {
            stats statstable(token_contract_account, sym_code.raw());
            const auto &st = statstable.get(sym_code.raw());
            return st.supply;
        }

        static asset get_balance(name token_contract_account, name owner, symbol_code sym_code) {
            accounts accountstable(token_contract_account, owner.value);
            const auto &ac = accountstable.get(sym_code.raw());
            return ac.balance;
        }


        using create_action = eosio::action_wrapper<"create"_n, &token::create>;
        using issue_action = eosio::action_wrapper<"issue"_n, &token::issue>;
        using mintfio_action = eosio::action_wrapper<"mintfio"_n, &token::mintfio>;
        using retire_action = eosio::action_wrapper<"retire"_n, &token::retire>;
        using transfer_action = eosio::action_wrapper<"transfer"_n, &token::transfer>;

    private:
        struct [[eosio::table]] account {
            asset balance;

            uint64_t primary_key() const { return balance.symbol.code().raw(); }
        };


        struct [[eosio::table]] currency_stats {
            asset supply;
            asset max_supply;
            name issuer = SYSTEMACCOUNT;

            uint64_t primary_key() const { return supply.symbol.code().raw(); }
        };

        typedef eosio::multi_index<"accounts"_n, account> accounts;
        typedef eosio::multi_index<"stat"_n, currency_stats> stats;

        void sub_balance(name owner, asset value);

        void add_balance(name owner, asset value, name ram_payer);

        bool can_transfer(const name &tokenowner, const uint64_t &feeamount, const uint64_t &transferamount,
                          const bool &isfee);

        bool can_transfer_general(const name &tokenowner,const uint64_t &transferamount);

        name transfer_public_key(const string &payee_public_key,
                                        const int64_t &amount,
                                        const int64_t &max_fee,
                                        const name &actor,
                                        const string &tpid,
                                        const int64_t &feeamount,
                                        const bool &errorifaccountexists);

    public:

        struct transfer_args {
            name from;
            name to;
            asset quantity;
            string memo;
        };

        struct bind2eosio {
            name accountName;
            string public_key;
            bool existing;
        };



        //this will compute the present unlocked tokens for this user based on the
        //unlocking schedule, it will update the lockedtokens table if the doupdate
        //is set to true.
        static uint64_t computeremaininglockedtokens(const name &actor, bool doupdate) {
            uint32_t present_time = now();

            eosiosystem::locked_tokens_table lockedTokensTable(SYSTEMACCOUNT, SYSTEMACCOUNT.value);
            auto lockiter = lockedTokensTable.find(actor.value);
            if (lockiter != lockedTokensTable.end()) {
                if (lockiter->inhibit_unlocking && (lockiter->grant_type == 2)) {
                    return lockiter->remaining_locked_amount;
                }
                if (lockiter->unlocked_period_count < 6) {
                    //to shorten the vesting schedule adapt these variables.
                    uint32_t daysSinceGrant = (int) ((present_time - lockiter->timestamp) / SECONDSPERDAY);
                    uint32_t firstPayPeriod = 90;
                    uint32_t payoutTimePeriod = 180;

                    //TEST LOCKED TOKENS uint32_t daysSinceGrant =  (int)((present_time  - lockiter->timestamp) / 60);
                    //TEST LOCKED TOKENS uint32_t firstPayPeriod = 15;
                    //TEST LOCKED TOKENS uint32_t payoutTimePeriod = 15;

                    bool ninetyDaysSinceGrant = daysSinceGrant >= firstPayPeriod;

                    uint32_t payoutsDue = 0;
                    if (daysSinceGrant > firstPayPeriod) {
                        daysSinceGrant -= firstPayPeriod;
                        payoutsDue = daysSinceGrant / payoutTimePeriod;
                        if (payoutsDue > 6) {
                            payoutsDue = 6;
                        }
                    }

                    uint32_t numberVestingPayouts = lockiter->unlocked_period_count;
                    uint32_t remainingPayouts = 0;

                    uint64_t newlockedamount = lockiter->remaining_locked_amount;
                    uint64_t totalgrantamount = lockiter->total_grant_amount;

                    uint64_t amountpay = 0;
                    uint64_t addone = 0;
                    bool didsomething = false;

                    //this is the first unlocking. 90 days after grant
                    if ((numberVestingPayouts == 0) && (ninetyDaysSinceGrant)) {
                        if ((lockiter->grant_type == 1) ||
                            (lockiter->grant_type == 2) ||
                            (lockiter->grant_type == 3)) {
                            //pay out 1% for type 1
                            amountpay = (totalgrantamount / 100) * 6;
                        } else if (lockiter->grant_type == 4) {
                            //pay out 0 for type 4
                            amountpay = 0;
                        } else {
                            check(false, "unknown grant type");
                        }

                        if (newlockedamount > amountpay) {
                            newlockedamount -= amountpay;
                        } else {
                            newlockedamount = 0;
                        }
                        addone = 1;
                        didsomething = true;
                    }

                    //this accounts for the first unlocking period being the day 0 unlocking period.
                    if (numberVestingPayouts > 0) {
                        numberVestingPayouts--;
                    }

                    if (payoutsDue > numberVestingPayouts) {
                        remainingPayouts = payoutsDue - numberVestingPayouts;
                        uint64_t percentperblock = 0;
                        if ((lockiter->grant_type == 1) ||
                            (lockiter->grant_type == 2) ||
                            (lockiter->grant_type == 3)) {
                            //this logic assumes to have 3 decimal places in the specified percentage
                            percentperblock = 18800;
                        } else if (lockiter->grant_type == 4) {
                            //this is assumed to have 3 decimal places in the specified percentage
                            return lockiter->remaining_locked_amount;
                        } else {  //unknown lock type, dont unlock
                            return lockiter->remaining_locked_amount;
                        }

                        //this is assumed to have 3 decimal places in the specified percentage
                        amountpay = (remainingPayouts * (totalgrantamount * percentperblock)) / 100000;

                        if (newlockedamount > amountpay) {
                            newlockedamount -= amountpay;
                        } else {
                            newlockedamount = 0;
                        }
                        didsomething = true;
                    }

                    if (didsomething && doupdate) {
                        //get fio balance for this account,
                        uint32_t present_time = now();
                        const auto my_balance = eosio::token::get_balance("fio.token"_n, actor, FIOSYMBOL.code());
                        uint64_t amount = my_balance.amount;

                        if (newlockedamount > amount) {
                            print(" WARNING computed amount ", newlockedamount, " is more than amount in account ",
                                  amount, " \n ",
                                  " Transaction processing order can cause this, this amount is being re-aligned, resetting remaining locked amount to ",
                                  amount, "\n");
                            newlockedamount = amount;
                        }
                        //update the locked table.
                        lockedTokensTable.modify(lockiter, SYSTEMACCOUNT, [&](auto &av) {
                            av.remaining_locked_amount = newlockedamount;
                            av.unlocked_period_count += remainingPayouts + addone;
                        });
                    }

                    return newlockedamount;

                } else {
                    return lockiter->remaining_locked_amount;
                }
            }
            return 0;
        }

        //begin general locked tokens
        //this will compute the present unlocked tokens for this user based on the
        //unlocking schedule, it will update the locktokens table if the doupdate
        //is set to true.
        static uint64_t computegenerallockedtokens(const name &actor, bool doupdate) {

            bool dbg = true;
            if (dbg) {
                print(" calling computegenerallockedtokens for actor ", actor, "\n");
            }
            uint32_t present_time = now();

            eosiosystem::general_locks_table generalLockTokensTable(SYSTEMACCOUNT, SYSTEMACCOUNT.value);
            auto locks_by_owner = generalLockTokensTable.get_index<"byowner"_n>();
            auto lockiter = locks_by_owner.find(actor.value);
            if (lockiter != locks_by_owner.end()) {

                if (lockiter->payouts_performed < lockiter->periods.size()) {
                    uint32_t secondsSinceGrant = (present_time - lockiter->timestamp);

                    uint32_t payoutsDue = 0;
                    for (int i=0;i<lockiter->periods.size(); i++){
                        if (lockiter->periods[i].duration <= secondsSinceGrant){
                            payoutsDue++;
                        }

                    }
                    if (dbg) {
                        print("  computegenerallockedtokens ", payoutsDue, " payouts are due to actor ",
                              actor, "\n");
                    }

                    uint64_t amountpay = 0;
                    uint64_t newlockedamount = lockiter->remaining_lock_amount;
                    bool didsomething = false;

                    if (payoutsDue > lockiter->payouts_performed) {
                        int64_t percentperblock = 0;
                       for (int i=lockiter->payouts_performed; i<payoutsDue;i++){
                           //special note -- we allow 3 decimal places for precision. this needs enforced
                           //in the input validation of these values.
                           percentperblock = (int)(lockiter->periods[i].percent * 1000);
                           int64_t amountadded = (lockiter->lock_amount * percentperblock)/100000;
                           if (dbg) {
                               print("  computegenerallockedtokens unlocking ", amountadded, " for actor ",
                                     actor, "\n");
                           }
                           amountpay += amountadded;
                       }

                        if (dbg) {
                            print("  computegenerallockedtokens unlocking total amount ", amountpay,
                                  " for actor ", actor, "\n");
                        }

                        if (newlockedamount > amountpay) {
                            newlockedamount -= amountpay;
                        } else {
                            newlockedamount = 0;
                        }
                    }

                    if ((amountpay > 0) && doupdate) {
                        //get fio balance for this account,
                        uint32_t present_time = now();
                        const auto my_balance = eosio::token::get_balance("fio.token"_n, actor, FIOSYMBOL.code());
                        uint64_t amount = my_balance.amount;

                        if (newlockedamount > amount) {
                            print(" WARNING computed amount ", newlockedamount, " is more than amount in account ",
                                  amount, " \n ",
                                  " Transaction processing order can cause this, this amount is being re-aligned, resetting remaining locked amount to ",
                                  amount, "\n");
                            newlockedamount = amount;
                        }

                        if (dbg) {
                            print("  computegenerallockedtokens setting new locked amount", newlockedamount,
                                  " for actor ", actor, "\n");
                        }

                        //update the locked table.
                        locks_by_owner.modify(lockiter, SYSTEMACCOUNT, [&](auto &av) {
                            av.remaining_lock_amount = newlockedamount;
                            av.payouts_performed = payoutsDue;
                        });
                    }

                    return newlockedamount;

                } else {
                    if (dbg) {
                        print("  computegenerallockedtokens using remaining locked amount ",
                              lockiter->remaining_lock_amount, " for actor ", actor, "\n");
                    }
                    return lockiter->remaining_lock_amount;
                }
            }
            if (dbg) {
                print("  computegenerallockedtokens NO locked tokens for actor ", actor, "\n");
            }
            return 0;
        }



    };
} /// namespace eosio
