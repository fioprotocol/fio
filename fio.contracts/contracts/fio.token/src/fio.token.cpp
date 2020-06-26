/** FioToken implementation file
 *  Description: FioToken is the smart contract that help manage the FIO Token.
 *  @author Adam Androulidakis, Casey Gardiner, Ed Rotthoff
 *  @modifedby
 *  @file fio.token.cpp
 *  @license FIO Foundation ( https://github.com/fioprotocol/fio/blob/master/LICENSE )
 */

#define MAXFIOMINT 100000000000000000

#include "fio.token/fio.token.hpp"

using namespace fioio;

namespace eosio {

    void token::create(asset maximum_supply) {
        require_auth(_self);

        const auto sym = maximum_supply.symbol;
        check(sym.is_valid(), "invalid symbol name");
        check(maximum_supply.is_valid(), "invalid supply");
        check(maximum_supply.amount > 0, "max-supply must be positive");
        check(maximum_supply.symbol == FIOSYMBOL, "symbol precision mismatch");

        stats statstable(_self, sym.code().raw());
        check(statstable.find(sym.code().raw()) == statstable.end(), "token with symbol already exists");

        statstable.emplace(get_self(), [&](auto &s) {
            s.supply.symbol = maximum_supply.symbol;
            s.max_supply = maximum_supply;
        });
    }

