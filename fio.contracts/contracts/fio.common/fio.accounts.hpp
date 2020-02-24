/** FioAccounts implementation file
 *  Description: holds the account names used by the FIO protocol
 *  @author Ed Rotthoff, Adam Androulidakis
 *  @modifedby
 *  @file fio.accounts.cpp
 *  @license FIO Foundation ( https://github.com/fioprotocol/fio/blob/master/LICENSE ) Dapix
 */

#pragma once

#define MAX_REGDOMAIN_TRANSACTION_SIZE 256
#define MAX_REGADDRESS_TRANSACTION_SIZE 256
#define MAX_ADDADDRESS_TRANSACTION_SIZE 1024
#define MAX_SETDOMPUB_TRANSACTION_SIZE 256
#define MAX_BIND2EOSIO_TRANSACTION_SIZE 86
#define MAX_BURNEXPIRED_TRANSACTION_SIZE 86
#define MAX_NEWFUNDSREQUEST_TRANSACTION_SIZE 768
#define MAX_RECORDOBT_TRANSACTION_SIZE 768
#define MAX_RENEWADDRESS_TRANSACTION_SIZE 256
#define MAX_RENEWDOMAIN_TRANSACTION_SIZE 256
#define MAX_TPIDCLAIM_TRANSACTION_SIZE 128
#define MAX_BPCLAIM_TRANSACTION_SIZE 128
#define MAX_TRANSFER_TRANSACTION_SIZE 256
#define MAX_REJECTFUNDS_TRANSACTION_SIZE 128
#define MAX_UPDATEFEES_TRANSACTION_SIZE 512
#define MAX_SETFEEVOTE_TRANSACTION_SIZE 512
#define MAX_SETFEEMULT_TRANSACTION_SIZE 256
#define MAX_MANDATORYFEE_TRANSACTION_SIZE 96
#define MAX_CREATEFEE_TRANSACTION_SIZE 182
#define MAX_BYTEMANDFEE_TRANSACTION_SIZE 128
#define MAX_BUNDLEVOTE_TRANSACTION_SIZE 256
#define MAX_ADDLOCKED_TRANSACTION_SIZE 96
#define MAX_BURNACTION_TRANASCTION_SIZE 256
#define MAX_CANCELDELAY_TRANSACTION_SIZE 128
#define MAX_CRAUTOPROXY_TRANSACTION_SIZE 128
#define MAX_DELETEAUTH_TRANASCTION_SIZE 128
#define MAX_INHIBITLOCK_TRANSACTION_SIZE 128
#define MAX_INIT_TRANSACTION_SIZE 128
#define MAX_LINKAUTH_TRANSACTION_SIZE 256
#define MAX_NEWACCOUNT_TRANSACTION_SIZE 256
#define MAX_REGPRODUCER_TRANSACTION_SIZE 256
#define MAX_REGPROXY_TRANSACTION_SIZE 212
#define MAX_REGIPRODUCER_TRANSACTION_SIZE 256
#define MAX_REGIPROXY_TRANSACTION_SIZE 128
#define MAX_RESETCLAIM_TRANSACTION_SIZE 86
#define MAX_RMVPRODUCER_TRANSACTION_SIZE 128
#define MAX_VOTEPROXY_TRANSACTION_SIZE 172
#define MAX_VOTEPRODUCER_TRANSACTION_SIZE 2128
#define MAX_UNREGPROXY_TRANSACTION_SIZE 128
#define MAX_UNREGPROD_TRANSACTION_SIZE 128
#define MAX_UNLOCKTOKENS_TRANSACTION_SIZE 128
#define MAX_SETPRIV_TRANSACTION_SIZE 128
#define MAX_EXEC_TRANSACTION_SIZE 96
#define MAX_APPROVE_TRANSACTION_SIZE 96
#define MAX_INVALIDATE_TRANSACTION_SIZE 96
#define MAX_UNAPPROVE_TRANSACTION_SIZE 96
#define MAX_CANCEL_TRANSACTION_SIZE 96

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
    static const name FOUNDATIONACCOUNT = name("fio.foundatn");
    static const name TREASURYACCOUNT =   name("fio.treasury");
    static const name FIOSYSTEMACCOUNT=   name("fio.system");
    static const name FIOACCOUNT =   name("fio");

    static constexpr name FIOISSUER = name("eosio"_n);
    static constexpr eosio::symbol FIOSYMBOL = eosio::symbol("FIO", 9);
    //owner permission
    static const name OWNER = name("owner");
    static const name ACTIVE = name("active");
} // namespace fioio
