/** Fio system contract file
 *  Description: this file contains actions and data structures pertaining to voting in the fio protocol
 *  @author Adam Androulidakis, Casey Gardiner, Ed Rotthoff
 *  @file voting.cpp
 *  @license FIO Foundation ( https://github.com/fioprotocol/fio/blob/master/LICENSE ) Dapix
 *
 *  Changes:
 */
#include <fio.system/fio.system.hpp>

#include <eosiolib/eosio.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/datastream.hpp>
#include <eosiolib/serialize.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/privileged.hpp>
#include <eosiolib/singleton.hpp>
#include <fio.token/fio.token.hpp>
#include <fio.fee/fio.fee.hpp>
#include <fio.common/fio.common.hpp>
#include <eosiolib/asset.hpp>

#include <algorithm>
#include <cmath>

namespace eosiosystem {
    using eosio::indexed_by;
    using eosio::const_mem_fun;
    using eosio::singleton;
    using eosio::transaction;


   /*******
    * this action will do the required work in the system tables that is necessary
    * before performing the burning of a fio address, the voters and producers
    * table must be cleaned up appropriately for the specified address
    * @param fioaddrhash  this is the hash of the fio address being considered for burning.
    */
    void
    system_contract::burnaction(const uint128_t &fioaddrhash) {
       require_auth(_self);
        //verify that this address is expired.
        //this helps to ensure bad actors cant use this action unintentionally.
        uint64_t nowtime = now();
        auto namesbyname = _fionames.get_index<"byname"_n>();
        auto nameiter = namesbyname.find(fioaddrhash);
        //check(nameiter != namesbyname.end(),"unexpected error verifying expired address");
        uint64_t expire = nameiter->expiration;

        if ((expire + ADDRESSWAITFORBURNDAYS) <= nowtime) {
            auto prodbyaddress = _producers.get_index<"byaddress"_n>();
            auto prod = prodbyaddress.find(fioaddrhash);
            if (prod != prodbyaddress.end()) {
                prodbyaddress.modify(prod, _self, [&](producer_info &info) {
                    info.fio_address = "";
                    info.addresshash = 0;
                    info.is_active = false;
                });
            }
            auto votersbyaddress = _voters.get_index<"byaddress"_n>();
            auto voters = votersbyaddress.find(fioaddrhash);
            if (voters != votersbyaddress.end()) {
                //be absolutely certain to only delete the record we are interested in.
                if (voters->addresshash == fioaddrhash) {
                    votersbyaddress.erase(voters);
                }
            }
        }
    }

    void
    system_contract::incram(const name &accountnm, const int64_t &amount) {
        require_auth(_self);
        bool debug=false;

        int64_t ram;
        int64_t cpu;
        int64_t net;
        get_resource_limits(accountnm.value,&ram,&net,&cpu);
        if (ram > 0 ) {
            if (debug) {
                print(" incremented the RAM for account ", accountnm, " saw pre-existing RAM value of  ", ram,
                      "\n");
                int64_t ramused = get_account_ram_usage(accountnm.value);
                print(" RAM used by account ", accountnm, " is ", ramused, "\n");
            }
            ram += amount;
            set_resource_limits(accountnm.value, ram, net, cpu);
            if(debug) {
                print(" incremented the RAM for account ", accountnm, " new amount is ", ram, "\n");
            }
        }else{
            if(debug) {
                print(" saw unlimited ram use for account ", accountnm, "\n");
            }


        }

    }


    /**
     *  This method will create a producer_config and producer_info object for 'producer'
     *
     *  @pre producer is not already registered
     *  @pre producer to register is an account
     *  @pre authority of producer to register
     *
     */
    void
    system_contract::regiproducer(const name &producer, const string &producer_key, const std::string &url,
                                  const uint16_t &location, const string &fio_address) {
        check(url.size() < 512, "url too long");
        check(producer_key != "", "public key should not be the default value");
        require_auth(producer);

        auto prodbyowner = _producers.get_index<"byowner"_n>();
        auto prod = prodbyowner.find(producer.value);
        uint128_t addresshash = string_to_uint128_hash(fio_address.c_str());
        const auto ct = current_time_point();

        if (prod != prodbyowner.end()) {
            if (prod->is_active) {
                fio_400_assert(false, "fio_address", fio_address,
                               "Already registered as producer", ErrorFioNameNotReg);
            } else {
                prodbyowner.modify(prod, producer, [&](producer_info &info) {
                    info.is_active = true;
                    info.fio_address = fio_address;
                    info.addresshash = addresshash;
                    info.producer_public_key = abieos::string_to_public_key(producer_key);
                    info.url = url;
                    info.location = location;
                    if (info.last_claim_time == time_point())
                        info.last_claim_time = ct;
                });
            }
        } else {
            uint64_t id = _producers.available_primary_key();

            _producers.emplace(producer, [&](producer_info &info) {
                info.id = id;
                info.owner = producer;
                info.fio_address = fio_address;
                info.addresshash = addresshash;
                info.total_votes = 0;
                info.producer_public_key = abieos::string_to_public_key(producer_key);
                info.is_active = true;
                info.url = url;
                info.location = location;
                info.last_claim_time = ct;
            });
        }
        fio_400_assert(transaction_size() <= MAX_TRX_SIZE, "transaction_size", std::to_string(transaction_size()),
          "Transaction is too large", ErrorTransactionTooLarge);

    }

