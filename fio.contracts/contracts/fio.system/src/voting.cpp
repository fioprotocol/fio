/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <fio.system/fio.system.hpp>

#include <eosiolib/eosio.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/datastream.hpp>
#include <eosiolib/serialize.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/privileged.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/transaction.hpp>
#include <fio.token/fio.token.hpp>

#include <algorithm>
#include <cmath>

namespace eosiosystem {
    using eosio::indexed_by;
    using eosio::const_mem_fun;
    using eosio::singleton;
    using eosio::transaction;

    /**
     *  This method will create a producer_config and producer_info object for 'producer'
     *
     *  @pre producer is not already registered
     *  @pre producer to register is an account
     *  @pre authority of producer to register
     *
     */
    void
    system_contract::regiproducer(const name producer, const eosio::public_key &producer_key, const std::string &url,
                                  uint16_t location) {
        check(url.size() < 512, "url too long");
        check(producer_key != eosio::public_key(), "public key should not be the default value");
        require_auth(producer);

        auto prod = _producers.find(producer.value);
        const auto ct = current_time_point();

        if (prod != _producers.end()) {
            _producers.modify(prod, producer, [&](producer_info &info) {
                info.producer_key = producer_key;
                info.is_active = true;
                info.url = url;
                info.location = location;
                if (info.last_claim_time == time_point())
                    info.last_claim_time = ct;
            });

            auto prod2 = _producers2.find(producer.value);
            if (prod2 == _producers2.end()) {
                _producers2.emplace(producer, [&](producer_info2 &info) {
                    info.owner = producer;
                    info.last_votepay_share_update = ct;
                });
                update_total_votepay_share(ct, 0.0, prod->total_votes);
                // When introducing the producer2 table row for the first time, the producer's votes must also be accounted for in the global total_producer_votepay_share at the same time.
            }
        } else {
            _producers.emplace(producer, [&](producer_info &info) {
                info.owner = producer;
                info.total_votes = 0;
                info.producer_key = producer_key;
                info.is_active = true;
                info.url = url;
                info.location = location;
                info.last_claim_time = ct;
            });
            _producers2.emplace(producer, [&](producer_info2 &info) {
                info.owner = producer;
                info.last_votepay_share_update = ct;
            });
        }
    }

    void
    system_contract::regproducer(const string fio_address, const std::string &url, uint16_t location, const name actor,
                                 uint16_t max_fee) {
        FioAddress fa;
        getFioAddressStruct(fio_address, fa);

        uint64_t nameHash = string_to_uint64_hash(fa.fioaddress.c_str());
        uint64_t domainHash = string_to_uint64_hash(fa.fiodomain.c_str());

        //need to verify the account that owns the address is the actor.
        auto fioname_iter = _fionames.find(nameHash);
        fio_404_assert(fioname_iter != _fionames.end(), "FIO Address not found", ErrorFioNameNotRegistered);

        //check that the name is not expired
        uint32_t name_expiration = fioname_iter->expiration;
        uint32_t present_time = now();

        uint64_t account = fioname_iter->owner_account;
        fio_403_assert(account == actor.value, ErrorSignature);
        fio_400_assert(present_time <= name_expiration, "fio_address", fio_address,
                       "FIO Address expired", ErrorFioNameExpired);

        auto domains_iter = _domains.find(domainHash);
        fio_404_assert(domains_iter != _domains.end(), "FIO Domain not found", ErrorDomainNotFound);

        uint32_t expiration = domains_iter->expiration;
        fio_400_assert(present_time <= expiration, "domain", fa.fiodomain, "FIO Domain expired",
                       ErrorDomainExpired);

        auto key_iter = _accountmap.find(account);
        const auto owner_pubkey = abieos::string_to_public_key(key_iter->owner);

        regiproducer(actor, owner_pubkey, url, location);

        //TODO: REFACTOR FEE ( PROXY / PRODUCER )
        //begin new fees, logic for Mandatory fees.
        uint64_t endpoint_hash = string_to_uint64_hash("register_producer");

        auto fees_by_endpoint = _fiofees.get_index<"byendpoint"_n>();
        auto fee_iter = fees_by_endpoint.find(endpoint_hash);
        //if the fee isnt found for the endpoint, then 400 error.
        fio_400_assert(fee_iter != fees_by_endpoint.end(), "endpoint_name", "register_producer",
                       "FIO fee not found for endpoint", ErrorNoEndpoint);

        uint64_t reg_amount = fee_iter->suf_amount;
        uint64_t fee_type = fee_iter->type;

        //if its not a mandatory fee then this is an error.
        fio_400_assert(fee_type == 0, "fee_type", to_string(fee_type),
                       "register_fio_address unexpected fee type for endpoint register_producer, expected 0",
                       ErrorNoEndpoint);

        fio_400_assert(max_fee >= reg_amount, "max_fee", to_string(max_fee), "Fee exceeds supplied maximum.",
                       ErrorMaxFeeExceeded);

        asset reg_fee_asset;
        reg_fee_asset.symbol = symbol("FIO", 9);
        reg_fee_asset.amount = reg_amount;
        print(reg_fee_asset.amount);

        fio_fees(actor, reg_fee_asset);

        //end new fees, logic for Mandatory fees.

        nlohmann::json json = {{"status",        "OK"},
                               {"fee_collected", reg_amount}};
        send_response(json.dump().c_str());
    }

