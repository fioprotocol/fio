/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include "eosio.token.hpp"
#include <eosio/chain/fioio/fioerror.hpp>
#include <fio.common/fio.common.hpp>
#include <fio.common/json.hpp>
#include <fio.common/keyops.hpp>
#include <fio.common/account_operations.hpp>
#include <eosiolib/asset.hpp>
#include <eosio/chain/fioio/fio_common_validator.hpp>
#include <eosio/chain/fioio/chain_control.hpp>

using namespace fioio;
namespace eosio {

void token::create( account_name issuer,
                    asset        maximum_supply )
{
    require_auth( _self );

    auto sym = maximum_supply.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( maximum_supply.is_valid(), "invalid supply");
    eosio_assert( maximum_supply.amount > 0, "max-supply must be positive");

    stats statstable( _self, sym.name() );
    auto existing = statstable.find( sym.name() );
    eosio_assert( existing == statstable.end(), "token with symbol already exists" );

    statstable.emplace( _self, [&]( auto& s ) {
       s.supply.symbol = maximum_supply.symbol;
       s.max_supply    = maximum_supply;
       s.issuer        = issuer;
    });
}


void token::issue( account_name to, asset quantity, string memo )
{
    auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    auto sym_name = sym.name();
    stats statstable( _self, sym_name );
    auto existing = statstable.find( sym_name );
    eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
    const auto& st = *existing;

    require_auth( st.issuer );
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity" );

    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

    statstable.modify( st, 0, [&]( auto& s ) {
       s.supply += quantity;
    });

    add_balance( st.issuer, quantity, st.issuer );

    if( to != st.issuer ) {
       SEND_INLINE_ACTION( *this, transfer, {st.issuer,N(active)}, {st.issuer, to, quantity, memo} );
    }
}

void token::transfer( account_name from,
                      account_name to,
                      asset        quantity,
                      string       memo )
{
    fio_400_assert( from != to, "payee_public_address", "NULL","Invalid FIO Public Address", ErrorChainAddressNotFound );
    require_auth( from );
    fio_400_assert( is_account( to ), "payee_public_address", "NULL","Invalid FIO Public Address", ErrorChainAddressNotFound );
    auto sym = quantity.symbol.name();
    stats statstable( _self, sym );
    const auto& st = statstable.get( sym );

    require_recipient( from );
    require_recipient( to );

    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );


    sub_balance( from, quantity );
    add_balance( to, quantity, from );
}

inline void token::fio_fees(const account_name &actor, const asset &fee) {
        if (appConfig.pmtson) {
            account_name fiosystem = eosio::string_to_name("fio.system");
            // check for funds is implicitly done as part of the funds transfer.
            print("Collecting FIO API fees: ", fee);
            transfer(actor,fiosystem,fee, string("FIO API fees. Thank you."));
        } else {
            print("Payments currently disabled.");
        }
}