    /******
     * reg producer will register a producer using the specified information
     * @param fio_address  this is the fio address that will be used to claim rewards.
     * @param fio_pub_key  this is the pub key that will be used to generate block, this
     *                      can be for a different account than the fio address.
     * @param url  this is the url of the block producer.
     * @param location this is the location number, sorting these determines BP order in the schedule.
     * @param actor  the account associated with signing.
     * @param max_fee the max fee willing to be paid.
     */
    void
    system_contract::regproducer(
            const string &fio_address,
            const string &fio_pub_key,
            const std::string &url,
            const uint16_t &location,
            const name &actor,
            const int64_t &max_fee) {

        require_auth(actor);

        fio_400_assert(max_fee >= 0, "max_fee", to_string(max_fee), "Invalid fee value",
                       ErrorMaxFeeInvalid);
        fio_400_assert(validateURLFormat(url), "url", url, "Invalid url",
                       ErrorMaxFeeInvalid);
        fio_400_assert(validateLocationFormat(location), "location", to_string(location), "Invalid location",
                       ErrorMaxFeeInvalid);
        fio_400_assert(isPubKeyValid(fio_pub_key),"fio_pub_key", fio_pub_key,
                       "Invalid FIO Public Key",
                       ErrorPubKeyValid);

        FioAddress fa;
        getFioAddressStruct(fio_address, fa);

        uint128_t nameHash = string_to_uint128_hash(fa.fioaddress.c_str());
        uint128_t domainHash = string_to_uint128_hash(fa.fiodomain.c_str());

        auto namesbyname = _fionames.get_index<"byname"_n>();
        auto fioname_iter = namesbyname.find(nameHash);
        fio_400_assert(fioname_iter != namesbyname.end(), "fio_address", fio_address,
                       "FIO Address not registered", ErrorFioNameNotReg);

        //check that the name is not expired
        uint32_t name_expiration = fioname_iter->expiration;
        uint32_t present_time = now();

        uint64_t account = fioname_iter->owner_account;
        fio_403_assert(account == actor.value, ErrorSignature);
        fio_400_assert(present_time <= name_expiration, "fio_address", fio_address,
                       "FIO Address expired", ErrorFioNameExpired);

        auto domainsbyname = _domains.get_index<"byname"_n>();
        auto domains_iter = domainsbyname.find(domainHash);

        fio_400_assert(domains_iter != domainsbyname.end(), "fio_address", fio_address,
                       "FIO Address not registered", ErrorFioNameNotReg);

        uint32_t expiration = domains_iter->expiration;

        //add 30 days to the domain expiration, this call will work until 30 days past expire.
        expiration = get_time_plus_seconds(expiration,SECONDS30DAYS);

        fio_400_assert(present_time <= expiration, "domain", fa.fiodomain, "FIO Domain expired",
                       ErrorDomainExpired);

        regiproducer(actor, fio_pub_key, url, location, fio_address);

        //TODO: REFACTOR FEE ( PROXY / PRODUCER )
        //begin new fees, logic for Mandatory fees.
        uint128_t endpoint_hash = string_to_uint128_hash("register_producer");

        auto fees_by_endpoint = _fiofees.get_index<"byendpoint"_n>();
        auto fee_iter = fees_by_endpoint.find(endpoint_hash);
        //if the fee isnt found for the endpoint, then 400 error.
        fio_400_assert(fee_iter != fees_by_endpoint.end(), "endpoint_name", "register_producer",
                       "FIO fee not found for endpoint", ErrorNoEndpoint);

        uint64_t reg_amount = fee_iter->suf_amount;
        uint64_t fee_type = fee_iter->type;

        //if its not a mandatory fee then this is an error.
        fio_400_assert(fee_type == 0, "fee_type", to_string(fee_type),
                       "register_producer unexpected fee type for endpoint register_producer, expected 0",
                       ErrorNoEndpoint);

        fio_400_assert(max_fee >= (int64_t)reg_amount, "max_fee", to_string(max_fee), "Fee exceeds supplied maximum.",
                       ErrorMaxFeeExceeded);

        fio_fees(actor, asset(reg_amount, FIOSYMBOL));
        processrewardsnotpid(reg_amount, get_self());
        //end new fees, logic for Mandatory fees.

        const string response_string = string("{\"status\": \"OK\",\"fee_collected\":") +
                                 to_string(reg_amount) + string("}");

        if (REGPRODUCERRAM > 0) {
            action(
                    permission_level{SYSTEMACCOUNT, "active"_n},
                    "eosio"_n,
                    "incram"_n,
                    std::make_tuple(actor, REGPRODUCERRAM)
            ).send();
        }

        fio_400_assert(transaction_size() <= MAX_TRX_SIZE, "transaction_size", std::to_string(transaction_size()),
          "Transaction is too large", ErrorTransactionTooLarge);

        send_response(response_string.c_str());
    }

    void system_contract::unregprod(
            const string &fio_address,
            const name &actor,
            const int64_t &max_fee) {
        require_auth(actor);

        fio_400_assert(max_fee >= 0, "max_fee", to_string(max_fee), "Invalid fee value",
                       ErrorMaxFeeInvalid);
        FioAddress fa;
        getFioAddressStruct(fio_address, fa);

        uint128_t nameHash = string_to_uint128_hash(fa.fioaddress.c_str());
        uint128_t domainHash = string_to_uint128_hash(fa.fiodomain.c_str());

        auto namesbyname = _fionames.get_index<"byname"_n>();
        auto fioname_iter = namesbyname.find(nameHash);

        fio_400_assert(fioname_iter != namesbyname.end(), "fio_address", fio_address,
                       "FIO Address not registered", ErrorFioNameNotReg);

        //check that the name is not expired
        uint32_t name_expiration = fioname_iter->expiration;
        uint32_t present_time = now();

        uint64_t account = fioname_iter->owner_account;
        fio_403_assert(account == actor.value, ErrorSignature);
        fio_400_assert(present_time <= name_expiration, "fio_address", fio_address,
                       "FIO Address expired", ErrorFioNameExpired);

        auto domainsbyname = _domains.get_index<"byname"_n>();
        auto domains_iter = domainsbyname.find(domainHash);

        fio_400_assert(domains_iter != domainsbyname.end(), "fio_address", fio_address,
                       "FIO Address not registered", ErrorFioNameNotReg);

        uint32_t expiration = domains_iter->expiration;
        fio_400_assert(present_time <= expiration, "domain", fa.fiodomain, "FIO Domain expired",
                       ErrorDomainExpired);

        auto prodbyowner = _producers.get_index<"byowner"_n>();
        const auto &prod = prodbyowner.find(actor.value);

        fio_400_assert(prod != prodbyowner.end(), "fio_address", fio_address,
                       "Not registered as producer", ErrorFioNameNotReg);

        prodbyowner.modify(prod, same_payer, [&](producer_info &info) {
            info.deactivate();
        });

        //begin new fees, logic for Mandatory fees.
        uint128_t endpoint_hash = string_to_uint128_hash("unregister_producer");

        auto fees_by_endpoint = _fiofees.get_index<"byendpoint"_n>();
        auto fee_iter = fees_by_endpoint.find(endpoint_hash);
        //if the fee isnt found for the endpoint, then 400 error.
        fio_400_assert(fee_iter != fees_by_endpoint.end(), "endpoint_name", "unregister_producer",
                       "FIO fee not found for endpoint", ErrorNoEndpoint);

        uint64_t reg_amount = fee_iter->suf_amount;
        uint64_t fee_type = fee_iter->type;

        //if its not a mandatory fee then this is an error.
        fio_400_assert(fee_type == 0, "fee_type", to_string(fee_type),
                       "register_producer unexpected fee type for endpoint register_producer, expected 0",
                       ErrorNoEndpoint);

        fio_400_assert(max_fee >= (int64_t)reg_amount, "max_fee", to_string(max_fee), "Fee exceeds supplied maximum.",
                       ErrorMaxFeeExceeded);


        fio_fees(actor, asset(reg_amount, FIOSYMBOL));
        processrewardsnotpid(reg_amount, get_self());

        //end new fees, logic for Mandatory fees.

        const string response_string = string("{\"status\": \"OK\",\"fee_collected\":") +
                                 to_string(reg_amount) + string("}");


       fio_400_assert(transaction_size() <= MAX_TRX_SIZE, "transaction_size", std::to_string(transaction_size()),
         "Transaction is too large", ErrorTransactionTooLarge);
        send_response(response_string.c_str());
    }

