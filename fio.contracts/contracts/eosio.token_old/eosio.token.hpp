/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <string>
#include "../fio.name/fio.name.hpp"
#include "../fio.fee/fio.fee.hpp"
#include <fio.common/fio.common.hpp>

namespace eosiosystem {
   class system_contract;
}

namespace eosio {

   using std::string;

   class token : public contract {
      private:
       fioio::eosio_names_table eosionames;
       fioio::fiofee_table  fiofees;
       fioio::config appConfig;
      public:
         token( account_name self ):contract(self),
         eosionames(N(fio.system), N(fio.system)),
         fiofees(fioio::FeeContract, fioio::FeeContract){
             fioio::configs_singleton configsSingleton(fioio::FeeContract, fioio::FeeContract);
             appConfig = configsSingleton.get_or_default(fioio::config());
         }

         void create( account_name issuer,
                      asset        maximum_supply);

         void issue( account_name to, asset quantity, string memo );
         inline void fio_fees(const account_name &actor, const asset &fee);

       void transfer( account_name from,
                        account_name to,
                        asset        quantity,
                        string       memo );

         void trnsfiopubky( string payee_public_key,
                         string amount,
                         uint64_t max_fee,
                         name actor);


         inline asset get_supply( symbol_name sym )const;

         inline asset get_balance( account_name owner, symbol_name sym )const;

      private:
         struct account {
            asset    balance;

            uint64_t primary_key()const { return balance.symbol.name(); }
         };

         struct currency_stats {
            asset          supply;
            asset          max_supply;
            account_name   issuer;

            uint64_t primary_key()const { return supply.symbol.name(); }
         };

         typedef eosio::multi_index<N(accounts), account> accounts;
         typedef eosio::multi_index<N(stat), currency_stats> stats;

         void sub_balance( account_name owner, asset value );
         void add_balance( account_name owner, asset value, account_name ram_payer );

      public:
         struct transfer_args {
            account_name  from;
            account_name  to;
            asset         quantity;
            string        memo;
         };

       struct bind2eosio {
           account_name accountName;
           string public_key;
           bool existing;
       };
   };

   asset token::get_supply( symbol_name sym )const
   {
      stats statstable( _self, sym );
      const auto& st = statstable.get( sym );
      return st.supply;
   }

   asset token::get_balance( account_name owner, symbol_name sym )const
   {
      accounts accountstable( _self, owner );
      const auto& ac = accountstable.get( sym );
      return ac.balance;
   }

} /// namespace eosio