    void token::issue(name to, asset quantity, string memo) {
        const auto sym = quantity.symbol;
        check(sym.is_valid(), "invalid symbol name");
        check(memo.size() <= 256, "memo has more than 256 bytes");
        check(quantity.symbol == FIOSYMBOL, "symbol precision mismatch");

        stats statstable(_self, sym.code().raw());
        auto existing = statstable.find(sym.code().raw());
        check(existing != statstable.end(), "token with symbol does not exist, create token before issue");
        const auto &st = *existing;

        require_auth(FIOISSUER);
        check(quantity.is_valid(), "invalid quantity");
        check(quantity.amount > 0, "must issue positive quantity");

        check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
        check(quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

        statstable.modify(st, same_payer, [&](auto &s) {
            s.supply += quantity;
        });

        add_balance(FIOISSUER, quantity, FIOISSUER);

        if (to != FIOISSUER) {
            SEND_INLINE_ACTION(*this, transfer, {{FIOISSUER, "active"_n}},
                               {FIOISSUER, to, quantity, memo}
            );
        }
    }


    void token::mintfio(const name &to, const uint64_t &amount) {
        //can only be called by fio.treasury@active
        require_auth(TREASURYACCOUNT);

        check((to == TREASURYACCOUNT || to == FOUNDATIONACCOUNT),
              "mint fio can only transfer to foundation or treasury accounts.");


        if (amount > 0 && amount < MAXFIOMINT) {
            action(permission_level{"eosio"_n, "active"_n},
                   "fio.token"_n, "issue"_n,
                   make_tuple(to, asset(amount, FIOSYMBOL),
                              string("New tokens produced from reserves"))
            ).send();
        }
    }

    void token::retire(asset quantity, string memo) {
        const symbol sym = quantity.symbol;
        check(sym.is_valid(), "invalid symbol name");
        check(memo.size() <= 256, "memo has more than 256 bytes");

        stats statstable(_self, sym.code().raw());
        auto existing = statstable.find(sym.code().raw());
        check(existing != statstable.end(), "token with symbol does not exist");
        const auto &st = *existing;

        require_auth(FIOISSUER);
        check(quantity.is_valid(), "invalid quantity");
        check(quantity.amount > 0, "must retire positive quantity");
        check(quantity.symbol == FIOSYMBOL, "symbol precision mismatch");

        check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");

        statstable.modify(st, same_payer, [&](auto &s) {
            s.supply -= quantity;
        });

        sub_balance(FIOISSUER, quantity);
    }

    bool token::can_transfer(const name &tokenowner, const uint64_t &feeamount, const uint64_t &transferamount,
                             const bool &isfee) {

        //get fio balance for this account,
        uint32_t present_time = now();
        const auto my_balance = eosio::token::get_balance("fio.token"_n, tokenowner, FIOSYMBOL.code());
        uint64_t amount = my_balance.amount;

        //see if the user is in the lockedtokens table, if so recompute the balance
        //based on grant type.
        auto lockiter = lockedTokensTable.find(tokenowner.value);
        if (lockiter != lockedTokensTable.end()) {
            //TEST LOCKED TOKENS uint32_t issueplus210 = lockiter->timestamp+(25*60);
            uint32_t issueplus210 = lockiter->timestamp + (210 * SECONDSPERDAY);

            if (
                //if lock type 1 or 2 or 3, 4 and not a fee subtract remaining locked amount from balance
                    (((lockiter->grant_type == 1) || (lockiter->grant_type == 2) || (lockiter->grant_type == 3) ||
                      (lockiter->grant_type == 4)) && !isfee) ||
                    //if lock type 2 and more than 210 days since grant and inhibit locking is set then subtract remaining locked amount from balance .
                    //this keeps the type 2 grant from being used for fees if the inhibit locking is not flipped after 210 days.
                    ((lockiter->grant_type == 2) && ((present_time > issueplus210) && lockiter->inhibit_unlocking))
                    ) {
                //recompute the remaining locked amount based on vesting.
                uint64_t lockedTokenAmount = computeremaininglockedtokens(tokenowner, false);//-feeamount;
                //subtract the lock amount from the balance
                if (lockedTokenAmount < amount) {
                    amount -= lockedTokenAmount;
                    return (amount >= transferamount);
                } else {
                    return false;
                }

            } else if (isfee) {

                uint64_t unlockedbalance = 0;
                if (amount > lockiter->remaining_locked_amount) {
                    unlockedbalance = amount - lockiter->remaining_locked_amount;
                }
                if (unlockedbalance >= transferamount) {
                    return true;
                } else {
                    uint64_t new_remaining_unlocked_amount =
                            lockiter->remaining_locked_amount - (transferamount - unlockedbalance);
                    INLINE_ACTION_SENDER(eosiosystem::system_contract, updlocked)
                            ("eosio"_n, {{_self, "active"_n}},
                             {tokenowner, new_remaining_unlocked_amount}
                            );
                    return true;
                }
            }
        } else {
            return true;
        }
        //avoid compile warning.
        return true;

    }

    bool token::can_transfer_general(const name &tokenowner, const uint64_t &transferamount) {
        bool dbg = true;
        //get fio balance for this account,
        uint32_t present_time = now();
        const auto my_balance = eosio::token::get_balance("fio.token"_n, tokenowner, FIOSYMBOL.code());

        uint64_t amount = my_balance.amount;
        if (dbg) {
            print("can_transfer_general can transfer general balance is ", amount, "\n");
        }

        //recompute the remaining locked amount based on vesting.
        uint64_t lockedTokenAmount = computegenerallockedtokens(tokenowner, false);
        if (dbg) {
            print("can_transfer_general locked amount is ", lockedTokenAmount, "\n");
        }
        //subtract the lock amount from the balance
        if (lockedTokenAmount < amount) {
            amount -= lockedTokenAmount;
            return (amount >= transferamount);
        } else {
            return false;
        }
    }

    name token::transfer_public_key(const string &payee_public_key,
                             const int64_t &amount,
                             const int64_t &max_fee,
                             const name &actor,
                             const string &tpid,
                             const int64_t &feeamount,
                             const bool &errorifaccountexists) {

        require_auth(actor);
        asset qty;

        fio_400_assert(isPubKeyValid(payee_public_key), "payee_public_key", payee_public_key,
                       "Invalid FIO Public Key", ErrorPubKeyValid);

        fio_400_assert(validateTPIDFormat(tpid), "tpid", tpid,
                       "TPID must be empty or valid FIO address",
                       ErrorPubKeyValid);

        qty.amount = amount;
        qty.symbol = FIOSYMBOL;

        fio_400_assert(amount > 0 && qty.amount > 0, "amount", std::to_string(amount),
                       "Invalid amount value", ErrorInvalidAmount);

        fio_400_assert(qty.is_valid(), "amount", std::to_string(amount), "Invalid amount value", ErrorLowFunds);

        fio_400_assert(max_fee >= 0, "max_fee", to_string(max_fee), "Invalid fee value.",
                       ErrorMaxFeeInvalid);
        
        string payee_account;
        fioio::key_to_account(payee_public_key, payee_account);

        name new_account_name = name(payee_account.c_str());
        bool accountExists = is_account(new_account_name);

        if (errorifaccountexists){
            fio_400_assert(!(accountExists), "payee_public_key", payee_public_key,
                           "Locked tokens can only be transferred to new account",
                           ErrorPubKeyValid);
        }
        auto other = eosionames.find(new_account_name.value);

        if (other == eosionames.end()) { //the name is not in the table.
            fio_400_assert(!accountExists, "payee_account", payee_account,
                           "Account exists on FIO chain but is not bound in eosionames",
                           ErrorPubAddressExist);

            const auto owner_pubkey = abieos::string_to_public_key(payee_public_key);

            eosiosystem::key_weight pubkey_weight = {
                    .key = owner_pubkey,
                    .weight = 1,
            };

            const auto owner_auth = authority{1, {pubkey_weight}, {}, {}};

            INLINE_ACTION_SENDER(call::eosio, newaccount)
                    ("eosio"_n, {{_self, "active"_n}},
                     {_self, new_account_name, owner_auth, owner_auth}
                    );

            action{
                    permission_level{_self, "active"_n},
                    AddressContract,
                    "bind2eosio"_n,
                    bind2eosio{
                            .accountName = new_account_name,
                            .public_key = payee_public_key,
                            .existing = accountExists
                    }
            }.send();

        } else {
            fio_400_assert(accountExists, "payee_account", payee_account,
                           "Account does not exist on FIO chain but is bound in eosionames",
                           ErrorPubAddressExist);

            eosio_assert_message_code(payee_public_key == other->clientkey, "FIO account already bound",
                                      fioio::ErrorPubAddressExist);
        }

        fio_fees(actor, asset{(int64_t) feeamount, FIOSYMBOL});
        process_rewards(tpid, feeamount,get_self(), new_account_name);

        require_recipient(actor);

        if (accountExists) {
            require_recipient(new_account_name);
        }

        INLINE_ACTION_SENDER(eosiosystem::system_contract, unlocktokens)
                ("eosio"_n, {{_self, "active"_n}},
                 {actor}
                );

        accounts from_acnts(_self, actor.value);
        const auto acnts_iter = from_acnts.find(FIOSYMBOL.code().raw());
        fio_400_assert(acnts_iter != from_acnts.end(), "amount", to_string(qty.amount),
                       "Insufficient balance",
                       ErrorLowFunds);
        fio_400_assert(acnts_iter->balance.amount >= qty.amount, "amount", to_string(qty.amount),
                       "Insufficient balance",
                       ErrorLowFunds);

        fio_400_assert(can_transfer(actor, feeamount, qty.amount, false), "amount", to_string(qty.amount),
                       "Insufficient balance tokens locked",
                       ErrorInsufficientUnlockedFunds);

        fio_400_assert(can_transfer_general(actor, qty.amount), "actor", to_string(actor.value),
                       "Funds locked",
                       ErrorInsufficientUnlockedFunds);

        sub_balance(actor, qty);
        add_balance(new_account_name, qty, actor);

        INLINE_ACTION_SENDER(eosiosystem::system_contract, updatepower)
                ("eosio"_n, {{_self, "active"_n}},
                 {actor, true}
                );

        if (accountExists) {
            INLINE_ACTION_SENDER(eosiosystem::system_contract, updatepower)
                    ("eosio"_n, {{_self, "active"_n}},
                     {new_account_name, true}
                    );
        }

        return new_account_name;
    }

    void token::transfer(name from,
                         name to,
                         asset quantity,
                         string memo) {

        /* we permit the use of transfer from the system account to any other accounts,
         * we permit the use of transfer from the treasury account to any other accounts.
         * we permit the use of transfer from any other accounts to the treasury account for fees.
         */
        if (from != SYSTEMACCOUNT && from != TREASURYACCOUNT) {
            check(to == TREASURYACCOUNT, "transfer not allowed");
        }
        eosio_assert((has_auth(SYSTEMACCOUNT) || has_auth(TREASURYACCOUNT)),
                     "missing required authority of treasury or eosio");


        check(from != to, "cannot transfer to self");
        check(is_account(to), "to account does not exist");
        auto sym = quantity.symbol.code();
        stats statstable(_self, sym.raw());
        const auto &st = statstable.get(sym.raw());

        require_recipient(from);
        require_recipient(to);

        check(quantity.is_valid(), "invalid quantity");
        check(quantity.amount > 0, "must transfer positive quantity");
        check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
        check(quantity.symbol == FIOSYMBOL, "symbol precision mismatch");
        check(memo.size() <= 256, "memo has more than 256 bytes");

        accounts from_acnts(_self, from.value);
        const auto acnts_iter = from_acnts.find(FIOSYMBOL.code().raw());

        fio_400_assert(acnts_iter != from_acnts.end(), "max_fee", to_string(quantity.amount),
                       "Insufficient funds to cover fee",
                       ErrorLowFunds);
        fio_400_assert(acnts_iter->balance.amount >= quantity.amount, "max_fee", to_string(quantity.amount),
                       "Insufficient funds to cover fee",
                       ErrorLowFunds);

        //we need to check the from, check for locked amount remaining
        fio_400_assert(can_transfer(from, 0, quantity.amount, true), "actor", to_string(from.value),
                       "Funds locked",
                       ErrorInsufficientUnlockedFunds);

        fio_400_assert(can_transfer_general(from, quantity.amount), "actor", to_string(from.value),
                       "Funds locked",
                       ErrorInsufficientUnlockedFunds);

        auto payer = has_auth(to) ? to : from;

        sub_balance(from, quantity);
        add_balance(to, quantity, payer);
    }

    void token::trnsfiopubky(const string &payee_public_key,
                             const int64_t &amount,
                             const int64_t &max_fee,
                             const name &actor,
                             const string &tpid) {

       uint128_t endpoint_hash = fioio::string_to_uint128_hash("transfer_tokens_pub_key");

       auto fees_by_endpoint = fiofees.get_index<"byendpoint"_n>();
       auto fee_iter = fees_by_endpoint.find(endpoint_hash);

       fio_400_assert(fee_iter != fees_by_endpoint.end(), "endpoint_name", "transfer_tokens_pub_key",
                      "FIO fee not found for endpoint", ErrorNoEndpoint);

       uint64_t reg_amount = fee_iter->suf_amount;
       uint64_t fee_type = fee_iter->type;

       fio_400_assert(fee_type == 0, "fee_type", to_string(fee_type),
                      "transfer_tokens_pub_key unexpected fee type for endpoint transfer_tokens_pub_key, expected 0",
                      ErrorNoEndpoint);

       fio_400_assert(max_fee >= reg_amount, "max_fee", to_string(max_fee), "Fee exceeds supplied maximum.",
                      ErrorMaxFeeExceeded);

        //do the transfer
        transfer_public_key(payee_public_key,amount,max_fee,actor,tpid,reg_amount,false);

        if (TRANSFERPUBKEYRAM > 0) {
            action(
                    permission_level{SYSTEMACCOUNT, "active"_n},
                    "eosio"_n,
                    "incram"_n,
                    std::make_tuple(actor, TRANSFERPUBKEYRAM)
            ).send();
        }

        const string response_string = string("{\"status\": \"OK\",\"fee_collected\":") +
                                       to_string(reg_amount) + string("}");

        fio_400_assert(transaction_size() <= MAX_TRX_SIZE, "transaction_size", std::to_string(transaction_size()),
          "Transaction is too large", ErrorTransactionTooLarge);

        send_response(response_string.c_str());

    }

    void token::trnsloctoks(const string &payee_public_key,
                             const int32_t &can_vote,
                             const vector<eosiosystem::lockperiods> periods,
                             const int64_t &amount,
                             const int64_t &max_fee,
                             const name &actor,
                             const string &tpid) {

        fio_400_assert(((periods.size()) >= 1 && (periods.size() <= 365)), "unlock_periods", "Invalid unlock periods",
                       "Invalid number of unlock periods", ErrorTransactionTooLarge);
        double totp = 0.0;
        double tv = 0.0;
        int64_t longestperiod = 0;
        for(int i=0;i<periods.size();i++){
            fio_400_assert(periods[i].percent > 0.0, "unlock_periods", "Invalid unlock periods",
                           "Invalid percentage value in unlock periods", ErrorInvalidUnlockPeriods);
            tv = periods[i].percent - (double(int(periods[i].percent * 1000.0)))/1000.0;
            fio_400_assert(tv == 0.0, "unlock_periods", "Invalid unlock periods",
                           "Invalid precision for percentage in unlock periods", ErrorInvalidUnlockPeriods);
            fio_400_assert(periods[i].duration > 0, "unlock_periods", "Invalid unlock periods",
                           "Invalid duration value in unlock periods", ErrorInvalidUnlockPeriods);
            totp += periods[i].percent;
            if (periods[i].duration > longestperiod){
                longestperiod = periods[i].duration;
            }
        }
        fio_400_assert(totp == 100.0, "unlock_periods", "Invalid unlock periods",
                       "Invalid total percentage for unlock periods", ErrorInvalidUnlockPeriods);

        fio_400_assert(((can_vote == 0)||(can_vote == 1)), "can_vote", to_string(can_vote),
                       "Invalid can_vote value", ErrorInvalidValue);


        bool dbg = true;

        if (dbg) {
            print(" calling trnsloctoks ");
        }

        uint128_t endpoint_hash = fioio::string_to_uint128_hash("transfer_locked_tokens");

        auto fees_by_endpoint = fiofees.get_index<"byendpoint"_n>();
        auto fee_iter = fees_by_endpoint.find(endpoint_hash);

        fio_400_assert(fee_iter != fees_by_endpoint.end(), "endpoint_name", "transfer_locked_tokens",
                       "FIO fee not found for endpoint", ErrorNoEndpoint);

        uint64_t reg_amount = fee_iter->suf_amount;
        uint64_t fee_type = fee_iter->type;

        fio_400_assert(fee_type == 0, "fee_type", to_string(fee_type),
                       "transfer_tokens_pub_key unexpected fee type for endpoint transfer_tokens_pub_key, expected 0",
                       ErrorNoEndpoint);

        fio_400_assert(max_fee >= reg_amount, "max_fee", to_string(max_fee), "Fee exceeds supplied maximum.",
                       ErrorMaxFeeExceeded);

        int64_t ninetydayperiods = longestperiod / (SECONDSPERDAY * 90);
        int64_t rem = longestperiod % (SECONDSPERDAY * 90);
        if (rem > 0){
            ninetydayperiods++;
        }
        reg_amount = ninetydayperiods * reg_amount;

        //check for pre existing account is done here.
        name owner = transfer_public_key(payee_public_key,amount,max_fee,actor,tpid,reg_amount,true);

        if (dbg) {
            print("trnsloctoks calling addgenlocked ", "\n");
        }

        bool canvote = (can_vote == 1);
        INLINE_ACTION_SENDER(eosiosystem::system_contract, addgenlocked)
                ("eosio"_n, {{_self, "active"_n}},
                 {owner,periods,canvote,amount}
                );

        int64_t raminc = 1024 + (64 * periods.size());

        action(
                permission_level{SYSTEMACCOUNT, "active"_n},
                "eosio"_n,
                "incram"_n,
                std::make_tuple(actor, raminc)
                ).send();


        const string response_string = string("{\"status\": \"OK\",\"fee_collected\":") +
                                       to_string(reg_amount) + string("}");

        fio_400_assert(transaction_size() <= MAX_TRX_SIZE, "transaction_size", std::to_string(transaction_size()),
                       "Transaction is too large", ErrorTransactionTooLarge);

        send_response(response_string.c_str());

    }


    void token::sub_balance(name owner, asset value) {
        accounts from_acnts(_self, owner.value);
        const auto &from = from_acnts.get(value.symbol.code().raw(), "no balance object found");

        fio_400_assert(from.balance.amount >= value.amount, "amount", to_string(value.amount),
                       "Insufficient balance", ErrorLowFunds);

        from_acnts.modify(from, owner, [&](auto &a) {
            a.balance -= value;
        });
    }

    void token::add_balance(name owner, asset value, name ram_payer) {
        accounts to_acnts(_self, owner.value);
        auto to = to_acnts.find(value.symbol.code().raw());
        if (to == to_acnts.end()) {
            to_acnts.emplace(ram_payer, [&](auto &a) {
                a.balance = value;
            });
        } else {
            to_acnts.modify(to, same_payer, [&](auto &a) {
                a.balance += value;
            });
        }
    }
} /// namespace eosio

EOSIO_DISPATCH( eosio::token, (create)(issue)(mintfio)(transfer)(trnsfiopubky)(trnsloctoks)
(retire))
