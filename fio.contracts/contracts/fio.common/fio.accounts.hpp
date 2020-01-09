/** FioAccounts implementation file
 *  Description: holds the account names used by the FIO protocol
 *  @author Ed Rotthoff
 *  @modifedby
 *  @file fio.accounts.cpp
 *  @copyright Dapix
 */

#pragma once



namespace fioio {
    using eosio::name;

    static const name MSIGACCOUNT = name("eosio.msig");
    static const name REQOBTACCOUNT = name("fio.reqobt");
    static const name WHITELISTACCOUNT = name("fio.whitelst");
    static const name WRAPACCOUNT = name("eosio.wrap");
    static const name FeeContract = name("fio.fee");    // account hosting the fee contract
    static const name AddressContract = name("fio.address");
    static const name TPIDContract = name("fio.tpid");
    static const name TokenContract = name("fio.token");
    static const name FOUNDATIONACCOUNT = name("fio.foundatn");
    static const name TREASURYACCOUNT = name("fio.treasury");
    static const name SYSTEMACCOUNT = name("eosio");
    static constexpr name FIOISSUER = name("eosio"_n);
    static constexpr eosio::symbol FIOSYMBOL = eosio::symbol("FIO", 9);
    //owner permission
    static const name OWNER = name("owner");
    static const name ACTIVE = name("active");

} // namespace fioio
