/** FioCommon implementation file
 *  Description: FioCommon is the helper directory that assists FIO is common tasks
 *  @author Adam Androulidakis, Casey Gardiner, Ed Rotthoff
 *  @modifedby
 *  @file fio.common.cpp
 *  @license FIO Foundation ( https://github.com/fioprotocol/fio/blob/master/LICENSE ) Dapix
 */

#pragma once

#include <vector>
#include <string>
#include <eosiolib/eosio.hpp>
#include <eosiolib/system.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/crypto.hpp>
#include <eosiolib/transaction.hpp>
#include "keyops.hpp"
#include "fioerror.hpp"
#include "fio.accounts.hpp"
#include "fio_common_validator.hpp"
#include "chain_control.hpp"
#include "account_operations.hpp"

#define YEARTOSECONDS 31536000
#define SECONDS30DAYS 2592000
#define SECONDSPERDAY  86400
#define DOMAINWAITFORBURNDAYS  90 * SECONDSPERDAY
#define ADDRESSWAITFORBURNDAYS  90 * SECONDSPERDAY
#define MAXBOUNTYTOKENSTOMINT 125000000000000000
//#define MINVOTEDFIO 10'000'000'000000000 //TESTNET ONLY
#define MINVOTEDFIO 65'000'000'000000000
#define MINUTE 60
#define SECONDSPERHOUR 3600
#define SECONDSBETWEENBPCLAIM (SECONDSPERHOUR * 4)
#define YEARDAYS 365
#define MAXBPS 42
#define MAXACTIVEBPS 21
#define DEFAULTBUNDLEAMT 100

namespace fioio {

    using namespace eosio;
    using namespace std;

    typedef long long int64;

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

    void fio_fees(const name &actor, const asset &fee) {
        if (fee.amount > 0) {
            action(permission_level{SYSTEMACCOUNT, "active"_n},
                   TokenContract, "transfer"_n,
                   make_tuple(actor, TREASURYACCOUNT, fee,
                              string("FIO API fees. Thank you."))
            ).send();
        }
    }


