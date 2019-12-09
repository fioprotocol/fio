/** FioCommon implementation file
 *  Description: FioCommon is the helper directory that assists FIO is common tasks
 *  @author Adam Androulidakis, Casey Gardiner, Ed Rotthoff
 *  @modifedby
 *  @file fio.common.cpp
 *  @copyright Dapix
 */

#pragma once

#include <vector>
#include <string>
#include <eosiolib/eosio.hpp>
#include <eosiolib/system.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/crypto.hpp>
#include "json.hpp"
#include "keyops.hpp"
#include "fioerror.hpp"
#include "fio.accounts.hpp"
#include "fio_common_validator.hpp"
#include "chain_control.hpp"
#include "account_operations.hpp"

#ifndef YEARTOSECONDS
#define YEARTOSECONDS 31536000
#endif

#ifndef SECONDS30DAYS
#define SECONDS30DAYS 2592000
#endif

#ifndef SECONDSPERDAY
#define SECONDSPERDAY  86400
#endif

#ifndef DOMAINWAITFORBURNDAYS
#define DOMAINWAITFORBURNDAYS  90 * SECONDSPERDAY
#endif

#ifndef ADDRESSWAITFORBURNDAYS
#define ADDRESSWAITFORBURNDAYS  90 * SECONDSPERDAY
#endif


#ifndef MAXBOUNTYTOKENSTOMINT
#define MAXBOUNTYTOKENSTOMINT 200000000000000000
#endif

#ifndef MINVOTEDFIO
#define MINVOTEDFIO 65'000'000'000000000
#endif



namespace fioio {

    using namespace eosio;
    using namespace std;

    typedef long long  int64;

    struct config {
        name tokencontr; // owner of the token contract
        bool pmtson = true; // enable/disable payments

        EOSLIB_SERIALIZE(config, (tokencontr)(pmtson))
    };
    typedef singleton<"configs"_n, config> configs_singleton;

    static constexpr char char_to_symbol(char c) {
        if (c >= 'a' && c <= 'z')
            return (c - 'a') + 6;
        if (c >= '1' && c <= '5')
            return (c - '1') + 1;
        return 0;
    }

    static constexpr uint64_t string_to_name(const char *str) {

        uint32_t len = 0;
        while (str[len]) ++len;

        uint64_t value = 0;

        for (uint32_t i = 0; i <= 12; ++i) {
            uint64_t c = 0;
            if (i < len && i <= 12) c = uint64_t(char_to_symbol(str[i]));
            if (i < 12) {
                c &= 0x1f;
                c <<= 64 - 5 * (i + 1);
            } else {
                c &= 0x0f;
            }

            value |= c;
        }
        return value;
    }

    static constexpr uint64_t string_to_uint64_hash(const char *str) {

        uint32_t len = 0;
        while (str[len]) ++len;

        uint64_t value = 0;
        uint64_t multv = 0;
        if (len > 0) {
            multv = 60 / len;
        }
        for (uint32_t i = 0; i < len; ++i) {
            uint64_t c = 0;
            if (i < len) c = uint64_t(str[i]);

            if (i < 60) {
                c &= 0x1f;
                c <<= 64 - multv * (i + 1);
            } else {
                c &= 0x0f;
            }

            value |= c;
        }
        return value;
    }

    static uint128_t string_to_uint128_hash(const char *str) {

        eosio::checksum160 tmp;
        uint128_t retval = 0;
        uint8_t *bp = (uint8_t *) &tmp;
        uint32_t len = 0;

        while (str[len]) ++len;

        tmp = eosio::sha1(str, len);

        bp = (uint8_t *) &tmp;
        memcpy(&retval, bp, sizeof(retval));

        return retval;
    }

    //use this for debug to see the value of your uint128_t, this will match what shows in get table.
    static std::string to_hex(const char *d, uint32_t s) {
        std::string r;
        const char *to_hex = "0123456789abcdef";
        uint8_t *c = (uint8_t *) d;
        for (uint32_t i = 0; i < s; ++i) {
            (r += to_hex[(c[i] >> 4)]) += to_hex[(c[i] & 0x0f)];

        }
        return r;
    }