    void system_contract::unregprod(const name producer) {
        require_auth(producer);

        const auto &prod = _producers.get(producer.value, "producer not found");
        _producers.modify(prod, same_payer, [&](producer_info &info) {
            info.deactivate();
        });
    }

    bool sort_producers_by_location(std::pair<eosio::producer_key,uint16_t> a, std::pair<eosio::producer_key,uint16_t> b) {
              return (a.second < b.second);
    }

    void system_contract::update_elected_producers(block_timestamp block_time) {
        _gstate.last_producer_schedule_update = block_time;

        auto idx = _producers.get_index<"prototalvote"_n>();

        std::vector <std::pair<eosio::producer_key, uint16_t>> top_producers;
        top_producers.reserve(21);

        for (auto it = idx.cbegin();
             it != idx.cend() && top_producers.size() < 21 && 0 < it->total_votes && it->active(); ++it) {
            top_producers.emplace_back(
                    std::pair<eosio::producer_key, uint16_t>({{it->owner, it->producer_key}, it->location}));
        }

        if (top_producers.size() < _gstate.last_producer_schedule_size) {
            return;
        }

        /// sort by producer location, location initialized to zero in fio.system.hpp
        /// location will be set upon call to register producer by the block producer.
        ///the location should be considered as a scheduled order of producers, they should
        /// set the location so that traversing locations gives most proximal producer locations
        /// to help address latency.
        std::sort( top_producers.begin(), top_producers.end(), sort_producers_by_location );

        std::vector <eosio::producer_key> producers;

        producers.reserve(top_producers.size());
        for (const auto &item : top_producers)
            producers.push_back(item.first);

        auto packed_schedule = pack(producers);

        if (set_proposed_producers(packed_schedule.data(), packed_schedule.size()) >= 0) {
            _gstate.last_producer_schedule_size = static_cast<decltype(_gstate.last_producer_schedule_size)>( top_producers.size());
        }
    }



