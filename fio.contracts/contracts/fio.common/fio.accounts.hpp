/** FioAccounts implementation file
 *  Description: holds the account names used by the FIO protocol
 *  @author Ed Rotthoff, Adam Androulidakis
 *  @modifedby
 *  @file fio.accounts.cpp
 *  @license FIO Foundation ( https://github.com/fioprotocol/fio/blob/master/LICENSE ) Dapix
 */

#pragma once

#define MAX_TRX_SIZE 8098

namespace fioio {
    using eosio::name;

    static const name MSIGACCOUNT =      name("eosio.msig");
    static const name WRAPACCOUNT =      name("eosio.wrap");
    static const name SYSTEMACCOUNT =    name("eosio");
    static const name ASSERTACCOUNT =    name("eosio.assert");


    //these are legacy system account names from EOS, we might consider blocking these.
    static const name BPAYACCOUNT =      name("eosio.bpay");
    static const name NAMESACCOUNT =     name("eosio.names");
    static const name RAMACCOUNT =       name("eosio.ram");
    static const name RAMFEEACCOUNT =    name("eosio.ramfee");
    static const name SAVINGACCOUNT =    name("eosio.saving");
    static const name STAKEACCOUNT =     name("eosio.stake");
    static const name VPAYACCOUNT =      name("eosio.vpay");


    static const name REQOBTACCOUNT =     name("fio.reqobt");
    static const name FeeContract =       name("fio.fee");
    static const name AddressContract =   name("fio.address");
    static const name TPIDContract =      name("fio.tpid");
    static const name TokenContract =     name("fio.token");
    static const name FOUNDATIONACCOUNT = name("tw4tjkmo4eyd");
    static const name TREASURYACCOUNT =   name("fio.treasury");
    static const name FIOSYSTEMACCOUNT=   name("fio.system");
    static const name FIOACCOUNT =   name("fio");

    static constexpr name FIOISSUER = name("eosio"_n);
    static constexpr eosio::symbol FIOSYMBOL = eosio::symbol("FIO", 9);
    //owner permission
    static const name OWNER = name("owner");
    static const name ACTIVE = name("active");

} // namespace fioio
