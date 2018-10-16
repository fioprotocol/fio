#pragma once

#include <vector>
#include <string>
//#include <time>

#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/asset.hpp>

#ifndef FEE_CONTRACT
#define FEE_CONTRACT "fio.fee"
#endif

#ifndef TOKEN_CONTRACT
#define TOKEN_CONTRACT "eosio.token"
#endif

namespace fioio {

    using namespace eosio;
    using namespace std;

    // Currently supported blockchains
    enum  class chain_type {
        FIO=0, EOS=1, BTC=2, ETH=3, XMR=4, BRD=5, BCH=6, NONE=7
    }; // enum  class chain_type

//    static const string Chain_fio_str = "FIO";
//    static const string Chain_eos_str = "EOS";
//    static const string Chain_btc_str = "BTC";
//    static const string Chain_eth_str = "ETH";
//    static const string Chain_xmr_str = "XMR";
//    static const string Chain_brd_str = "BRD";
//    static const string Chain_bch_str = "BCH";

//    // Three letter acronyms for individual blockchains. The entries are matched to ${chain_type} positions.
    static const std::vector<std::string> chain_str {"FIO", "EOS", "BTC", "ETH", "XMR", "BRD", "BCH"};

    struct trxfee {

        // election info
        uint64_t    id = 0;             // current election id
        time        expiration = now(); // current election expiration

        // wallet names associated fees
        asset  domregiter = asset(0, S(4, FIO));    // Fee paid upon the original domain registration/renewal by the user registering. Allows the owner to retain ownership
        // of the wallet domain for a period of 1 year or until transfer
        asset  nameregister = asset(0, S(4, FIO));  // Fee paid upon the original name registration/renewal by the user registering. Allows the owner to retain ownership
        // of the wallet name for a period of 1 year or until the expiration date of wallet domain. Re-sets the counter for Fee-free Transaction.
        asset  domtransfer = asset(0, S(4, FIO));   // Fee paid upon wallet domain transfer of ownership by the transferring user.
        asset  nametransfer = asset(0, S(4, FIO));  // Fee paid upon wallet name transfer of ownership by the transferring user.
        asset  namelookup = asset(0, S(4, FIO));    // Fee paid for looking up a public address for a given wallet name and coin.
        asset  upaddress = asset(0, S(4, FIO));     // Fees paid when wallet name to public address mapping is updated.

        // taken associated fees
        asset  transfer = asset(0, S(4, FIO));  // Fee paid when FIO token is transferred.

        // meta-data associated fees
        asset metadata = asset(0, S(4, FIO));    // Fee paind when recording information about the transaction (i.e. status or Request)

        EOSLIB_SERIALIZE(trxfee, (id)(expiration) (domregiter)(nameregister)(domtransfer)(nametransfer)(namelookup)(upaddress)(transfer)(metadata))
    }; // struct trxfee
    typedef singleton<N(trxfees), trxfee> trxfees_singleton;
    static const account_name FeeContract = eosio::string_to_name(FEE_CONTRACT);

} // namespace fioio