void token::trnsfiopubky( string      payee_public_key,
                         string       amount,
                         uint64_t     max_fee,
                         name         actor)
                         {

    asset qty;
    //we assume the amount is in fio sufs.
    qty.amount = (int64_t)atoi(amount.c_str());
    qty.symbol = ::eosio::string_to_symbol(9,"FIO");

    ///BEGIN new account management logic!!!!

    //first check the pub key for validity.
   fio_400_assert(payee_public_key.length() == 53, "payee_public_key", payee_public_key,
                           "Invalid Public Key", ErrorChainAddressNotFound);

    string pubkey_prefix("EOS");
    auto result = mismatch(pubkey_prefix.begin(), pubkey_prefix.end(),
                           payee_public_key.begin());
    eosio_assert(result.first == pubkey_prefix.end(),
            "Public key should be prefix with EOS");
    auto base58substr = payee_public_key.substr(pubkey_prefix.length());

    vector<unsigned char> vch;
    eosio_assert(decode_base58(base58substr, vch), "Decode pubkey failed");
    fio_400_assert( vch.size() == 37, "payee_public_address", payee_public_key, "Invalid FIO Public Key", ErrorChainAddressNotFound );

                             array<unsigned char, 33> pubkey_data;
    copy_n(vch.begin(), 33, pubkey_data.begin());

    checksum160 check_pubkey;
    ripemd160(reinterpret_cast<char *>(pubkey_data.data()), 33, &check_pubkey);
    fio_400_assert( memcmp(&check_pubkey.hash, &vch.end()[-4], 4) == 0, "payee_public_address", payee_public_key, "Invalid FIO Public Key", ErrorChainAddressNotFound );

    string payee_account;
    fioio::key_to_account(payee_public_key, payee_account);

    print("hashed account name from the payee_public_key ", payee_account, "\n");

    //see if the payee_actor is in the eosionames table.
    eosio_assert(payee_account.length() == 12, "Length of account name should be 12");
    account_name new_account_name = string_to_name(payee_account.c_str());
    bool accountExists = is_account(new_account_name);

    auto other = eosionames.find(new_account_name);

    if (other == eosionames.end()) { //the name is not in the table.
       // if account does exist on the chain this is an error. DANGER account was created without binding!
        fio_400_assert(!accountExists, "payee_account", payee_account,
                "Account exists on FIO chain but is not bound in eosionames",
                ErrorPubAddressExist);

        //the account does not exist on the fio chain yet, and the binding does not exists
        //yet, so create the account and then and add it to the eosionames table.
        const auto owner_pubkey = abieos::string_to_public_key(payee_public_key);

        eosiosystem::key_weight pubkey_weight = {
                .key = owner_pubkey,
                .weight = 1,
                };

        const auto owner_auth = authority{1, {pubkey_weight}, {}, {}};
        const auto rbprice = rambytes_price(3 * 1024);

        // Create account.
        INLINE_ACTION_SENDER(call::eosio, newaccount)
        (N(eosio), {{_self, N(active)}},
                {_self, new_account_name, owner_auth, owner_auth});

        // Buy ram for account.
        INLINE_ACTION_SENDER(eosiosystem::system_contract, buyram)
        (N(eosio), {{_self, N(active)}},
                {_self, new_account_name, rbprice});

        // Replace lost ram.
        INLINE_ACTION_SENDER(eosiosystem::system_contract, buyram)
        (N(eosio), {{_self, N(active)}},
                {_self, _self, rbprice});

        print("created the account!!!!", new_account_name, "\n");

        action {
                permission_level{_self, N(active)},
                N(fio.system),
                N(bind2eosio),
                bind2eosio {
                        .accountName = new_account_name,
                        .public_key = payee_public_key,
                        .existing = accountExists
                }
        }.send();

        print("performed bind of the account!!!!", new_account_name, "\n");
    } else {
        //if account does not on the chain this is an error. DANGER binding was recorded without the associated account.
        fio_400_assert(accountExists, "payee_account", payee_account,
                "Account does not exist on FIO chain but is bound in eosionames",
                ErrorPubAddressExist);
        //if the payee public key doesnt match whats in the eosionames table this is an error,it means there is a collision on hashing!
        eosio_assert_message_code(payee_public_key == other->clientkey, "FIO account already bound",
                                  ErrorPubAddressExist);
    }

    ///end new account management logic!!!!!
    //special note, though we have created the account and can use it herein,
    //the account is not yet officially on the chain and is_account will return false.
    //require_recipient will also fail.

    //begin new fees, logic for Mandatory fees.
    uint64_t endpoint_hash = string_to_uint64_t("transfer_tokens_to_pub_key");

    auto fees_by_endpoint = fiofees.get_index<N(byendpoint)>();
    auto fee_iter = fees_by_endpoint.find(endpoint_hash);
    //if the fee isnt found for the endpoint, then 400 error.
    fio_400_assert(fee_iter != fees_by_endpoint.end(),"endpoint_name", "transfer_tokens_to_pub_key",
            "FIO fee not found for endpoint", ErrorNoEndpoint);

    uint64_t reg_amount = fee_iter->suf_amount;
    uint64_t fee_type = fee_iter->type;

    //if its not a mandatory fee then this is an error.
    fio_400_assert(fee_type == 0,"fee_type", to_string(fee_type),
            "transfer_tokens_to_pub_key unexpected fee type for endpoint transfer_tokens_to_pub_key, expected 0", ErrorNoEndpoint);

    fio_400_assert(max_fee >= reg_amount, "max_fee", to_string(max_fee), "Fee exceeds supplied maximum.",ErrorMaxFeeExceeded );

    asset reg_fee_asset = asset(reg_amount);
    fio_fees(actor, reg_fee_asset);
    //end new fees, logic for Mandatory fees.

    auto sym = qty.symbol.name();
    stats statstable( _self, sym );
    const auto& st = statstable.get( sym );

    require_recipient( actor );
    //require recipient if the account was found on the chain.
    if (accountExists) {
        require_recipient(new_account_name);
    }

    fio_400_assert(qty.is_valid(),"amount", amount.c_str(), "Invalid quantity", ErrorLowFunds);
    eosio_assert( qty.amount > 0, "must transfer positive quantity" );
    eosio_assert( qty.symbol == st.supply.symbol, "symbol precision mismatch" );

    sub_balance( actor, qty );
    add_balance( new_account_name, qty, actor );

    nlohmann::json json = {{"status",     "OK"},
                           {"fee_collected",reg_amount}};
    send_response(json.dump().c_str());
}


void token::sub_balance( account_name owner, asset value ) {
   accounts from_acnts( _self, owner );

   const auto& from = from_acnts.get( value.symbol.name(), "Insufficient balance" );
   fio_400_assert(from.balance.amount >= value.amount,"amount", "", "Insufficient balance", ErrorLowFunds);


   if( from.balance.amount == value.amount ) {
      from_acnts.erase( from );
   } else {
      from_acnts.modify( from, owner, [&]( auto& a ) {
          a.balance -= value;
      });
   }
}

void token::add_balance( account_name owner, asset value, account_name ram_payer )
{
   accounts to_acnts( _self, owner );
   auto to = to_acnts.find( value.symbol.name() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      to_acnts.modify( to, 0, [&]( auto& a ) {
        a.balance += value;
      });
   }
}

} /// namespace eosio

EOSIO_ABI( eosio::token, (create)(issue)(transfer)(trnsfiopubky) )