    void system_contract::update_elected_producers(const block_timestamp& block_time) {

      _gstate.last_producer_schedule_update = block_time;

      bool debug=false;

      auto idx = _producers.get_index<"prototalvote"_n>();

      using value_type = std::pair<eosio::producer_key, uint16_t>;
      std::vector< value_type > top_producers;
      top_producers.reserve(MAXACTIVEBPS);

      vector<name> prevprods;

      //clear _topprods table
      auto iter = _topprods.begin();
      while (iter != _topprods.end()) {
          prevprods.push_back(iter->producer);
          iter = _topprods.erase(iter);

      }

      for( auto it = idx.cbegin(); it != idx.cend() && top_producers.size() < MAXACTIVEBPS && 0 < it->total_votes && it->active(); ++it ) {
         top_producers.emplace_back(
                        std::pair<eosio::producer_key, uint16_t>({{it->owner, it->producer_public_key}, it->location}));

         _topprods.emplace(get_self(), [&](auto &p) {
            p.producer = it->owner;
          });

          std::vector<name>::iterator pos = std::find(prevprods.begin(), prevprods.end(), it->owner);
          if (pos != prevprods.end()) {
              //it was in the list before, do not ajust the resource limits, remove from list
              prevprods.erase(pos);
          }
          else {
              if(debug) {
                  print("setting producer to unlimited resources for account ", it->owner, "\n");
              }
              //it was not in the list before, set it unlimited
              set_resource_limits(it->owner.value, -1,-1,-1);
          }
      }

        //this is the producers exiting the schedule, reset their resoource limits.
        for(int i=0; i < prevprods.size(); i++){
            //get the ram that this account has used.
            int64_t ram = get_account_ram_usage(prevprods[i].value);
            if(debug) {
                print(" de-schedule producer, found ram usage ", ram, " for account ", prevprods[i], "\n");
            }
            //increment the ram by the set amount.
            ram += ADDITIONALRAMBPDESCHEDULING;
            //set the new limits going forward.
            set_resource_limits(prevprods[i].value, ram, -1, -1);
            if(debug) {
                print(" de-schedule producer, setting RAM limit ", ram, " for account ", prevprods[i],
                      "\n");
            }
        }

      if( top_producers.size() == 0 || top_producers.size() < _gstate.last_producer_schedule_size ) {
         return;
      }

      /// sort by producer location, location initialized to zero in fio.system.hpp
      /// location will be set upon call to register producer by the block producer.
      /// the location should be considered as a scheduled order of producers, they should
      /// set the location so that traversing locations gives most proximal producer locations
      /// to help address latency.
      std::sort( top_producers.begin(), top_producers.end(), []( const value_type& lhs, const value_type& rhs ) {
         return lhs.first.producer_name < rhs.first.producer_name; // sort by producer name
         // return lhs.second < rhs.second; // sort by location
      } );

      std::vector<eosio::producer_key> producers;

      producers.reserve(top_producers.size());
      for( auto& item : top_producers )
         producers.push_back( std::move(item.first) );

      auto packed_schedule = pack(producers);
      if( set_proposed_producers(packed_schedule.data(), packed_schedule.size() ) >= 0 ) {
         _gstate.last_producer_schedule_size = static_cast<decltype(_gstate.last_producer_schedule_size)>( top_producers.size() );
      }
    }

    double system_contract::update_total_votepay_share(time_point ct,
                                                       double additional_shares_delta,
                                                       double shares_rate_delta) {
        double delta_total_votepay_share = 0.0;
        if (ct > _gstate3.last_vpay_state_update) {
            delta_total_votepay_share = _gstate3.total_vpay_share_change_rate
                                        * double((ct - _gstate3.last_vpay_state_update).count() / 1E6);
        }

        delta_total_votepay_share += additional_shares_delta;
        if (delta_total_votepay_share < 0 && _gstate2.total_producer_votepay_share < -delta_total_votepay_share) {
            _gstate2.total_producer_votepay_share = 0.0;
        } else {
            _gstate2.total_producer_votepay_share += delta_total_votepay_share;
        }

        if (shares_rate_delta < 0 && _gstate3.total_vpay_share_change_rate < -shares_rate_delta) {
            _gstate3.total_vpay_share_change_rate = 0.0;
        } else {
            _gstate3.total_vpay_share_change_rate += shares_rate_delta;
        }

        _gstate3.last_vpay_state_update = ct;

        return _gstate2.total_producer_votepay_share;
    }


    /**
     *  @pre producers must be sorted from lowest to highest and must be registered and active
     *  @pre if proxy is set then no producers can be voted for
     *  @pre if proxy is set then proxy account must exist and be registered as a proxy
     *  @pre every listed producer or proxy must have been previously registered
     *  @pre voter must authorize this action
     *  @pre voter must have previously staked some amount of CORE_SYMBOL for voting
     *  @pre voter->staked must be up to date
     *
     *  @post every producer previously voted for will have vote reduced by previous vote weight
     *  @post every producer newly voted for will have vote increased by new vote amount
     *  @post prior proxy will proxied_vote_weight decremented by previous vote weight
     *  @post new proxy will proxied_vote_weight incremented by new vote weight
     *
     *  If voting for a proxy, the producer votes will not change until the proxy updates their own vote.
     */

    /*** remove vproducer
    void system_contract::vproducer(const name &voter_name, const name &proxy, const std::vector <name> &producers) {
        require_auth(voter_name);
        update_votes(voter_name, proxy, producers, true);
    }
     ****/