    /***
        * This function will increment the specified time by the specified number of seconds.
        * @param timetoincrement   this is the number of years from now to return as a value
        * @param numbverseconds   this is the number of seconds to add to the time
        * @return  the incremented time
        */
    inline uint32_t get_time_plus_seconds(uint32_t timetoincrement, int numberseconds) {


        uint32_t incremented_time = timetoincrement + numberseconds;
        return incremented_time;
    }

    struct [[eosio::action]] bpreward {

        uint64_t rewards;

        uint64_t primary_key() const { return rewards; }

        EOSLIB_SERIALIZE(bpreward, (rewards)
        )
    };

    typedef singleton<"bprewards"_n, bpreward> bprewards_table;

    // @abi table bucketpool i64
    struct [[eosio::action]] bucketpool {

        uint64_t rewards;

        uint64_t primary_key() const { return rewards; }

        EOSLIB_SERIALIZE(bucketpool, (rewards)
        )
    };

    typedef singleton<"bpbucketpool"_n, bucketpool> bpbucketpool_table;

    // @abi table fdtnreward i64
    struct [[eosio::action]] fdtnreward {

        uint64_t rewards;

        uint64_t primary_key() const { return rewards; }

        EOSLIB_SERIALIZE(fdtnreward, (rewards)
        )
    };

    typedef singleton<"fdtnrewards"_n, fdtnreward> fdtnrewards_table;

    // @abi table bounties i64
    struct [[eosio::action]] bounty {

        uint64_t tokensminted;

        uint64_t primary_key() const { return tokensminted; }

        EOSLIB_SERIALIZE(bounty, (tokensminted)
        )
    };

    typedef singleton<"bounties"_n, bounty> bounties_table;

    void process_rewards(const string &tpid, const uint64_t &amount, const name &actor) {

        action(
                permission_level{actor, "active"_n},
                TREASURYACCOUNT,
                "fdtnrwdupdat"_n,
                std::make_tuple((uint64_t) (static_cast<double>(amount) * .02))
        ).send();
        fionames_table fionames(AddressContract, AddressContract.value);
        uint128_t fioaddhash = string_to_uint128_hash(tpid.c_str());

        auto namesbyname = fionames.get_index<"byname"_n>();
        auto fionamefound = namesbyname.find(fioaddhash);

        if (fionamefound != namesbyname.end()) {

            bounties_table bounties(TPIDContract, TPIDContract.value);
            uint64_t bamount = 0;

            if (bounties.get().tokensminted < MAXBOUNTYTOKENSTOMINT) {
                bamount = (uint64_t) (static_cast<double>(amount) * .65);

                action(permission_level{TREASURYACCOUNT, "active"_n},
                       TokenContract, "mintfio"_n,
                       make_tuple(TREASURYACCOUNT,bamount)
                ).send();

                action(
                        permission_level{TREASURYACCOUNT, "active"_n},
                        TPIDContract,
                        "updatebounty"_n,
                        std::make_tuple(bamount)
                ).send();

            }

            action(
                    permission_level{actor, "active"_n},
                    TPIDContract,
                    "updatetpid"_n,
                    std::make_tuple(tpid, actor, (amount / 10) + bamount)
            ).send();

            action(
                    permission_level{actor, "active"_n},
                    TREASURYACCOUNT,
                    "bprewdupdate"_n,
                    std::make_tuple((uint64_t) (static_cast<double>(amount) * .88))
            ).send();

        } else {
            action(
                    permission_level{actor, "active"_n},
                    TREASURYACCOUNT,
                    "bprewdupdate"_n,
                    std::make_tuple((uint64_t) (static_cast<double>(amount) * .98))
            ).send();
        }
    }



