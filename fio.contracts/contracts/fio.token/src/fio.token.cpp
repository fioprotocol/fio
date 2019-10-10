/** FioToken implementation file
 *  Description: FioToken is the smart contract that help manage the FIO Token.
 *  @author Adam Androulidakis, Casey Gardiner
 *  @modifedby
 *  @file fio.token.cpp
 *  @copyright Dapix
 */

#include "fio.token/fio.token.hpp"

using namespace fioio;

namespace eosio {

    void token::create(asset maximum_supply) {
        require_auth(_self);

        auto sym = maximum_supply.symbol;
        check(sym.is_valid(), "invalid symbol name");
        check(maximum_supply.is_valid(), "invalid supply");
        check(maximum_supply.amount > 0, "max-supply must be positive");
        check(maximum_supply.symbol == FIOSYMBOL, "symbol precision mismatch");

        stats statstable(_self, sym.code().raw());
        auto existing = statstable.find(sym.code().raw());
        check(existing == statstable.end(), "token with symbol already exists");

        statstable.emplace(_self, [&](auto &s) {
            s.supply.symbol = maximum_supply.symbol;
            s.max_supply = maximum_supply;
        });
    }

    void token::issue(name to, asset quantity, string memo) {
        auto sym = quantity.symbol;
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

    void token::mintfio(const uint64_t &amount) {
        //can only be called by fio.treasury@active
        require_auth("fio.treasury"_n);
        if (amount > 0 && amount < 100000000000000000) { //100,000,000 FIO max can be minted by this call
            print("\n\nMintfio called\n");
            action(permission_level{"eosio"_n, "active"_n},
                   "fio.token"_n, "issue"_n,
                   make_tuple("fio.treasury"_n, asset(amount, symbol("FIO", 9)),
                              string("New tokens produced from reserves"))
            ).send();
        }
    }

    void token::retire(asset quantity, string memo) {
        symbol sym = quantity.symbol;

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

    void token::transfer(name from,
                         name to,
                         asset quantity,
                         string memo) {


        check(from != to, "cannot transfer to self");
        require_auth(from);
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

        auto payer = has_auth(to) ? to : from;

        sub_balance(from, quantity);
        add_balance(to, quantity, payer);
    }

    inline void token::fio_fees(const name &actor, const asset &fee) {
        if (appConfig.pmtson) {
            // check for funds is implicitly done as part of the funds transfer.
            print("Collecting FIO API fees: ", fee);
            transfer(actor, "fio.treasury"_n, fee, string("FIO API fees. Thank you."));
        } else {
            print("Payments currently disabled.");
        }
    }

    inline int64_t stoi(const char *s) //1000000000000
    {
        int64_t i;
        i = 0;

        while (*s >= '0' && *s <= '9') {
            i = i * 10 + (*s - '0');
            s++;
        }
        return i;
    }

    void token::trnsfiopubky(string payee_public_key,
                             string amount,
                             uint64_t max_fee,
                             name actor,
                             const string &tpid) {

        asset qty;
        int64_t i64 = stoi(amount.c_str());
        qty.amount = i64;
        qty.symbol = symbol("FIO", 9);

        fio_400_assert(payee_public_key.length() == 53, "payee_public_key", payee_public_key,
                       "Invalid Public Key", ErrorChainAddressNotFound);

        string pubkey_prefix("FIO");
        auto result = mismatch(pubkey_prefix.begin(), pubkey_prefix.end(),
                               payee_public_key.begin());
        eosio_assert(result.first == pubkey_prefix.end(),
                     "Public key should be prefix with FIO");
        auto base58substr = payee_public_key.substr(pubkey_prefix.length());

        vector<unsigned char> vch;
        eosio_assert(fioio::decode_base58(base58substr, vch), "Decode pubkey failed");
        fio_400_assert(vch.size() == 37, "payee_public_address", payee_public_key, "Invalid FIO Public Key",
                       fioio::ErrorChainAddressNotFound);

        array<unsigned char, 33> pubkey_data;
        copy_n(vch.begin(), 33, pubkey_data.begin());

        capi_checksum160 check_pubkey;
        ripemd160(reinterpret_cast<char *>(pubkey_data.data()), 33, &check_pubkey);
        fio_400_assert(memcmp(&check_pubkey.hash, &vch.end()[-4], 4) == 0, "payee_public_address", payee_public_key,
                       "Invalid FIO Public Key", ErrorChainAddressNotFound);

        string payee_account;
        fioio::key_to_account(payee_public_key, payee_account);

        print("hashed account name from the payee_public_key ", payee_account, "\n");

        eosio_assert(payee_account.length() == 12, "Length of account name should be 12");
        name new_account_name = name(payee_account.c_str());
        bool accountExists = is_account(new_account_name);

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

            print("created the account!!!!", new_account_name, "\n");

            action{
                    permission_level{_self, "active"_n},
                    "fio.system"_n,
                    "bind2eosio"_n,
                    bind2eosio{
                            .accountName = new_account_name,
                            .public_key = payee_public_key,
                            .existing = accountExists
                    }
            }.send();

            print("performed bind of the account!!!!", new_account_name, "\n");
        } else {
            fio_400_assert(accountExists, "payee_account", payee_account,
                           "Account does not exist on FIO chain but is bound in eosionames",
                           ErrorPubAddressExist);

            eosio_assert_message_code(payee_public_key == other->clientkey, "FIO account already bound",
                                      fioio::ErrorPubAddressExist);
        }

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

        auto sym = qty.symbol;
        stats statstable(_self, sym.code().raw());
        const auto &st = statstable.get(sym.code().raw());

        asset reg_fee_asset = asset();
        reg_fee_asset.amount = reg_amount;
        reg_fee_asset.symbol = symbol("FIO", 9);

        fio_fees(actor, reg_fee_asset);
        process_rewards(tpid, reg_amount, get_self());

        require_recipient(actor);

        if (accountExists) {
            require_recipient(new_account_name);
        }

        fio_400_assert(qty.is_valid(), "amount", amount.c_str(), "Invalid quantity", ErrorLowFunds);
        eosio_assert(qty.amount > 0, "must transfer positive quantity");
        eosio_assert(qty.symbol == st.supply.symbol, "symbol precision mismatch");

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

        nlohmann::json json = {{"status",        "OK"},
                               {"fee_collected", reg_amount}};

        send_response(json.dump().c_str());
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

EOSIO_DISPATCH( eosio::token, (create)(issue)(mintfio)(transfer)(trnsfiopubky)
(retire))