    void system_contract::voteproducer(const std::vector<string> &producers, const string &fio_address, const name &actor, const int64_t &max_fee) {
        require_auth(actor);

        fio_400_assert(max_fee >= 0, "max_fee", to_string(max_fee), "Invalid fee value",
                       ErrorMaxFeeInvalid);
        name proxy;
        std::vector<name> producers_accounts;

        FioAddress fa;
        getFioAddressStruct(fio_address, fa);
        fio_400_assert(validateFioNameFormat(fa), "fio_address", fio_address, "Invalid FIO Address format",
                       ErrorDomainAlreadyRegistered);

        uint128_t voterHash = string_to_uint128_hash(fio_address.c_str());
        uint128_t voterDomainHash = string_to_uint128_hash(fa.fiodomain.c_str());

        // compare fio_address owner and compare to actor
        auto namesbyname = _fionames.get_index<"byname"_n>();
        auto voter_iter = namesbyname.find(voterHash);

        fio_400_assert(voter_iter != namesbyname.end(), "fio_address", fio_address,
                       "FIO address not registered", ErrorFioNameNotRegistered);

        uint32_t voter_expiration = voter_iter->expiration;
        uint32_t present_time = now();

        fio_400_assert(present_time <= voter_expiration, "fio_address", fio_address, "FIO Address expired",
                       ErrorDomainExpired);

        auto domainsbyname = _domains.get_index<"byname"_n>();
        auto voterdomain_iter = domainsbyname.find(voterDomainHash);

        fio_400_assert(voterdomain_iter != domainsbyname.end(), "fio_address", fio_address,
                       "FIO Address not registered", ErrorFioNameNotReg);
        fio_403_assert(voter_iter->owner_account == actor.value, ErrorSignature);

        uint32_t voterdomain_expiration = voterdomain_iter->expiration;
        fio_400_assert(present_time <= voterdomain_expiration, "fio_address", fio_address, "FIO Domain expired",
                       ErrorDomainExpired);

        for (size_t i = 0; i < producers.size(); i++) {
            getFioAddressStruct(producers[i], fa);

            uint128_t nameHash = string_to_uint128_hash(fa.fioaddress.c_str());
            uint128_t domainHash = string_to_uint128_hash(fa.fiodomain.c_str());

            auto fioname_iter = namesbyname.find(nameHash);
            fio_400_assert(fioname_iter != namesbyname.end(), "fio_address", fio_address,
                           "FIO Address not registered", ErrorFioNameNotReg);

            uint32_t name_expiration = fioname_iter->expiration;

            uint64_t account = fioname_iter->owner_account;
            fio_400_assert(present_time <= name_expiration, "fio_address", producers[i],
                           "FIO Address expired", ErrorFioNameExpired);

            auto domains_iter = domainsbyname.find(domainHash);

            fio_400_assert(domains_iter != domainsbyname.end(), "fio_address", fio_address,
                           "FIO Address not registered", ErrorFioNameNotReg);

            uint32_t expiration = domains_iter->expiration;
            fio_400_assert(present_time <= expiration, "domain", fa.fiodomain, "FIO Domain expired",
                           ErrorDomainExpired);

            auto proxy_name = name{account};
            producers_accounts.push_back(proxy_name);
        }

        auto votersbyowner = _voters.get_index<"byowner"_n>();
        auto voter_prod_iter = votersbyowner.find(actor.value);

        if (voter_prod_iter == votersbyowner.end()) {
            uint64_t id = _voters.available_primary_key();
            _voters.emplace(actor, [&](auto &p) {
                p.id = id;
                p.owner = actor;
            });
        }

        eosio::token::computeremaininglockedtokens(actor,true);
        sort(producers_accounts.begin(),producers_accounts.end());

        update_votes(actor, proxy, producers_accounts, true);

        uint128_t endpoint_hash = string_to_uint128_hash("vote_producer");
        auto fees_by_endpoint = _fiofees.get_index<"byendpoint"_n>();
        auto fee_iter = fees_by_endpoint.find(endpoint_hash);

        uint64_t bundleeligiblecountdown = voter_iter->bundleeligiblecountdown;
        uint64_t fee_amount = 0;

        if (bundleeligiblecountdown > 0) {
            action{
                    permission_level{_self, "active"_n},
                    AddressContract,
                    "decrcounter"_n,
                    make_tuple(fio_address, 1)
            }.send();
        } else {
            fee_amount = fee_iter->suf_amount;
            fio_400_assert(max_fee >= (int64_t) fee_amount, "max_fee", to_string(max_fee),
                           "Fee exceeds supplied maximum.",
                           ErrorMaxFeeExceeded);

            fio_fees(actor, asset(fee_amount, FIOSYMBOL));
            processrewardsnotpid(fee_amount, get_self());
            //end new fees, logic for Mandatory fees.
        }

        const string response_string = string("{\"status\": \"OK\",\"fee_collected\":") +
                                 to_string(fee_amount) + string("}");

        if (VOTEPRODUCERRAM > 0) {
            action(
                    permission_level{SYSTEMACCOUNT, "active"_n},
                    "eosio"_n,
                    "incram"_n,
                    std::make_tuple(actor, VOTEPRODUCERRAM)
            ).send();
        }

        fio_400_assert(transaction_size() <= MAX_TRX_SIZE, "transaction_size", std::to_string(transaction_size()),
          "Transaction is too large", ErrorTransactionTooLarge);

        send_response(response_string.c_str());
    }