    //this will compute the present unlocked tokens for this user based on the
    //unlocking schedule, it will update the lockedtokens table if the doupdate
    //is set to true.
    uint64_t computeremaininglockedtokens(const name &actor, bool doupdate){
        uint32_t present_time = now();

        print(" unlock_tokens for ",actor,"\n");

        print(" present time is ",present_time,"\n");


        eosiosystem::locked_tokens_table lockedTokensTable(SYSTEMACCOUNT, SYSTEMACCOUNT.value);
        auto lockiter = lockedTokensTable.find(actor.value);
        if(lockiter != lockedTokensTable.end()){
            if(lockiter->inhibit_unlocking && (lockiter->grant_type == 2)){
                return lockiter->remaining_locked_amount;
            }
            if (lockiter->unlocked_period_count < 9)  {
                print(" issue time is ",lockiter->timestamp,"\n");
                print(" present time - issue time is ",(present_time  - lockiter->timestamp),"\n");
                // uint32_t timeElapsed90DayBlocks = (int)((present_time  - lockiter->timestamp) / SECONDSPERDAY) / 90;
                //we kludge the time block evaluation to become one block per 3 minutes
                uint32_t timeElapsed90DayBlocks = (int)((present_time  - lockiter->timestamp) / 180) / 1;
                print("--------------------DANGER------------------------------ ","\n");
                print("--------------------DANGER------------------------------ ","\n");
                print("--------------------DANGER------------------------------ ","\n");
                print("------time step for unlocking is kludged to 3 min-------","\n");
                print("--------------------DANGER------------------------------ ","\n");
                print("--------------------DANGER------------------------------ ","\n");
                print(" timeElapsed90DayBlocks ",timeElapsed90DayBlocks,"\n");
                uint32_t numberVestingPayouts = lockiter->unlocked_period_count;
                print(" number payouts so far ",numberVestingPayouts,"\n");
                uint32_t remainingPayouts = 0;

                uint64_t newlockedamount = lockiter->remaining_locked_amount;
                print(" locked amount ",newlockedamount,"\n");

                uint64_t totalgrantamount = lockiter->total_grant_amount;
                print(" total grant amount ",totalgrantamount,"\n");

                uint64_t amountpay = 0;

                uint64_t addone = 0;

                if (timeElapsed90DayBlocks > 8){
                    timeElapsed90DayBlocks = 8;
                }

                bool didsomething = false;

                //do the day zero unlocking, this is the first unlocking.
                if(numberVestingPayouts == 0) {
                    if (lockiter->grant_type == 1) {
                        //pay out 1% for type 1
                        amountpay = totalgrantamount / 100;
                        print(" amount to pay type 1 ",amountpay,"\n");

                    } else if (lockiter->grant_type == 2) {
                        //pay out 2% for type 2
                        amountpay = (totalgrantamount/100)*2;
                        print(" amount to pay type 2 ",amountpay,"\n");
                    }else{
                        check(false,"unknown grant type");
                    }
                    if (newlockedamount > amountpay) {
                        newlockedamount -= amountpay;
                    }else {
                        newlockedamount = 0;
                    }
                    print(" recomputed locked amount ",newlockedamount,"\n");
                    addone = 1;
                    didsomething = true;
                }

                //this accounts for the first unlocking period being the day 0 unlocking period.
                if (numberVestingPayouts >0){
                    numberVestingPayouts--;
                }

                if (timeElapsed90DayBlocks > numberVestingPayouts) {
                    remainingPayouts = timeElapsed90DayBlocks - numberVestingPayouts;
                    uint64_t percentperblock = 0;
                    if (lockiter->grant_type == 1) {
                        //this logic assumes to have 3 decimal places in the specified percentage
                        percentperblock = 12375;
                    } else {
                        //this is assumed to have 3 decimal places in the specified percentage
                        percentperblock = 12275;
                    }
                    print("remaining payouts ", remainingPayouts, "\n");
                    //this is assumed to have 3 decimal places in the specified percentage
                    amountpay = (remainingPayouts * (totalgrantamount * percentperblock)) / 100000;
                    print(" amount to pay ", amountpay, "\n");

                    if (newlockedamount > amountpay) {
                        newlockedamount -= amountpay;
                    } else {
                        newlockedamount = 0;
                    }
                    print(" recomputed locked amount ", newlockedamount, "\n");
                    didsomething = true;
                }

                if(didsomething && doupdate) {
                    print(" updating recomputed locked amount into table ", newlockedamount, "\n");
                    //update the locked table.
                    lockedTokensTable.modify(lockiter, SYSTEMACCOUNT, [&](auto &av) {
                        av.remaining_locked_amount = newlockedamount;
                        av.unlocked_period_count += remainingPayouts + addone;
                    });
                }else {
                    print(" NOT updating recomputed locked amount into table ", newlockedamount, "\n");
                }

                return newlockedamount;

            }else{
                return lockiter->remaining_locked_amount;
            }
        }
        return 0;
    }






