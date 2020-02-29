/** FioError definitions file
 *  Description: Mapping of actions to account names for use in processing FIO transactions.
 *  @author Casey Gardiner
 *  @file actionmapping.hpp
 *  @license FIO Foundation ( https://github.com/fioprotocol/fio/blob/master/LICENSE )
 *
 *  Changes:
 */

#pragma once

#include <string>

namespace fioio {

    static std::string map_to_contract(const std::string &action) {

        // msig actions
        if (action == "approve" || action == "cancel" || action == "invalidate" ||
            action == "exec" || action == "propose" || action == "unapprove")
          return "eosio.msig";

        // fio.address actions
        if (action == "regaddress" || action == "regdomain" || action == "addaddress" ||
            action == "renewdomain" || action == "renewaddress" ||
            action == "setdomainpub" || action == "bind2eosio" ||
            action == "burnexpired" || action == "decrcounter" )
          return "fio.address";



        // fio.fee actions
        if (action == "setfeemult" || action == "bundlevote" || action == "setfeevote" ||
            action == "bytemandfee" || action == "updatefees" || action == "mandatoryfee" ||
            action == "createfee")
          return "fio.fee";


        // fio.treasury actions
        if (action == "tpidclaim" || action == "bpclaim" || action == "bppoolupdate" ||
            action == "fdtnrwdupdat" || action == "bprewdupdate" || action == "startclock" ||
            action == "updateclock")
          return "fio.treasury";

        //fio.token actions
        if (action == "trnsfiopubky" || action == "create" || action == "issue" ||
            action == "transfer" || action == "mintfio")
          return "fio.token";
        //fio.request.obt actions
        if (action == "recordobt" || action == "rejectfndreq" || action == "newfundsreq")
          return "fio.reqobt";

        //fio.tpid actions
        if (action == "updatebounty" || action == "rewardspaid" || action == "updatetpid")
          return "fio.tpid";
          
        // eosio.wrap actions
        if (action == "execute")
          return "eosio.wrap";

        //system actions
        if (action == "newaccount" || action == "onblock" || action == "addlocked" ||
            action == "regproducer" || action == "unregprod" || action == "regproxy" ||
            action == "voteproducer" || action == "unregproxy" || action == "voteproxy" ||
            action == "setabi" || action == "setcode" || action == "updateauth" ||
            action == "setprods" || action == "setpriv" || action == "init" ||
            action == "nonce" || action == "burnaction" || action == "canceldelay" ||
            action == "crautoproxy" || action == "deleteauth" || action == "inhibitunlck" ||
            action == "linkauth" || action == "onerror" ||
            action == "rmvproducer" || action == "setautoproxy" || action == "setparams" ||
            action == "unlocktokens" || action == "updtrevision" ||action == "updlocked" ||
            action == "updatepower" ||
            action == "updlbpclaim" || action == "resetclaim" || action == "incram")
          return "eosio";

        return "nomap";
    }


}