    void system_contract::voteproxy(const string &proxy, const string &fio_address, const name &actor, const int64_t &max_fee) {
        require_auth(actor);

        fio_400_assert(max_fee >= 0, "max_fee", to_string(max_fee), "Invalid fee value",
                       ErrorMaxFeeInvalid);
        FioAddress fa, va;
        getFioAddressStruct(proxy, fa);
        getFioAddressStruct(fio_address, va);

        fio_400_assert(validateFioNameFormat(va), "proxy", proxy, "Invalid FIO Address format",
                       ErrorDomainAlreadyRegistered);
        fio_400_assert(validateFioNameFormat(fa), "fio_address", fio_address, "Invalid FIO Address format",
                       ErrorDomainAlreadyRegistered);

        uint128_t voterHash = string_to_uint128_hash(fio_address.c_str());
        uint128_t voterDomainHash = string_to_uint128_hash(va.fiodomain.c_str());

        // compare fio_address owner and compare to actor
        auto namesbyname = _fionames.get_index<"byname"_n>();
        auto voter_iter = namesbyname.find(voterHash);

        fio_400_assert(voter_iter != namesbyname.end(), "fio_address", fio_address,
                       "FIO address not registered", ErrorFioNameNotRegistered);

        uint32_t voter_expiration = voter_iter->expiration;
        uint32_t present_time = now();

        fio_400_assert(present_time <= voter_expiration, "fio_address", fio_address, "FIO Address expired",
                       ErrorDomainExpired);

        auto domainsbyname = _domains.get_index<"byname"_n>();
        auto voterdomain_iter = domainsbyname.find(voterDomainHash);
        fio_400_assert(voterdomain_iter != domainsbyname.end(), "fio_address", fio_address,
                       "FIO Address not registered", ErrorFioNameNotReg);
        fio_403_assert(voter_iter->owner_account == actor.value, ErrorSignature);

        uint32_t voterdomain_expiration = voterdomain_iter->expiration;
        fio_400_assert(present_time <= voterdomain_expiration, "fio_address", fio_address, "FIO Domain expired",
                       ErrorDomainExpired);

        uint128_t nameHash = string_to_uint128_hash(fa.fioaddress.c_str());
        uint128_t domainHash = string_to_uint128_hash(fa.fiodomain.c_str());
        auto fioname_iter = namesbyname.find(nameHash);
        fio_400_assert(fioname_iter != namesbyname.end(), "proxy", proxy,
                       "FIO Address not registered", ErrorFioNameNotReg);

        //check that the name is not expired
        uint32_t name_expiration = fioname_iter->expiration;
        uint64_t account = fioname_iter->owner_account;
        fio_400_assert(present_time <= name_expiration, "proxy", proxy,
                       "FIO Address expired", ErrorFioNameExpired);

        auto domains_iter = domainsbyname.find(domainHash);

        fio_400_assert(domains_iter != domainsbyname.end(), "proxy", proxy,
                       "FIO Address not registered", ErrorFioNameNotReg);

        uint32_t expiration = domains_iter->expiration;

        //add 30 days to the domain expiration, this call will work until 30 days past expire.
        expiration = get_time_plus_seconds(expiration,SECONDS30DAYS);

        fio_400_assert(present_time <= expiration, "proxy", proxy, "FIO Domain expired",
                       ErrorDomainExpired);

        auto proxy_name = name{account};
        auto votersbyowner = _voters.get_index<"byowner"_n>();
        auto voter_proxy_iter = votersbyowner.find(account);

        //the first opportunity to throw this error is when the owner account is not present
        //in the table.
        fio_400_assert(voter_proxy_iter != votersbyowner.end(), "fio_address", proxy,
                       "This address is not a proxy", AddressNotProxy);

        //the second opportunity to throw this error is when the row is present and is not a proxy
        fio_400_assert(voter_proxy_iter->is_proxy, "fio_address", fio_address,
                       "This address is not a proxy", AddressNotProxy);


        std::vector<name> producers{}; // Empty


        voter_proxy_iter = votersbyowner.find(actor.value);

        if (voter_proxy_iter == votersbyowner.end()) {
            uint64_t id = _voters.available_primary_key();
            _voters.emplace(actor, [&](auto &p) {
                p.id = id;
                p.owner = actor;
            });
        }

        eosio::token::computeremaininglockedtokens(actor,true);

        update_votes(actor, proxy_name, producers, true);

        uint128_t endpoint_hash = string_to_uint128_hash("vote_producer");
        auto fees_by_endpoint = _fiofees.get_index<"byendpoint"_n>();
        auto fee_iter = fees_by_endpoint.find(endpoint_hash);

        uint64_t bundleeligiblecountdown = voter_iter->bundleeligiblecountdown;
        uint64_t fee_amount = 0;

        if (bundleeligiblecountdown > 0) {
            action{
                    permission_level{_self, "active"_n},
                    AddressContract,
                    "decrcounter"_n,
                      make_tuple(fio_address, 1)
            }.send();
        } else {
            fee_amount = fee_iter->suf_amount;
            fio_400_assert(max_fee >= (int64_t) fee_amount, "max_fee", to_string(max_fee),
                           "Fee exceeds supplied maximum.",
                           ErrorMaxFeeExceeded);

            fio_fees(actor, asset(fee_amount, FIOSYMBOL));
            processrewardsnotpid(fee_amount, get_self());
            //end new fees, logic for Mandatory fees.
        }

        const string response_string = string("{\"status\": \"OK\",\"fee_collected\":") +
                                 to_string(fee_amount) + string("}");

        if (VOTEPROXYRAM > 0) {
            action(
                    permission_level{SYSTEMACCOUNT, "active"_n},
                    "eosio"_n,
                    "incram"_n,
                    std::make_tuple(actor, VOTEPROXYRAM)
            ).send();
        }

        fio_400_assert(transaction_size() <= MAX_TRX_SIZE, "transaction_size", std::to_string(transaction_size()),
          "Transaction is too large", ErrorTransactionTooLarge);

        send_response(response_string.c_str());
    }


    //this sets the inhibit_unlocking flag in the lockedtokens table.
    // set to 0 is false, non zero is true.
    void system_contract::inhibitunlck(const name &owner, const uint32_t &value){
        require_auth(FOUNDATIONACCOUNT);

        //get the owner, check that the specified grant is type 2, do the
        //inhibit, do not do the inhibit otherwise.
        auto lockiter = _lockedtokens.find(owner.value);
        if(lockiter != _lockedtokens.end()){
            if(lockiter->grant_type == 2) {
                //update the locked table.
                _lockedtokens.modify(lockiter, _self, [&](auto &av) {
                    av.inhibit_unlocking = value;
                });
            }
        }
    }


    void system_contract::unlocktokens(const name &actor){
        require_auth(TokenContract);
        eosio::token::computeremaininglockedtokens(actor,true);
    }

    uint64_t system_contract::get_votable_balance(const name &tokenowner){

        //get fio balance for this account,
        uint32_t present_time = now();
        const auto my_balance = eosio::token::get_balance("fio.token"_n,tokenowner, FIOSYMBOL.code() );
        uint64_t amount = my_balance.amount;

        //see if the user is in the lockedtokens table, if so recompute the balance
        //based on grant type.
        auto lockiter = _lockedtokens.find(tokenowner.value);
        if(lockiter != _lockedtokens.end()){
            check(amount >= lockiter->remaining_locked_amount,"votable balance lock amount is incoherent.");
            //if lock type 1 always subtract remaining locked amount from balance
            if (lockiter->grant_type == 1) {
                double percent = 1.0 - (double)((double)lockiter->remaining_locked_amount / (double)lockiter->total_grant_amount);
                if (percent <= .3){
                    double onethirdgrant = (double)lockiter->total_grant_amount * .3;
                    //amount is lesser of account amount and 1/3 of the total grant
                    double damount = (double)amount;
                    if (onethirdgrant <= damount){
                        amount = onethirdgrant;
                    }else{
                        amount = damount;
                    }

                }else{
                    //amount is all the available tokens in the account.
                    return amount;
                }
            }
           //TEST TEST TEST LOCKED TOKENS
           //TEST TEST TEST LOCKED TOKENS  uint32_t issueplus210 = lockiter->timestamp+(25*60);
           //TEST TEST TEST LOCKED TOKENS
           uint32_t issueplus210 = lockiter->timestamp+(210*SECONDSPERDAY);

            //if lock type 2 only subtract remaining locked amount if 210 days since launch, and inhibit locking true.
           if (((lockiter->grant_type == 2)&&((present_time > issueplus210)&&lockiter->inhibit_unlocking)) ||
                   (lockiter->grant_type == 4)
            ){
                //subtract the lock amount from the balance
                if (lockiter->remaining_locked_amount < amount) {
                    amount -= lockiter->remaining_locked_amount;
                } else {
                    amount = 0;
                }
            }
        }
        return amount;
    }