    void processbucketrewards(const string &tpid, const uint64_t &amount, const name &actor) {

        action(
                permission_level{actor, "active"_n},
                TREASURYACCOUNT,
                "fdtnrwdupdat"_n,
                std::make_tuple((uint64_t) (static_cast<double>(amount) * .02))
        ).send();

        fionames_table fionames(AddressContract, AddressContract.value);
        uint128_t fioaddhash = string_to_uint128_hash(tpid.c_str());
        auto namesbyname = fionames.get_index<"byname"_n>();
        auto fionamefound = namesbyname.find(fioaddhash);

        if (fionamefound != namesbyname.end()) {

            bounties_table bounties(TPIDContract, TPIDContract.value);
            uint64_t bamount = 0;

            if (bounties.get().tokensminted < MAXBOUNTYTOKENSTOMINT) {
                bamount = (uint64_t) (static_cast<double>(amount) * .65);
                action(permission_level{TREASURYACCOUNT, "active"_n},
                       TokenContract, "mintfio"_n,
                       make_tuple(TREASURYACCOUNT,bamount)
                ).send();

                action(
                        permission_level{TREASURYACCOUNT, "active"_n},
                        TPIDContract,
                        "updatebounty"_n,
                        std::make_tuple(bamount)
                ).send();

            }

            action(
                    permission_level{actor, "active"_n},
                    TPIDContract,
                    "updatetpid"_n,
                    std::make_tuple(tpid, actor, (amount / 10) + bamount)
            ).send();


            action(
                    permission_level{actor, "active"_n},
                    TREASURYACCOUNT,
                    "bppoolupdate"_n,
                    std::make_tuple((uint64_t) (static_cast<double>(amount) * .88))
            ).send();
        } else {

            action(
                    permission_level{actor, "active"_n},
                    TREASURYACCOUNT,
                    "bppoolupdate"_n,
                    std::make_tuple((uint64_t) (static_cast<double>(amount) * .98))
            ).send();
        }
    }

    //Precondition: this method should only be called by register_producer, vote_producer, unregister_producer, register_proxy, unregister_proxy, vote_proxy
    // after transaction fees have been defined
    //Postcondition: the foundation has been rewarded 2% of the transaction fee and top 21/active block producers rewarded 98% of the transaction fee
    void processrewardsnotpid(const uint64_t &amount, const name &actor) {

        action(
                permission_level{actor, "active"_n},
                TREASURYACCOUNT,
                "bprewdupdate"_n,
                std::make_tuple((uint64_t) (static_cast<double>(amount) * .98))
        ).send();

        action(
                permission_level{actor, "active"_n},
                TREASURYACCOUNT,
                "fdtnrwdupdat"_n,
                std::make_tuple((uint64_t) (static_cast<double>(amount) * .02))
        ).send();
    }

    inline bool isPubKeyValid(const string &pubkey) {

        if (pubkey.length() != 53) return false;

        string pubkey_prefix("FIO");
        auto result = mismatch(pubkey_prefix.begin(), pubkey_prefix.end(),
                               pubkey.begin());
        if (result.first != pubkey_prefix.end()) return false;
        auto base58substr = pubkey.substr(pubkey_prefix.length());

        vector<unsigned char> vch;
        if (!decode_base58(base58substr, vch) || (vch.size() != 37)) return false;

        array<unsigned char, 33> pubkey_data;
        copy_n(vch.begin(), 33, pubkey_data.begin());

        capi_checksum160 check_pubkey;
        ripemd160(reinterpret_cast<char *>(pubkey_data.data()), 33, &check_pubkey);
        if (memcmp(&check_pubkey.hash, &vch.end()[-4], 4) != 0) return false;
        //end of the public key validity check.

        return true;
    }
} // namespace fioio