    double stake2vote(int64_t staked) {
       //in EOS the weighting of a vote is strengthened each week. in FIO we remove this ever increasing vote strength
       //and we just always use the amount staked.
       // double weight =
       //         int64_t((now() - (block_timestamp::block_timestamp_epoch / 1000)) / (seconds_per_day * 7)) / double(52);
       // return double(staked) * std::pow(2, weight);
       return double(staked);
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

    double system_contract::update_producer_votepay_share(const producers_table2::const_iterator &prod_itr,
                                                          time_point ct,
                                                          double shares_rate,
                                                          bool reset_to_zero) {
        double delta_votepay_share = 0.0;
        if (shares_rate > 0.0 && ct > prod_itr->last_votepay_share_update) {
            delta_votepay_share = shares_rate * double((ct - prod_itr->last_votepay_share_update).count() /
                                                       1E6); // cannot be negative
        }

        double new_votepay_share = prod_itr->votepay_share + delta_votepay_share;
        _producers2.modify(prod_itr, same_payer, [&](auto &p) {
            if (reset_to_zero)
                p.votepay_share = 0.0;
            else
                p.votepay_share = new_votepay_share;

            p.last_votepay_share_update = ct;
        });

        return new_votepay_share;
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
    void system_contract::voteproducer(const name voter_name, const name proxy, const std::vector <name> &producers) {
        require_auth(voter_name);
        update_votes(voter_name, proxy, producers, true);
    }

    void system_contract::update_votes(const name voter_name, const name proxy, const std::vector <name> &producers,
                                       bool voting) {
        //validate input
        if (proxy) {
            check(producers.size() == 0, "cannot vote for producers and proxy at same time");
            check(voter_name != proxy, "cannot proxy to self");
        } else {
            check(producers.size() <= 30, "attempt to vote for too many producers");
            for (size_t i = 1; i < producers.size(); ++i) {
                check(producers[i - 1] < producers[i], "producer votes must be unique and sorted");
            }
        }

        auto voter = _voters.find(voter_name.value);
        check(voter != _voters.end(), "user must stake before they can vote"); /// staking creates voter object
        check(!proxy || !voter->is_proxy, "account registered as a proxy is not allowed to use a proxy");

        /**
         * The first time someone votes we calculate and set last_vote_weight, since they cannot unstake until
         * after total_activated_stake hits threshold, we can use last_vote_weight to determine that this is
         * their first vote and should consider their stake activated.
         */
        if (voter->last_vote_weight <= 0.0) {
            _gstate.total_activated_stake += voter->staked;
            if (_gstate.total_activated_stake >= min_activated_stake &&
                _gstate.thresh_activated_stake_time == time_point()) {
                _gstate.thresh_activated_stake_time = current_time_point();
            }
        }

        auto new_vote_weight = stake2vote(voter->staked);
        if (voter->is_proxy) {
            new_vote_weight += voter->proxied_vote_weight;
        }

        boost::container::flat_map <name, pair<double, bool /*new*/>> producer_deltas;
        if (voter->last_vote_weight > 0) {
            if (voter->proxy) {
                auto old_proxy = _voters.find(voter->proxy.value);
                check(old_proxy != _voters.end(), "old proxy not found"); //data corruption
                _voters.modify(old_proxy, same_payer, [&](auto &vp) {
                    vp.proxied_vote_weight -= voter->last_vote_weight;
                });
                propagate_weight_change(*old_proxy);
            } else {
                for (const auto &p : voter->producers) {
                    auto &d = producer_deltas[p];
                    d.first -= voter->last_vote_weight;
                    d.second = false;
                }
            }
        }

        if (proxy) {
            auto new_proxy = _voters.find(proxy.value);
            check(new_proxy != _voters.end(),
                  "invalid proxy specified"); //if ( !voting ) { data corruption } else { wrong vote }
            check(!voting || new_proxy->is_proxy, "proxy not found");
            if (new_vote_weight >= 0) {
                _voters.modify(new_proxy, same_payer, [&](auto &vp) {
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
        for (const auto &pd : producer_deltas) {
            auto pitr = _producers.find(pd.first.value);
            if (pitr != _producers.end()) {
                check(!voting || pitr->active() || !pd.second.second /* not from new set */,
                      "producer is not currently registered");
                double init_total_votes = pitr->total_votes;
                _producers.modify(pitr, same_payer, [&](auto &p) {
                    p.total_votes += pd.second.first;
                    if (p.total_votes < 0) { // floating point arithmetics can give small negative numbers
                        p.total_votes = 0;
                    }
                    _gstate.total_producer_vote_weight += pd.second.first;
                    //check( p.total_votes >= 0, "something bad happened" );
                });
                auto prod2 = _producers2.find(pd.first.value);
                if (prod2 != _producers2.end()) {
                    const auto last_claim_plus_3days = pitr->last_claim_time + microseconds(3 * useconds_per_day);
                    bool crossed_threshold = (last_claim_plus_3days <= ct);
                    bool updated_after_threshold = (last_claim_plus_3days <= prod2->last_votepay_share_update);
                    // Note: updated_after_threshold implies cross_threshold

                    double new_votepay_share = update_producer_votepay_share(prod2,
                                                                             ct,
                                                                             updated_after_threshold ? 0.0
                                                                                                     : init_total_votes,
                                                                             crossed_threshold &&
                                                                             !updated_after_threshold // only reset votepay_share once after threshold
                    );

                    if (!crossed_threshold) {
                        delta_change_rate += pd.second.first;
                    } else if (!updated_after_threshold) {
                        total_inactive_vpay_share += new_votepay_share;
                        delta_change_rate -= init_total_votes;
                    }
                }
            } else {
                check(!pd.second.second /* not from new set */, "producer is not registered"); //data corruption
            }
        }

        update_total_votepay_share(ct, -total_inactive_vpay_share, delta_change_rate);

        _voters.modify(voter, same_payer, [&](auto &av) {
            av.last_vote_weight = new_vote_weight;
            av.producers = producers;
            av.proxy = proxy;
        });
    }

    void system_contract::setautoproxy(name proxy,name owner)
    {
        //first verify that the proxy exists and is registered as a proxy.
        //look it up and check it.
        //if its there then emplace the owner record into the voting_info table with is_auto_proxy set.
        auto itervi = _voters.find(proxy.value);
        check(itervi != _voters.end(), "specified proxy not found.");
        check(itervi->is_proxy == true,"specified proxy is not registered as a proxy");


        itervi = _voters.find(owner.value);
        check(itervi != _voters.end(), "specified owner not found.");
        _voters.modify(itervi, same_payer, [&](auto &av) {
            av.is_auto_proxy = true;
            av.proxy = proxy;
        });

    }


    void system_contract::crautoproxy(name proxy,name owner)
    {
        print ("call to set auto proxy for voter ",owner," to proxy ",proxy,"\n");
        //first verify that the proxy exists and is registered as a proxy.
        //look it up and check it.
        //if its there then emplace the owner record into the voting_info table with is_auto_proxy set.
        auto itervi = _voters.find(proxy.value);
        //this needs to be silent. so comment out this following check. remove after uat.
        //check(itervi != _voters.end(), "specified proxy not found.");
       //this needs to be silent. so comment out this following check. remove after uat.
       // check(itervi->is_proxy == true,"specified proxy is not registered as a proxy");

        if (itervi != _voters.end() &&
           itervi->is_proxy) {

            auto itervoter = _voters.find(owner.value);
            if (itervoter == _voters.end()) {
                _voters.emplace(owner, [&](auto &p) {
                    p.owner = owner;
                    p.is_auto_proxy = true;
                    p.proxy = proxy;
                });
                print("new proxy has been set to tpid ", proxy, "\n");
            } else if (itervoter->is_auto_proxy && itervoter->proxy != proxy) {
                _voters.modify(itervoter, _self, [&](struct voter_info &a) {
                    a.proxy = proxy;
                });
                print("auto proxy was updated to be tpid ",proxy,"\n");
            }
        }else{
            print("could not find the tpid ",proxy, " in the voters table or this voter is not a proxy","\n");
        }

    }

    static constexpr eosio::name token_account{"fio.token"_n};
    static constexpr eosio::name treasury_account{"fio.treasury"_n};

    inline void fio_fees(const name &actor, const asset &fee)  {
            action(permission_level{actor, "active"_n},
                   token_account, "transfer"_n,
                   make_tuple(actor, treasury_account, fee,
                              string("FIO API fees. Thank you."))
            ).send();

    }

    void system_contract::unregproxy(const std::string &fio_address,const name &actor,uint64_t max_fee ) {
        FioAddress fa;
        getFioAddressStruct(fio_address, fa);

        uint64_t nameHash = string_to_uint64_hash(fa.fioaddress.c_str());
        uint64_t domainHash = string_to_uint64_hash(fa.fiodomain.c_str());

        //need to verify the account that owns the address is the actor.
        auto fioname_iter = _fionames.find(nameHash);
        fio_404_assert(fioname_iter != _fionames.end(), "FIO Address not found", ErrorFioNameNotRegistered);

        //check that the name is not expired
        uint32_t name_expiration = fioname_iter->expiration;
        uint32_t present_time = now();

        uint64_t account = fioname_iter->owner_account;
        fio_403_assert(account == actor.value, ErrorSignature);
        fio_400_assert(present_time <= name_expiration, "fio_address", fio_address,
                       "FIO Address expired", ErrorFioNameExpired);

        auto domains_iter = _domains.find(domainHash);
        fio_404_assert(domains_iter != _domains.end(), "FIO Domain not found", ErrorDomainNotFound);

        uint32_t expiration = domains_iter->expiration;
        fio_400_assert(present_time <= expiration, "domain", fa.fiodomain, "FIO Domain expired",
                       ErrorDomainExpired);

        regiproxy(actor,fio_address,false);

        //begin new fees, logic for Mandatory fees.
        uint64_t endpoint_hash = string_to_uint64_hash("unregister_proxy");

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

        fio_400_assert(max_fee >= reg_amount, "max_fee", to_string(max_fee), "Fee exceeds supplied maximum.",
                       ErrorMaxFeeExceeded);

        asset reg_fee_asset;
        reg_fee_asset.symbol = symbol("FIO",9);
        reg_fee_asset.amount = reg_amount;
        print(reg_fee_asset.amount);

        fio_fees(actor, reg_fee_asset);

        //end new fees, logic for Mandatory fees.

        nlohmann::json json = {{"status",        "OK"},
                               {"fee_collected", reg_amount}};
        send_response(json.dump().c_str());
    }


    void system_contract::regproxy(const std::string &fio_address,const name &actor,uint64_t max_fee ) {
        FioAddress fa;
        getFioAddressStruct(fio_address, fa);

        uint64_t nameHash = string_to_uint64_hash(fa.fioaddress.c_str());
        uint64_t domainHash = string_to_uint64_hash(fa.fiodomain.c_str());

        //need to verify the account that owns the address is the actor.
        auto fioname_iter = _fionames.find(nameHash);
        fio_404_assert(fioname_iter != _fionames.end(), "FIO Address not found", ErrorFioNameNotRegistered);

        //check that the name is not expired
        uint32_t name_expiration = fioname_iter->expiration;
        uint32_t present_time = now();

        uint64_t account = fioname_iter->owner_account;
        fio_403_assert(account == actor.value, ErrorSignature);
        fio_400_assert(present_time <= name_expiration, "fio_address", fio_address,
                       "FIO Address expired", ErrorFioNameExpired);

        auto domains_iter = _domains.find(domainHash);
        fio_404_assert(domains_iter != _domains.end(), "FIO Domain not found", ErrorDomainNotFound);

        uint32_t expiration = domains_iter->expiration;
        fio_400_assert(present_time <= expiration, "domain", fa.fiodomain, "FIO Domain expired",
                       ErrorDomainExpired);

        regiproxy(actor,fio_address,true);

        //begin new fees, logic for Mandatory fees.
        uint64_t endpoint_hash = string_to_uint64_hash("register_proxy");

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

        fio_400_assert(max_fee >= reg_amount, "max_fee", to_string(max_fee), "Fee exceeds supplied maximum.",
                       ErrorMaxFeeExceeded);

        asset reg_fee_asset;
        reg_fee_asset.symbol = symbol("FIO",9);
        reg_fee_asset.amount = reg_amount;
        print(reg_fee_asset.amount);

        fio_fees(actor, reg_fee_asset);

        //end new fees, logic for Mandatory fees.

        nlohmann::json json = {{"status",        "OK"},
                               {"fee_collected", reg_amount}};
        send_response(json.dump().c_str());
    }



    /**
     * this action will allow a caller to register a proxy for use in voting going forward.
     * or it will allow a caller to register as a proxy for use by others going forward.
     * to register the use of a proxy call this with the desired proxy and false
     * to register as a proxy going forward call this with the desired proxy and true.
     *
     *  @param isproxy - true if proxy wishes to vote on behalf of others, false otherwise
     */
    void system_contract::regiproxy(const name proxy, const string &fio_address, bool isproxy) {

       require_auth(proxy);

       print ("called regiproxy with proxy ",proxy, " isproxy ", isproxy,"\n");

        auto pitr = _voters.find(proxy.value);
        if (pitr != _voters.end()) {

            //if the values are equal and isproxy, then show this error.
            fio_400_assert((isproxy != pitr->is_proxy)|| !isproxy, "public_address", fio_address,
                           "Already registered as proxy. ", ErrorPubAddressExist);
            //check(!isproxy || !pitr->proxy, "account that uses a proxy is not allowed to become a proxy");
            if (isproxy && !pitr->proxy) {
                _voters.modify(pitr, same_payer, [&](auto &p) {
                    p.is_proxy = isproxy;
                    p.is_auto_proxy = false;
                });
            }else if (!isproxy) { //this is how we undo/clear a proxy
                name nm;
                _voters.modify(pitr, same_payer, [&](auto &p) {
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
            _voters.emplace(proxy, [&](auto &p) {
                p.owner = proxy;
                p.is_proxy = isproxy;
            });
        }
    }

    void system_contract::propagate_weight_change(const voter_info &voter) {
        check(!voter.proxy || !voter.is_proxy, "account registered as a proxy is not allowed to use a proxy");
        double new_weight = stake2vote(voter.staked);
        if (voter.is_proxy) {
            new_weight += voter.proxied_vote_weight;
        }

        /// don't propagate small changes (1 ~= epsilon)
        if (fabs(new_weight - voter.last_vote_weight) > 1) {
            if (voter.proxy) {
                auto &proxy = _voters.get(voter.proxy.value, "proxy not found"); //data corruption
                _voters.modify(proxy, same_payer, [&](auto &p) {
                                   p.proxied_vote_weight += new_weight - voter.last_vote_weight;
                               }
                );
                propagate_weight_change(proxy);
            } else {
                auto delta = new_weight - voter.last_vote_weight;
                const auto ct = current_time_point();
                double delta_change_rate = 0;
                double total_inactive_vpay_share = 0;
                for (auto acnt : voter.producers) {
                    auto &prod = _producers.get(acnt.value, "producer not found"); //data corruption
                    const double init_total_votes = prod.total_votes;
                    _producers.modify(prod, same_payer, [&](auto &p) {
                        p.total_votes += delta;
                        _gstate.total_producer_vote_weight += delta;
                    });
                    auto prod2 = _producers2.find(acnt.value);
                    if (prod2 != _producers2.end()) {
                        const auto last_claim_plus_3days = prod.last_claim_time + microseconds(3 * useconds_per_day);
                        bool crossed_threshold = (last_claim_plus_3days <= ct);
                        bool updated_after_threshold = (last_claim_plus_3days <= prod2->last_votepay_share_update);
                        // Note: updated_after_threshold implies cross_threshold

                        double new_votepay_share = update_producer_votepay_share(prod2,
                                                                                 ct,
                                                                                 updated_after_threshold ? 0.0
                                                                                                         : init_total_votes,
                                                                                 crossed_threshold &&
                                                                                 !updated_after_threshold // only reset votepay_share once after threshold
                        );

                        if (!crossed_threshold) {
                            delta_change_rate += delta;
                        } else if (!updated_after_threshold) {
                            total_inactive_vpay_share += new_votepay_share;
                            delta_change_rate -= init_total_votes;
                        }
                    }
                }

                update_total_votepay_share(ct, -total_inactive_vpay_share, delta_change_rate);
            }
        }
        _voters.modify(voter, same_payer, [&](auto &v) {
                           v.last_vote_weight = new_weight;
                       }
        );
    }

} /// namespace eosiosystem