    void system_contract::update_votes(
            const name &voter_name,
            const name &proxy,
            const std::vector <name> &producers,
            const bool &voting) {
        //validate input
        if (proxy) {
            check(producers.size() == 0, "cannot vote for producers and proxy at same time");
            check(voter_name != proxy, "Invalid or duplicated producers0");
        } else {
            check(producers.size() <= 30, "attempt to vote for too many producers");
            for (size_t i = 1; i < producers.size(); ++i) {
                check(producers[i - 1] < producers[i], "producer votes must be unique and sorted");
            }
        }
        auto votersbyowner = _voters.get_index<"byowner"_n>();
        auto voter = votersbyowner.find(voter_name.value);
        check(voter != votersbyowner.end(), "user must vote before votes can be updated");
        check(!proxy || !voter->is_proxy, "account registered as a proxy is not allowed to use a proxy");

        //change to get_unlocked_balance() Ed 11/25/2019
        uint64_t amount = get_votable_balance(voter->owner);

        //on the first vote we update the total voted fio.


        auto new_vote_weight = (double)amount;
        if (voter->is_proxy) {
            new_vote_weight += voter->proxied_vote_weight;
        }

        if( !(proxy) ) {

            if( voter->last_vote_weight > 0.0 ) {
                _gstate.total_voted_fio -= voter->last_vote_weight;
            }

            _gstate.total_voted_fio += new_vote_weight;

            if( _gstate.total_voted_fio >= MINVOTEDFIO && _gstate.thresh_voted_fio_time == time_point() ) {
                _gstate.thresh_voted_fio_time = current_time_point();
            }
        }


        boost::container::flat_map <name, pair<double, bool /*new*/>> producer_deltas;
        if (voter->last_vote_weight > 0) {
            if (voter->proxy) {
                auto old_proxy = votersbyowner.find(voter->proxy.value);
                check(old_proxy != votersbyowner.end(), "old proxy not found"); //data corruption
                votersbyowner.modify(old_proxy, same_payer, [&](auto &vp) {
                    vp.proxied_vote_weight -= voter->last_vote_weight;
                });
                propagate_weight_change(*old_proxy);
            } else {
                for (const auto &p : voter->producers) {
                    auto &d = producer_deltas[p.producer];
                    d.first -= voter->last_vote_weight;
                    d.second = false;
                }
            }
        }

        if (proxy) {
            auto new_proxy = votersbyowner.find(proxy.value);
            check(new_proxy != votersbyowner.end(),
                  "invalid proxy specified"); //if ( !voting ) { data corruption } else { wrong vote }
            fio_403_assert(!voting || new_proxy->is_proxy, ErrorProxyNotFound);
            if (new_vote_weight >= 0) {
                votersbyowner.modify(new_proxy, same_payer, [&](auto &vp) {
                    vp.proxied_vote_weight += new_vote_weight;
                });
                propagate_weight_change(*new_proxy);
            }
        } else {
            if (new_vote_weight >= 0) {
                for (const auto &p : producers) {
                    auto &d = producer_deltas[p];
                    d.first += new_vote_weight;
                    d.second = true;
                }
            }
        }

        const auto ct = current_time_point();
        double delta_change_rate = 0.0;
        double total_inactive_vpay_share = 0.0;
        auto prodbyowner = _producers.get_index<"byowner"_n>();
        for (const auto &pd : producer_deltas) {
            auto pitr = prodbyowner.find(pd.first.value);
            if (pitr != prodbyowner.end()) {
                check(!voting || pitr->active() || !pd.second.second /* not from new set */,
                      "Invalid or duplicated producers1");
                double init_total_votes = pitr->total_votes;
                prodbyowner.modify(pitr, same_payer, [&](auto &p) {
                    p.total_votes += pd.second.first;
                    if (p.total_votes < 0) { // floating point arithmetics can give small negative numbers
                        p.total_votes = 0;
                    }
                    _gstate.total_producer_vote_weight += pd.second.first;
                    //check( p.total_votes >= 0, "something bad happened" );
                });
            } else {
                check(!pd.second.second , "Invalid or duplicated producers2"); //data corruption
            }

        }

        update_total_votepay_share(ct, -total_inactive_vpay_share, delta_change_rate);


        vector<producername> producers_addresses;

          auto namesbyname = _fionames.get_index<"byowner"_n>();
        for (const auto &p : producers) {
          auto res = namesbyname.find(p.value);
          producers_addresses.push_back(producername{p, res->name});
        }

        votersbyowner.modify(voter, same_payer, [&](auto &av) {
            av.last_vote_weight = new_vote_weight;
            av.producers = producers_addresses;
            av.proxy = proxy;
        });
    }

    void system_contract::updlocked(const name &owner,const uint64_t &amountremaining)
    {
        require_auth(TokenContract);

        auto iterlocked = _lockedtokens.find(owner.value);
        check(iterlocked != _lockedtokens.end(), "locked funds account not found.");
        check(iterlocked->remaining_locked_amount >= amountremaining,"locked funds remaining amount cannot increase.");

        _lockedtokens.modify(iterlocked, _self, [&](auto &av) {
            av.remaining_locked_amount = amountremaining;
        });
    }

    void system_contract::setautoproxy(const name &proxy,const name &owner)
    {
        require_auth(TPIDContract);


        fio_400_assert(!(isFIOSystem(owner)), "owner", "setautoproxy",
                       "Auto proxy cannot be to a system account", ErrorActorIsSystemAccount);
        fio_400_assert(!(isFIOSystem(proxy)), "proxy", "setautoproxy",
                "proxy cannot be a from system account", ErrorActorIsSystemAccount);
        //first verify that the proxy exists and is registered as a proxy.
        //look it up and check it.
        //if its there then emplace the owner record into the voting_info table with is_auto_proxy set.
        auto votersbyowner = _voters.get_index<"byowner"_n>();
        auto itervi = votersbyowner.find(proxy.value);
        check(itervi != votersbyowner.end(), "specified proxy not found.");
        check(itervi->is_proxy == true,"specified proxy is not registered as a proxy");

        itervi = votersbyowner.find(owner.value);
        check(itervi != votersbyowner.end(), "specified owner not found.");
        votersbyowner.modify(itervi, same_payer, [&](auto &av) {
            av.is_auto_proxy = true;
            av.proxy = proxy;
        });
    }

