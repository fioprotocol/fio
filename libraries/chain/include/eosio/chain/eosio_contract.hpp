/**
 *  @file
 *  @copyright defined in fio/LICENSE
 */
#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/contract_types.hpp>

namespace eosio {
    namespace chain {

        class apply_context;

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

        /**
         * @defgroup native_action_handlers Native Action Handlers
         */
        ///@{
        void apply_eosio_newaccount(apply_context &);

        void apply_eosio_updateauth(apply_context &);

        void apply_eosio_deleteauth(apply_context &);

        void apply_eosio_linkauth(apply_context &);

        void apply_eosio_unlinkauth(apply_context &);

        /*
        void apply_eosio_postrecovery(apply_context&);
        void apply_eosio_passrecovery(apply_context&);
        void apply_eosio_vetorecovery(apply_context&);
        */

        void apply_eosio_setcode(apply_context &);

        void apply_eosio_setabi(apply_context &);

        void apply_eosio_canceldelay(apply_context &);
        ///@}  end action handlers

    }
} /// namespace eosio::chain
