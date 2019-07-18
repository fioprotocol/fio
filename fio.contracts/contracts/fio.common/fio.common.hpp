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
#include "fio_common_validator.hpp"
#include "chain_control.hpp"
#include "account_operations.hpp"

#ifndef FEE_CONTRACT
#define FEE_CONTRACT "fio.fee"
#endif

#ifndef TOKEN_CONTRACT
#define TOKEN_CONTRACT "fio.token"
#endif

#ifndef FIO_SYSTEM
#define FIO_SYSTEM "fio.system"
#endif

#ifndef NAME_CONTRACT
#define NAME_CONTRACT "fio.name"
#endif

#ifndef FINANCE_CONTRACT
#define FINANCE_CONTRACT "fio.finance"
#endif

#ifndef YEARTOSECONDS
#define YEARTOSECONDS 31536000
#endif

namespace fioio {

    using namespace eosio;
    using namespace std;
    using time = uint32_t;

    static const name FeeContract = name(FEE_CONTRACT);    // account hosting the fee contract
    static const name SystemContract = name(FIO_SYSTEM);
    static const name TPIDContract = name("fio.tpid");
    static const name TokenContract = name("fio.token");

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

    void process_rewards(const string &tpid, const uint64_t &amount, const name &actor) {

      action(
      permission_level{actor,"active"_n},
      "fio.treasury"_n,
      "fdtnrwdupdat"_n,
      std::make_tuple((uint64_t)(static_cast<double>(amount) * .02))
      ).send();



      if (!tpid.empty()) {
        action(
        permission_level{actor,"active"_n},
        "fio.tpid"_n,
        "updatetpid"_n,
        std::make_tuple(tpid, actor, amount / 10)
        ).send();


        action(
        permission_level{actor,"active"_n},
        "fio.treasury"_n,
        "bprewdupdate"_n,
        std::make_tuple((uint64_t)(static_cast<double>(amount) * .88))
        ).send();

    } else {

      action(
      permission_level{actor,"active"_n},
      "fio.treasury"_n,
      "bprewdupdate"_n,
      std::make_tuple((uint64_t)(static_cast<double>(amount) * .98))
      ).send();

    }

    }

} // namespace fioio