    void system_contract::crautoproxy(const name &proxy,const name &owner)
    {
        bool debug = false;
        require_auth(TPIDContract);
        if (debug) {
            print("calling create auto proxy ", proxy, " owner ", owner, "\n");
        }

        fio_400_assert(!(isFIOSystem(owner)), "owner", "setautoproxy",
                "Auto proxy cannot be to a system account", ErrorActorIsSystemAccount);
        fio_400_assert(!(isFIOSystem(proxy)), "proxy", "setautoproxy",
                "proxy cannot be a from system account", ErrorActorIsSystemAccount);

        //first verify that the proxy exists and is registered as a proxy.
        //look it up and check it.
        //if its there then emplace the owner record into the voting_info table with is_auto_proxy set.
        auto votersbyowner = _voters.get_index<"byowner"_n>();
        auto itervi = votersbyowner.find(proxy.value);

        if (itervi != votersbyowner.end() &&
           itervi->is_proxy) {
            if (debug) {
                print("create auto proxy found the proxy ", proxy, "\n");
            }

            auto itervoter = votersbyowner.find(owner.value);
            if (itervoter == votersbyowner.end()) {
                uint64_t id = _voters.available_primary_key();
                _voters.emplace(owner, [&](auto &p) {
                    p.id = id;
                    p.owner = owner;
                    p.is_auto_proxy = true;
                    p.proxy = proxy;
                });
            } else if (itervoter->is_auto_proxy && itervoter->proxy != proxy) {
                votersbyowner.modify(itervoter, _self, [&](struct voter_info &a) {
                    a.proxy = proxy;
                });
            }
        }
        else{
            if (debug) {
                print("create auto proxy didnt find the proxy ", proxy, "\n");
            }
        }
    }

    void system_contract::unregproxy(const std::string &fio_address, const name &actor, const int64_t &max_fee) {
       require_auth(actor);

        fio_400_assert(max_fee >= 0, "max_fee", to_string(max_fee), "Invalid fee value",
                       ErrorMaxFeeInvalid);
        FioAddress fa;
        getFioAddressStruct(fio_address, fa);


        uint128_t nameHash = string_to_uint128_hash(fa.fioaddress.c_str());
        uint128_t domainHash = string_to_uint128_hash(fa.fiodomain.c_str());

        auto namesbyname = _fionames.get_index<"byname"_n>();
        auto fioname_iter = namesbyname.find(nameHash);

        fio_400_assert(fioname_iter != namesbyname.end(), "fio_address", fio_address,
                       "FIO Address not registered", ErrorFioNameNotReg);

        //check that the name is not expired
        uint32_t name_expiration = fioname_iter->expiration;
        uint32_t present_time = now();

        uint64_t account = fioname_iter->owner_account;
        fio_403_assert(account == actor.value, ErrorSignature);
        fio_400_assert(present_time <= name_expiration, "fio_address", fio_address,
                       "FIO Address expired", ErrorFioNameExpired);

        auto domainsbyname = _domains.get_index<"byname"_n>();
        auto domains_iter = domainsbyname.find(domainHash);

        fio_400_assert(domains_iter != domainsbyname.end(), "fio_address", fio_address,
                       "FIO Address not registered", ErrorFioNameNotReg);

        uint32_t expiration = domains_iter->expiration;
        fio_400_assert(present_time <= expiration, "domain", fa.fiodomain, "FIO Domain expired",
                       ErrorDomainExpired);

        regiproxy(actor,fio_address,false);

        //begin new fees, logic for Mandatory fees.
        uint128_t endpoint_hash = string_to_uint128_hash("unregister_proxy");

        auto fees_by_endpoint = _fiofees.get_index<"byendpoint"_n>();
        auto fee_iter = fees_by_endpoint.find(endpoint_hash);
        //if the fee isnt found for the endpoint, then 400 error.
        fio_400_assert(fee_iter != fees_by_endpoint.end(), "endpoint_name", "unregister_proxy",
                       "FIO fee not found for endpoint", ErrorNoEndpoint);

        uint64_t reg_amount = fee_iter->suf_amount;
        uint64_t fee_type = fee_iter->type;

        //if its not a mandatory fee then this is an error.
        fio_400_assert(fee_type == 0, "fee_type", to_string(fee_type),
                       "unregister_proxy unexpected fee type for endpoint unregister_proxy, expected 0",
                       ErrorNoEndpoint);

        fio_400_assert(max_fee >= (int64_t)reg_amount, "max_fee", to_string(max_fee), "Fee exceeds supplied maximum.",
                       ErrorMaxFeeExceeded);

        asset reg_fee_asset;
        reg_fee_asset.symbol = symbol("FIO",9);
        reg_fee_asset.amount = reg_amount;

        fio_fees(actor, reg_fee_asset);
        processrewardsnotpid(reg_amount, get_self());
        //end new fees, logic for Mandatory fees.

        const string response_string = string("{\"status\": \"OK\",\"fee_collected\":") +
                                 to_string(reg_amount) + string("}");
        fio_400_assert(transaction_size() <= MAX_TRX_SIZE, "transaction_size", std::to_string(transaction_size()),
         "Transaction is too large", ErrorTransactionTooLarge);
        send_response(response_string.c_str());
    }


    void system_contract::regproxy(const std::string &fio_address, const name &actor, const int64_t &max_fee) {
       require_auth(actor);

        fio_400_assert(max_fee >= 0, "max_fee", to_string(max_fee), "Invalid fee value",
                       ErrorMaxFeeInvalid);

        FioAddress fa;
        getFioAddressStruct(fio_address, fa);

        uint128_t nameHash = string_to_uint128_hash(fa.fioaddress.c_str());
        uint128_t domainHash = string_to_uint128_hash(fa.fiodomain.c_str());

        auto namesbyname = _fionames.get_index<"byname"_n>();
        auto fioname_iter = namesbyname.find(nameHash);
        fio_400_assert(fioname_iter != namesbyname.end(), "fio_address", fio_address,
                       "FIO Address not registered", ErrorFioNameNotReg);

        //check that the name is not expired
        uint32_t name_expiration = fioname_iter->expiration;
        uint32_t present_time = now();

        uint64_t account = fioname_iter->owner_account;
        fio_403_assert(account == actor.value, ErrorSignature);
        fio_400_assert(present_time <= name_expiration, "fio_address", fio_address,
                       "FIO Address expired", ErrorFioNameExpired);

        auto domainsbyname = _domains.get_index<"byname"_n>();
        auto domains_iter = domainsbyname.find(domainHash);

        fio_400_assert(domains_iter != domainsbyname.end(), "fio_address", fio_address,
                       "FIO Address not registered", ErrorFioNameNotReg);

        uint32_t expiration = domains_iter->expiration;

        //add 30 days to the domain expiration, this call will work until 30 days past expire.
        expiration = get_time_plus_seconds(expiration,SECONDS30DAYS);

        fio_400_assert(present_time <= expiration, "domain", fa.fiodomain, "FIO Domain expired",
                       ErrorDomainExpired);

        regiproxy(actor,fio_address,true);

        //begin new fees, logic for Mandatory fees.
        uint128_t endpoint_hash = string_to_uint128_hash("register_proxy");

        auto fees_by_endpoint = _fiofees.get_index<"byendpoint"_n>();
        auto fee_iter = fees_by_endpoint.find(endpoint_hash);
        //if the fee isnt found for the endpoint, then 400 error.
        fio_400_assert(fee_iter != fees_by_endpoint.end(), "endpoint_name", "register_proxy",
                       "FIO fee not found for endpoint", ErrorNoEndpoint);

        uint64_t reg_amount = fee_iter->suf_amount;
        uint64_t fee_type = fee_iter->type;

        //if its not a mandatory fee then this is an error.
        fio_400_assert(fee_type == 0, "fee_type", to_string(fee_type),
                       "register_proxy unexpected fee type for endpoint register_proxy, expected 0",
                       ErrorNoEndpoint);

        fio_400_assert(max_fee >= (int64_t)reg_amount, "max_fee", to_string(max_fee), "Fee exceeds supplied maximum.",
                       ErrorMaxFeeExceeded);

        asset reg_fee_asset;
        reg_fee_asset.symbol = symbol("FIO",9);
        reg_fee_asset.amount = reg_amount;

        fio_fees(actor, reg_fee_asset);
        processrewardsnotpid(reg_amount, get_self());
        //end new fees, logic for Mandatory fees.

        INLINE_ACTION_SENDER(eosiosystem::system_contract, updatepower)
                ("eosio"_n, {{_self, "active"_n}},
                 {actor, false}
                );

        const string response_string = string("{\"status\": \"OK\",\"fee_collected\":") +
                                 to_string(reg_amount) + string("}");
        if (REGPROXYRAM > 0) {
            action(
                    permission_level{SYSTEMACCOUNT, "active"_n},
                    "eosio"_n,
                    "incram"_n,
                    std::make_tuple(actor, REGPROXYRAM)
            ).send();
        }

        fio_400_assert(transaction_size() <= MAX_TRX_SIZE, "transaction_size", std::to_string(transaction_size()),
          "Transaction is too large", ErrorTransactionTooLarge);

        send_response(response_string.c_str());
    }