    inline bool isFIOSystem(const name &actor){
        return
            (actor == fioio::MSIGACCOUNT ||
             actor == fioio::WRAPACCOUNT ||
             actor == fioio::SYSTEMACCOUNT ||
             actor == fioio::ASSERTACCOUNT ||
             actor == fioio::REQOBTACCOUNT ||
             actor == fioio::FeeContract ||
             actor == fioio::AddressContract ||
             actor == fioio::TPIDContract ||
             actor == fioio::TokenContract ||
             actor == fioio::TREASURYACCOUNT ||
             actor == fioio::FIOSYSTEMACCOUNT ||
             actor == fioio::FIOACCOUNT);
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

    static uint128_t string_to_uint128_hash(const string str) {

        eosio::checksum160 tmp;
        uint128_t retval = 0;
        uint8_t *bp = (uint8_t * ) & tmp;

        tmp = eosio::sha1(str.c_str(), str.length());

        bp = (uint8_t * ) & tmp;
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

    void process_rewards(const string &tpid, const uint64_t &amount, const name &auth, const name &actor) {

        action(
                permission_level{auth, "active"_n},
                TREASURYACCOUNT,
                "fdtnrwdupdat"_n,
                std::make_tuple((uint64_t)(static_cast<double>(amount) * .05))
        ).send();
        fionames_table fionames(AddressContract, AddressContract.value);
        uint128_t fioaddhash = string_to_uint128_hash(tpid.c_str());

        auto namesbyname = fionames.get_index<"byname"_n>();
        auto fionamefound = namesbyname.find(fioaddhash);

        if (fionamefound != namesbyname.end()) {

            bounties_table bounties(TPIDContract, TPIDContract.value);
            uint64_t bamount = 0;

            if (bounties.get().tokensminted < MAXBOUNTYTOKENSTOMINT) {
                bamount = (uint64_t)(static_cast<double>(amount) * .40);

                action(permission_level{TREASURYACCOUNT, "active"_n},
                       TokenContract, "mintfio"_n,
                       make_tuple(TREASURYACCOUNT, bamount)
                ).send();

                action(
                        permission_level{TREASURYACCOUNT, "active"_n},
                        TPIDContract,
                        "updatebounty"_n,
                        std::make_tuple(bamount)
                ).send();

            }

            action(
                    permission_level{auth, "active"_n},
                    TPIDContract,
                    "updatetpid"_n,
                    std::make_tuple(tpid, actor, (amount / 10) + bamount)
            ).send();

            action(
                    permission_level{auth, "active"_n},
                    TREASURYACCOUNT,
                    "bprewdupdate"_n,
                    std::make_tuple((uint64_t)(static_cast<double>(amount) * .85))
            ).send();

        } else {
            action(
                    permission_level{auth, "active"_n},
                    TREASURYACCOUNT,
                    "bprewdupdate"_n,
                    std::make_tuple((uint64_t)(static_cast<double>(amount) * .95))
            ).send();
        }
    }


    void processbucketrewards(const string &tpid, const uint64_t &amount, const name &auth, const name &actor) {


        action(
                permission_level{auth, "active"_n},
                TREASURYACCOUNT,
                "fdtnrwdupdat"_n,
                std::make_tuple((uint64_t)(static_cast<double>(amount) * .05))
        ).send();

        fionames_table fionames(AddressContract, AddressContract.value);
        uint128_t fioaddhash = string_to_uint128_hash(tpid.c_str());
        auto namesbyname = fionames.get_index<"byname"_n>();
        auto fionamefound = namesbyname.find(fioaddhash);

        if (fionamefound != namesbyname.end()) {

            bounties_table bounties(TPIDContract, TPIDContract.value);
            uint64_t bamount = 0;

            if (bounties.get().tokensminted < MAXBOUNTYTOKENSTOMINT) {
                bamount = (uint64_t)(static_cast<double>(amount) * .40);
                action(permission_level{TREASURYACCOUNT, "active"_n},
                       TokenContract, "mintfio"_n,
                       make_tuple(TREASURYACCOUNT, bamount)
                ).send();

                action(
                        permission_level{TREASURYACCOUNT, "active"_n},
                        TPIDContract,
                        "updatebounty"_n,
                        std::make_tuple(bamount)
                ).send();

            }

            action(
                    permission_level{auth, "active"_n},
                    TPIDContract,
                    "updatetpid"_n,
                    std::make_tuple(tpid, actor, (amount / 10) + bamount)
            ).send();


            action(
                    permission_level{auth, "active"_n},
                    TREASURYACCOUNT,
                    "bppoolupdate"_n,
                    std::make_tuple((uint64_t)(static_cast<double>(amount) * .85))
            ).send();
        } else {

            action(
                    permission_level{auth, "active"_n},
                    TREASURYACCOUNT,
                    "bppoolupdate"_n,
                    std::make_tuple((uint64_t)(static_cast<double>(amount) * .95))
            ).send();
        }
    }

    //Precondition: this method should only be called by register_producer, vote_producer, unregister_producer, register_proxy, unregister_proxy, vote_proxy
    // after transaction fees have been defined
    //Postcondition: the foundation has been rewarded 5% of the transaction fee and top 21/active block producers rewarded 95% of the transaction fee
    void processrewardsnotpid(const uint64_t &amount, const name &actor) {

        action(
                permission_level{actor, "active"_n},
                TREASURYACCOUNT,
                "bprewdupdate"_n,
                std::make_tuple((uint64_t)(static_cast<double>(amount) * .95))
        ).send();

        action(
                permission_level{actor, "active"_n},
                TREASURYACCOUNT,
                "fdtnrwdupdat"_n,
                std::make_tuple((uint64_t)(static_cast<double>(amount) * .05))
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

    static const uint64_t INITIALACCOUNTRAM  = 25600;
    static const uint64_t ADDITIONALRAMBPDESCHEDULING = 25600;

    static const uint64_t REGDOMAINRAM  = 2560;  //integrated.
    static const uint64_t REGADDRESSRAM = 2560; //integrated.
    static const uint64_t ADDADDRESSRAM = 512; //integrated.
    static const uint64_t SETDOMAINPUBRAM = 256; //integrated.
    static const uint64_t NEWFUNDSREQUESTRAM = 2048; //integrated.
    static const uint64_t RECORDOBTRAM = 2048; //integrated.
    static const uint64_t RENEWADDRESSRAM = 1024; //integrated.
    static const uint64_t RENEWDOMAINRAM = 1024; //integrated.
    static const uint64_t XFERRAM = 512; //integrated.
    static const uint64_t TRANSFERPUBKEYRAM = 1024; //integrated.
    static const uint64_t REJECTFUNDSRAM = 512; //integrated.
    static const uint64_t CANCELFUNDSRAM = 512; //integrated.
    static const uint64_t SETFEEVOTERAM = 0; //integrated.
    static const uint64_t BUNDLEVOTERAM = 0; //integrated.




} // namespace fioio