    /**
     * this action will allow a caller to register a proxy for use in voting going forward.
     * or it will allow a caller to register as a proxy for use by others going forward.
     * to register the use of a proxy call this with the desired proxy and false
     * to register as a proxy going forward call this with the desired proxy and true.
     *
     *  @param isproxy - true if proxy wishes to vote on behalf of others, false otherwise
     */
    void system_contract::regiproxy(const name &proxy, const string &fio_address, const bool &isproxy) {

       require_auth(proxy);

        auto votersbyowner = _voters.get_index<"byowner"_n>();
        auto pitr = votersbyowner.find(proxy.value);
        uint128_t addresshash = string_to_uint128_hash(fio_address.c_str());
        if (pitr != votersbyowner.end()) {

            //if the values are equal and isproxy, then show this error.
            fio_400_assert((isproxy != pitr->is_proxy)|| !isproxy, "fio_address", fio_address,
                           "Already registered as proxy. ", ErrorPubAddressExist);
            //check(!isproxy || !pitr->proxy, "account that uses a proxy is not allowed to become a proxy");
            if (isproxy && !pitr->proxy) {
                votersbyowner.modify(pitr, same_payer, [&](auto &p) {
                    p.is_proxy = isproxy;
                    p.is_auto_proxy = false;
                });
            }else if (!isproxy) { //this is how we undo/clear a proxy
                name nm;
                votersbyowner.modify(pitr, same_payer, [&](auto &p) {
                    p.is_proxy = isproxy;
                    p.is_auto_proxy = false;
                    p.proxy = nm; //set to a null state, an uninitialized name,
                                  //we need to be sure this returns true on (!proxy) so other logic
                                  //areas work correctly.
                });
            }
            propagate_weight_change(*pitr);
        } else if (isproxy){  //only do the emplace if isproxy is true,
                              //it makes no sense to emplace a voter record when isproxy is false,
                              // this means making a voting record with no votes, and not a proxy,
                              //and not having a proxy, its kind of a null vote, so dont emplace unless isproxy is true.

            uint64_t id = _voters.available_primary_key();
            uint128_t addresshash = string_to_uint128_hash(fio_address.c_str());
            _voters.emplace(proxy, [&](auto &p) {
                p.id = id;
                p.fioaddress = fio_address;
                p.addresshash = addresshash;
                p.owner = proxy;
                p.is_proxy = isproxy;
            });
        }

        fio_400_assert(transaction_size() <= MAX_TRX_SIZE, "transaction_size", std::to_string(transaction_size()),
          "Transaction is too large", ErrorTransactionTooLarge);

    }

    void system_contract::propagate_weight_change(const voter_info &voter) {
        check(!voter.proxy || !voter.is_proxy, "account registered as a proxy is not allowed to use a proxy");

        //changed to get_unlocked_balance() Ed 11/25/2019
        uint64_t amount = get_votable_balance(voter.owner);
        //instead of staked we use the voters current FIO balance MAS-522 eliminate stake from voting.
        auto new_weight = (double)amount;
        if (voter.is_proxy) {
            new_weight += voter.proxied_vote_weight;
        }
        auto votersbyowner = _voters.get_index<"byowner"_n>();

        /// don't propagate small changes (1 ~= epsilon)
        if (fabs(new_weight - voter.last_vote_weight) > 1) {
            if (voter.proxy) {

                auto pitr = votersbyowner.find(voter.proxy.value);
                check(pitr != votersbyowner.end(),"proxy not found");

                votersbyowner.modify(pitr, same_payer, [&](auto &p) {
                                   p.proxied_vote_weight += new_weight - voter.last_vote_weight;
                               }
                );
                propagate_weight_change(*pitr);
            } else {
                auto delta = new_weight - voter.last_vote_weight;
                const auto ct = current_time_point();
                double delta_change_rate = 0;
                double total_inactive_vpay_share = 0;
                for (auto acnt : voter.producers) {
                    auto prodbyowner = _producers.get_index<"byowner"_n>();
                    auto prod =  prodbyowner.find(acnt.producer.value);
                    check(prod != prodbyowner.end(), "producer not found"); //data corruption
                    const double init_total_votes = prod->total_votes;
                    prodbyowner.modify(prod, same_payer, [&](auto &p) {
                        p.total_votes += delta;
                        _gstate.total_producer_vote_weight += delta;
                    });
                }

                update_total_votepay_share(ct, -total_inactive_vpay_share, delta_change_rate);
            }
        }
        auto pitr = votersbyowner.find(voter.owner.value);
        check(pitr != votersbyowner.end(),"voter not found");

        //adapt the total voted fio.
        if( pitr->last_vote_weight > 0.0 ) {
            _gstate.total_voted_fio -= pitr->last_vote_weight;
        }

        _gstate.total_voted_fio += new_weight;

        if( _gstate.total_voted_fio >= MINVOTEDFIO && _gstate.thresh_voted_fio_time == time_point() ) {
            _gstate.thresh_voted_fio_time = current_time_point();
        }

        votersbyowner.modify(pitr, same_payer, [&](auto &v) {
                           v.last_vote_weight = new_weight;
                       }
        );
    }

} /// namespace eosiosystem
