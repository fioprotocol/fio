/** FioName Token implementation file
 *  Description: FioName smart contract allows issuance of unique domains and names for easy public address resolution
 *  @author Adam Androulidakis, Casey Gardiner, Ciju John, Ed Rotthoff, Phil Mesnier
 *  @file fio.name.hpp
 *  @copyright Dapix
 */

#include "fio.name.hpp"
#include <fio.fee/fio.fee.hpp>
#include <fio.common/fio.common.hpp>
#include <fio.token/include/fio.token/fio.token.hpp>
#include <eosiolib/asset.hpp>

namespace fioio {

    class [[eosio::contract("FioNameLookup")]]  FioNameLookup : public eosio::contract {

    private:
        domains_table domains;
        chains_table chains;
        fionames_table fionames;
        fiofee_table fiofees;
        eosio_names_table accountmap;
        tpids_table tpids;
        eosiosystem::voters_table voters;
        config appConfig;



    public:
        using contract::contract;

        FioNameLookup(name s, name code, datastream<const char *> ds) : contract(s, code, ds),
                                                                        domains(_self, _self.value),
                                                                        fionames(_self, _self.value),
                                                                        fiofees(FeeContract, FeeContract.value),
                                                                        accountmap(_self, _self.value),
                                                                        chains(_self, _self.value),
                                                                        tpids(TPIDContract, TPIDContract.value),
                                                                        voters(SystemContract, SystemContract.value){
            configs_singleton configsSingleton(FeeContract, FeeContract.value);
            appConfig = configsSingleton.get_or_default(config());
        }


        inline name accountmgnt(const name &actor, const string &owner_fio_public_key) {
            require_auth(actor); // check for requestor authority; required for fee transfer

            name owner_account_name;

            //if the owner public key is empty then lookup the actor in the binding table, make sure its bound
            if (owner_fio_public_key.length() == 0) {
                //get the info from the binding table, and skip the public key verification
                //check that the account exists, and use the account.
                bool accountExists = is_account(actor);

                auto other = accountmap.find(actor.value);

                fio_400_assert(other != accountmap.end(), "owner_account", actor.to_string(),
                               "Account is not bound on the fio chain",
                               ErrorPubAddressExist);
                fio_400_assert(accountExists, "owner_account", actor.to_string(),
                               "Account does not yet exist on the fio chain",
                               ErrorPubAddressExist);

                owner_account_name = actor;

            } else {
                //check the owner_fio_public_key, if its empty then go and lookup the actor in the accountmap table
                //and use this as the owner_fio_public_key.
                eosio_assert(owner_fio_public_key.length() == 53, "Length of publik key should be 53");

                string pubkey_prefix("FIO");
                auto result = mismatch(pubkey_prefix.begin(), pubkey_prefix.end(),
                                       owner_fio_public_key.begin());
                eosio_assert(result.first == pubkey_prefix.end(),
                             "Public key should be prefix with FIO");
                auto base58substr = owner_fio_public_key.substr(pubkey_prefix.length());

                vector<unsigned char> vch;
                eosio_assert(decode_base58(base58substr, vch), "Decode pubkey failed");
                eosio_assert(vch.size() == 37, "Invalid public key");

                array<unsigned char, 33> pubkey_data;
                copy_n(vch.begin(), 33, pubkey_data.begin());

                capi_checksum160 check_pubkey;
                ripemd160(reinterpret_cast<char *>(pubkey_data.data()), 33, &check_pubkey);
                eosio_assert(memcmp(&check_pubkey.hash, &vch.end()[-4], 4) == 0,
                             "invalid public key");
                //end of the public key validity check.

                string owner_account;
                key_to_account(owner_fio_public_key, owner_account);
                owner_account_name = name(owner_account.c_str());

                print("hashed account name from the owner_fio_public_key ", owner_account, "\n");

                //see if the payee_actor is in the accountmap table.
                eosio_assert(owner_account.length() == 12, "Length of account name should be 12");

                bool accountExists = is_account(owner_account_name);

                auto other = accountmap.find(owner_account_name.value);

                if (other == accountmap.end()) { //the name is not in the table.
                    // if account does exist on the chain this is an error. DANGER account was created without binding!
                    fio_400_assert(!accountExists, "owner_account", owner_account,
                                   "Account exists on FIO chain but is not bound in accountmap",
                                   ErrorPubAddressExist);

                    //the account does not exist on the fio chain yet, and the binding does not exists
                    //yet, so create the account and then and add it to the accountmap table.
                    const auto owner_pubkey = abieos::string_to_public_key(owner_fio_public_key);

                    eosiosystem::key_weight pubkey_weight = {
                            .key = owner_pubkey,
                            .weight = 1,
                    };

                    const auto owner_auth = authority{1, {pubkey_weight}, {}, {}};

                    //create account
                    INLINE_ACTION_SENDER(call::eosio, newaccount)
                            ("eosio"_n, {{_self, "active"_n}},
                             {_self, owner_account_name, owner_auth, owner_auth}
                            );

                    print("created the account!!!!", owner_account_name, "\n");

                    uint64_t nmi = owner_account_name.value;

                    accountmap.emplace(_self, [&](struct eosio_name &p) {
                        p.account = nmi;
                        p.clientkey = owner_fio_public_key;
                        p.keyhash = string_to_uint64_hash(owner_fio_public_key.c_str());
                    });

                    print("performed bind of the account!!!!", owner_account_name, "\n");
                } else {
                    //if account does not on the chain this is an error. DANGER binding was recorded without the associated account.
                    fio_400_assert(accountExists, "owner_account", owner_account,
                                   "Account does not exist on FIO chain but is bound in accountmap",
                                   ErrorPubAddressExist);
                    //if the payee public key doesnt match whats in the accountmap table this is an error,it means there is a collision on hashing!
                    eosio_assert_message_code(owner_fio_public_key == other->clientkey, "FIO account already bound",
                                              ErrorPubAddressExist);
                }
            }//end else, the owner_fio_public_key has a value and is not empty.
            return owner_account_name;
        }



        static constexpr eosio::name token_account{"fio.token"_n};
        static constexpr eosio::name treasury_account{"fio.treasury"_n};

        inline void fio_fees(const name &actor, const asset &fee) const {
            if (appConfig.pmtson) {

                action(permission_level{actor, "active"_n},
                       token_account, "transfer"_n,
                       make_tuple(actor, treasury_account, fee,
                                  string("FIO API fees. Thank you."))
                ).send();

            } else {
                print("Payments currently disabled.");
            }
        }

        inline void process_tpid(const string &tpid, name owner) {

            uint64_t hashname = string_to_uint64_hash(tpid.c_str());
            print("process tpid hash of name ", tpid, " is value ", hashname ,"\n");

            auto iter = tpids.find(hashname);
            if (iter == tpids.end()){
                print("process tpid, tpid not found ","\n");
                //tpid does not exist. do nothing.
            }
            else{
                print("process tpid, found a tpid ","\n");
                //tpid exists, use the info to find the owner of the tpid
                auto iternm = fionames.find(iter->fioaddhash);
                if (iternm != fionames.end()) {
                    print("process found the fioname associated with the TPID in the fionames ","\n");
                    name proxy_name = name(iternm->owner_account);
                    //do the auto proxy
                    // autoproxy(proxy_name,owner);
                }
            }
        }


        inline void register_errors(const FioAddress &fa, bool domain) const {
            int res = fa.domainOnly ? isFioNameValid(fa.fiodomain) * 10 : isFioNameValid(fa.fioname);
            string fioname = "fio_address";
            string fioerror = "Invalid FIO address";
            if (domain) {
                fioname = "fio_domain";
                fioerror = "Invalid FIO domain";
            }

            fio_400_assert(res == 0, fioname, fa.fioaddress, fioerror, ErrorInvalidFioNameFormat);
        }

        inline void addaddress_errors(const string &tokencode, const string &pubaddress, const FioAddress &fa) const {
            fio_400_assert(isFioNameValid(fa.fioaddress), "fio_address", fa.fioaddress, "Invalid public address format",
                           ErrorDomainAlreadyRegistered);
            fio_400_assert(isChainNameValid(tokencode), "token_code", tokencode, "Invalid token code format",
                           ErrorInvalidFioNameFormat);
            fio_400_assert(isPubAddressValid(pubaddress), "public_address", pubaddress, "Invalid public address format",
                           ErrorChainAddressEmpty);
            fio_400_assert(!(pubaddress.size() == 0), "public_address", pubaddress, "Invalid public address format",
                           ErrorChainAddressEmpty);
        }

        uint32_t fio_address_update(const name &owneractor, const name &actor, uint64_t max_fee, const FioAddress &fa,
                                    const string &tpid) {
            // will not check the contents of tpid here, it was already checked at the beginning of regaddress that called this method

            uint32_t expiration_time = 0;
            uint64_t nameHash = string_to_uint64_hash(fa.fioaddress.c_str());
            uint64_t domainHash = string_to_uint64_hash(fa.fiodomain.c_str());

            fio_400_assert(!fa.domainOnly, "fio_address", fa.fioaddress, "Invalid FIO address",
                           ErrorInvalidFioNameFormat);

            // check if domain exists.
            auto domains_iter = domains.find(domainHash);
            fio_400_assert(domains_iter != domains.end(), "fio_address", fa.fioaddress, "FIO Domain not registered",
                           ErrorDomainNotRegistered);

            bool isPublic = domains_iter->public_domain;
            uint64_t domain_owner = domains_iter->account;

            if (!isPublic) {
                fio_400_assert(domain_owner == owneractor.value, "fio_address", fa.fioaddress,
                               "FIO Domain is not public. Only owner can create FIO Addresses.",
                               ErrorInvalidFioNameFormat);
            }

            //check if the domain is expired.
            uint32_t domain_expiration = domains_iter->expiration;
            uint32_t present_time = now();
            fio_400_assert(present_time <= domain_expiration, "fio_address", fa.fioaddress, "FIO Domain expired",
                           ErrorDomainExpired);

            // check if fioname is available
            auto fioname_iter = fionames.find(nameHash);
            fio_400_assert(fioname_iter == fionames.end(), "fio_address", fa.fioaddress,
                           "FIO address already registered", ErrorFioNameAlreadyRegistered);

            //set the expiration on this new fioname
            expiration_time = get_now_plus_one_year();

            // check if callee has requisite dapix funds.
            // DO SOMETHING

            // Issue, create and transfer fioname token
            // DO SOMETHING

            auto key_iter = accountmap.find(actor.value);
            uint64_t ownerHash = string_to_uint64_hash(key_iter->clientkey.c_str());
            print("OWNER:", actor, "...Value:", actor.value, "...Key:", key_iter->clientkey, "...hash:", ownerHash,
                  "\n");

            // Add fioname entry in fionames table
            fionames.emplace(_self, [&](struct fioname &a) {
                a.name = fa.fioaddress;
                a.addresses = vector<string>(20, ""); // TODO: Remove prior to production
                a.namehash = nameHash;
                a.domain = fa.fiodomain;
                a.domainhash = domainHash;
                a.expiration = expiration_time;
                a.owner_account = actor.value;
                a.bundleeligiblecountdown = 10000;
            });

            uint64_t fee_amount = chain_data_update(fa.fioaddress, "FIO", key_iter->clientkey, max_fee, fa, actor,
                                                    true, tpid);

            return expiration_time;
        }

        uint32_t fio_domain_update(const string &fio_domain, const string &owner_fio_public_key, const name &actor,
                                   const FioAddress &fa) {
            uint64_t domainHash = string_to_uint64_hash(fio_domain.c_str());
            uint32_t expiration_time;

            fio_400_assert(fa.domainOnly, "fio_address", fa.fioaddress, "Invalid FIO domain",
                           ErrorInvalidFioNameFormat);

            // check for domain availability
            auto domains_iter = domains.find(domainHash);
            fio_400_assert(domains_iter == domains.end(), "fio_name", fa.fioaddress,
                           "FIO domain already registered", ErrorDomainAlreadyRegistered);
            // check if callee has requisite dapix funds. Also update to domain fees

            //get the expiration for this new domain.
            expiration_time = get_now_plus_one_year();

            // Issue, create and transfer nft domain token
            // Add domain entry in domain table
            domains.emplace(_self, [&](struct domain &d) {
                d.name = fa.fiodomain;
                d.domainhash = domainHash;
                d.expiration = expiration_time;
                d.account = actor.value;
            });
            return expiration_time;
        }
        //adddomain

        uint64_t
        chain_data_update(const string &fioaddress, const string &tokencode, const string &pubaddress, uint64_t max_fee, const FioAddress &fa, const name &actor, const bool isFIO, const string &tpid) {

            uint64_t nameHash = string_to_uint64_hash(fa.fioaddress.c_str());
            uint64_t domainHash = string_to_uint64_hash(fa.fiodomain.c_str());

            auto fioname_iter = fionames.find(nameHash);
            fio_404_assert(fioname_iter != fionames.end(), "FIO Address not found", ErrorFioNameNotRegistered);

            //check that the name is not expired
            uint32_t name_expiration = fioname_iter->expiration;
            uint32_t present_time = now();

            uint64_t account = fioname_iter->owner_account;
            fio_403_assert(account == actor.value, ErrorSignature);
            fio_400_assert(present_time <= name_expiration, "fio_address", fioaddress,
                           "FIO Address expired", ErrorFioNameExpired);

            auto domains_iter = domains.find(domainHash);
            fio_404_assert(domains_iter != domains.end(), "FIO Domain not found", ErrorDomainNotFound);

            uint32_t expiration = domains_iter->expiration;
            fio_400_assert(present_time <= expiration, "domain", fa.fiodomain, "FIO Domain expired",
                           ErrorDomainExpired);

            uint64_t chainhash = string_to_uint64_hash(tokencode.c_str());
            auto size = distance(chains.cbegin(), chains.cend());
            auto chain_iter = chains.find(chainhash);

            if (chain_iter == chains.end()) {
                chains.emplace(_self, [&](struct chainList &a) {
                    a.id = size;
                    a.chainname = tokencode;
                    a.chainhash = chainhash;
                });
                chain_iter = chains.find(chainhash);
            }

            string oldkey = fioname_iter->addresses[static_cast<size_t>((chain_iter)->by_index())];
            uint64_t oldkeyhash = string_to_uint64_hash(oldkey.c_str());

            // insert/update <chain, address> pair
            fionames.modify(fioname_iter, _self, [&](struct fioname &a) {
                a.addresses[static_cast<size_t>((chain_iter)->by_index())] = pubaddress;
            });

            //begin new fees, bundle eligible fee logic
            uint64_t endpoint_hash = string_to_uint64_hash("add_pub_address");

            auto fees_by_endpoint = fiofees.get_index<"byendpoint"_n>();
            auto fee_iter = fees_by_endpoint.find(endpoint_hash);
            //if the fee isnt found for the endpoint, then 400 error.
            fio_400_assert(fee_iter != fees_by_endpoint.end(), "endpoint_name", "add_pub_address",
                           "FIO fee not found for endpoint", ErrorNoEndpoint);

            int64_t reg_amount = fee_iter->suf_amount;
            uint64_t fee_type = fee_iter->type;

            //if its not a bundleeligible fee then this is an error.
            fio_400_assert(fee_type == 1, "fee_type", to_string(fee_type),
                           "register_fio_address unexpected fee type for endpoint add_pub_address, expected 0",
                           ErrorNoEndpoint);

            uint64_t bundleeligiblecountdown = fioname_iter->bundleeligiblecountdown;

            uint64_t fee_amount = 0;

            if (isFIO) {
                return fee_amount;
            }

            if (bundleeligiblecountdown > 0) {
                //fee is zero, and decrement the counter.
                fee_amount = 0;

                fionames.modify(fioname_iter, _self, [&](struct fioname &a) {
                    a.bundleeligiblecountdown = (bundleeligiblecountdown - 1);
                });
            } else {
                fee_amount = fee_iter->suf_amount;
                fio_400_assert(max_fee >= fee_amount, "max_fee", to_string(max_fee), "Fee exceeds supplied maximum.",
                               ErrorMaxFeeExceeded);

                asset reg_fee_asset;
                //NOTE -- question here, should we always record the transfer for the fees, even when its zero,
                //or should we do as this code does and not do a transaction when the fees are 0.
                reg_fee_asset.symbol = symbol("FIO",9);
                reg_fee_asset.amount = reg_amount;
                print(reg_fee_asset.amount);
                //ADAM how to set thisreg_fee_asset = asset::from_string(to_string(reg_amount));
                fio_fees(actor, reg_fee_asset);
                process_rewards(tpid, reg_amount, get_self());
            }

            return fee_amount;

            //end new fees, bundle eligible fee logic
        }

        inline uint32_t get_time_plus_one_year(uint64_t timein) {
            uint32_t incremented_time = timein + YEARTOSECONDS;
            return incremented_time;
        }

        /***
         * This method will return now plus one year.
         * the result is the present block time, which is number of seconds since 1970
         * incremented by secondss per year.
         */
        inline uint32_t get_now_plus_one_year() {
            uint32_t present_time = now();
            uint32_t incremented_time = present_time + YEARTOSECONDS;
            return incremented_time;
        }


        /***
         * This method will decrement the now time by the specified number of years.
         * @param nyearsago   this is the number of years ago from now to return as a value
         * @return  the decremented now() time by nyearsago
         */
        inline uint32_t get_now_minus_years(int nyearsago) {
            uint32_t present_time = now();

            uint32_t decremented_time = present_time - (YEARTOSECONDS * nyearsago);
            return decremented_time;
        }

        /***
         * This method will increment the now time by the specified number of years.
         * @param nyearsago   this is the number of years from now to return as a value
         * @return  the decremented now() time by nyearsago
         */
        inline uint32_t get_now_plus_years(int nyearsago) {
            uint32_t present_time = now();

            uint32_t decremented_time = present_time + (YEARTOSECONDS * nyearsago);
            return decremented_time;
        }

        /********* CONTRACT ACTIONS ********/

        // this action will perform the logic of checking the voter_info,
        // and setting the proxy and auto proxy for auto proxy.
        inline void autoproxy(name proxy_name, name owner_name )
        {

            print(" called autoproxy","\n");
            //check the voter_info table for a record matching owner_name.
            auto viter = voters.find(owner_name.value);
            if (viter == voters.end())
            {
                print(" autoproxy voter info not found, calling crautoproxy","\n");
                //if the record is not there then send inline action to crautoprx (a new action in the system contract).
                //note this action will set the auto proxy and is_aut_proxy, so return after.
                // Buy ram for account.

                action(
                        permission_level{get_self(), "active"_n},
                        "eosio"_n,
                        "crautoproxy"_n,
                        std::make_tuple(proxy_name, owner_name)
                ).send();

                return;

            }
            else
            {
                //check if the record has auto proxy and proxy matching proxy_name, set has_proxy. if so return.
                if (viter->is_auto_proxy)
                {
                    if (proxy_name == viter->proxy) {
                        return;
                    }
                }
                else if ((viter->proxy) || (viter->producers.size() > 0))
                {
                    //check if the record has another proxy or producers. if so return.
                    return;
                }

                //invoke the fio.system contract action to set auto proxy and proxy name.
                action(
                        permission_level{get_self(), "active"_n},
                        "eosio"_n,
                        "setautoproxy"_n,
                        std::make_tuple(proxy_name, owner_name)
                ).send();
            }
        }

        [[eosio::action]]
        void regaddress(const string &fio_address, const string &owner_fio_public_key, uint64_t max_fee, const name &actor, const string &tpid) {

            if(!tpid.empty()) {
              process_tpid(tpid, actor);
            }

            name owner_account_name = accountmgnt(actor, owner_fio_public_key);
            // Split the fio name and domain portions

            FioAddress fa;
            getFioAddressStruct(fio_address, fa);
            register_errors(fa, false);
            name nm = name{owner_account_name};

            uint32_t expiration_time = fio_address_update(actor, nm, max_fee, fa, tpid);

            //begin new fees, logic for Mandatory fees.
            uint64_t endpoint_hash = string_to_uint64_hash("register_fio_address");

            auto fees_by_endpoint = fiofees.get_index<"byendpoint"_n>();
            auto fee_iter = fees_by_endpoint.find(endpoint_hash);
            //if the fee isnt found for the endpoint, then 400 error.
            fio_400_assert(fee_iter != fees_by_endpoint.end(), "endpoint_name", "register_fio_address",
                           "FIO fee not found for endpoint", ErrorNoEndpoint);

            uint64_t reg_amount = fee_iter->suf_amount;
            uint64_t fee_type = fee_iter->type;

            //if its not a mandatory fee then this is an error.
            fio_400_assert(fee_type == 0, "fee_type", to_string(fee_type),
                           "register_fio_address unexpected fee type for endpoint register_fio_address, expected 0",
                           ErrorNoEndpoint);

            fio_400_assert(max_fee >= reg_amount, "max_fee", to_string(max_fee), "Fee exceeds supplied maximum.",
                           ErrorMaxFeeExceeded);

            asset reg_fee_asset;
            reg_fee_asset.symbol = symbol("FIO",9);
            reg_fee_asset.amount = reg_amount;
            print(reg_fee_asset.amount);
            //ADAM how to set thisreg_fee_asset = asset::from_string(to_string(reg_amount));
            fio_fees(actor, reg_fee_asset);
            process_rewards(tpid, reg_amount, get_self());

            //end new fees, logic for Mandatory fees.

            nlohmann::json json = {{"status",        "OK"},
                                   {"expiration",    expiration_time},
                                   {"fee_collected", reg_amount}};
            send_response(json.dump().c_str());

        }

        [[eosio::action]]
        void
        regdomain(const string &fio_domain, const string &owner_fio_public_key, uint64_t max_fee, const name &actor, const string &tpid) {
            name owner_account_name = accountmgnt(actor, owner_fio_public_key);
            // Split the fio name and domain portions
            if(!tpid.empty()) {
              process_tpid(tpid, actor);
            }
            FioAddress fa;
            getFioAddressStruct(fio_domain, fa);
            register_errors(fa, true);

            name nm = name{owner_account_name};

            uint32_t expiration_time = fio_domain_update(fio_domain, owner_fio_public_key, actor, fa);

            //begin new fees, logic for Mandatory fees.
            uint64_t endpoint_hash = string_to_uint64_hash("register_fio_domain");

            auto fees_by_endpoint = fiofees.get_index<"byendpoint"_n>();
            auto fee_iter = fees_by_endpoint.find(endpoint_hash);
            //if the fee isnt found for the endpoint, then 400 error.
            fio_400_assert(fee_iter != fees_by_endpoint.end(), "endpoint_name", "register_fio_domain",
                           "FIO fee not found for endpoint", ErrorNoEndpoint);

            uint64_t reg_amount = fee_iter->suf_amount;
            uint64_t fee_type = fee_iter->type;

            //if its not a mandatory fee then this is an error.
            fio_400_assert(fee_type == 0, "fee_type", to_string(fee_type),
                           "register_fio_address unexpected fee type for endpoint register_fio_domain, expected 0",
                           ErrorNoEndpoint);

            fio_400_assert(max_fee >= reg_amount, "max_fee", to_string(max_fee), "Fee exceeds supplied maximum.",
                           ErrorMaxFeeExceeded);

            asset reg_fee_asset;
            //ADAM how to set thisreg_fee_asset = asset::from_string(to_string(reg_amount));
            reg_fee_asset.symbol = symbol("FIO",9);
            reg_fee_asset.amount = reg_amount;
            print(reg_fee_asset.amount);
            //ADAM how to set thisreg_fee_asset = asset::from_string(to_string(reg_amount));
            fio_fees(actor, reg_fee_asset);
            process_rewards(tpid, reg_amount, get_self());
            //end new fees, logic for Mandatory fees.

            nlohmann::json json = {{"status",        "OK"},
                                   {"expiration",    expiration_time},
                                   {"fee_collected", reg_amount}};

            send_response(json.dump().c_str());
        }

        /*
         * This action will renew the specified domain.
         * NOTE-- TPID is not used, this needs to be integrated properly when tpid recording is integrated
         *
         */
        [[eosio::action]]
        void
        renewdomain(const string &fio_domain, uint64_t max_fee, const string &tpid, const name &actor) {

            // Split the fio name and domain portions
            if(!tpid.empty()) {
              process_tpid(tpid, actor);
            }
            FioAddress fa;
            getFioAddressStruct(fio_domain, fa);
            register_errors(fa, true);

            uint64_t domainHash = string_to_uint64_hash(fio_domain.c_str());
            uint32_t expiration_time;

            fio_400_assert(fa.domainOnly, "fio_address", fa.fioaddress, "Invalid FIO domain",
                           ErrorInvalidFioNameFormat);

            // check for domain availability
            auto domains_iter = domains.find(domainHash);
            fio_400_assert(domains_iter != domains.end(), "fio_domain", fa.fioaddress,
                           "FIO domain not found", ErrorDomainNotRegistered);


            expiration_time = domains_iter->expiration;

            //begin new fees, logic for Mandatory fees.
            uint64_t endpoint_hash = string_to_uint64_hash("renew_fio_domain");

            auto fees_by_endpoint = fiofees.get_index<"byendpoint"_n>();
            auto fee_iter = fees_by_endpoint.find(endpoint_hash);
            //if the fee isnt found for the endpoint, then 400 error.
            fio_400_assert(fee_iter != fees_by_endpoint.end(), "endpoint_name", "register_fio_domain",
                           "FIO fee not found for endpoint", ErrorNoEndpoint);

            uint64_t reg_amount = fee_iter->suf_amount;
            uint64_t fee_type = fee_iter->type;

            //if its not a mandatory fee then this is an error.
            fio_400_assert(fee_type == 0, "fee_type", to_string(fee_type),
                           "register_fio_address unexpected fee type for endpoint register_fio_domain, expected 0",
                           ErrorNoEndpoint);

            fio_400_assert(max_fee >= reg_amount, "max_fee", to_string(max_fee), "Fee exceeds supplied maximum.",
                           ErrorMaxFeeExceeded);

            asset reg_fee_asset;

            reg_fee_asset.symbol = symbol("FIO",9);
            reg_fee_asset.amount = reg_amount;
            print(reg_fee_asset.amount);

            fio_fees(actor, reg_fee_asset);
            process_rewards(tpid, reg_amount, get_self());

            //end new fees, logic for Mandatory fees.

            //update the index table.

            uint64_t new_expiration_time = get_time_plus_one_year(expiration_time);

            domains.modify(domains_iter, _self, [&](struct domain &a) {
                    a.expiration = new_expiration_time;
            });

            nlohmann::json json = {{"status",        "OK"},
                                   {"expiration",    new_expiration_time},
                                   {"fee_collected", reg_amount}};

            send_response(json.dump().c_str());
        }

        /*
        * This action will renew the specified address.
        * NOTE-- TPID is not used, this needs to be integrated properly when tpid recording is integrated
        *
        */
        [[eosio::action]]
        void
        renewaddress(const string &fio_domain, uint64_t max_fee, const string &tpid, const name &actor) {
            if(!tpid.empty()) {
              process_tpid(tpid,actor);
            }
            // Split the fio name and domain portions
            FioAddress fa;
            getFioAddressStruct(fio_domain, fa);
            register_errors(fa, false);


            uint64_t nameHash = string_to_uint64_hash(fa.fioaddress.c_str());
            uint64_t domainHash = string_to_uint64_hash(fa.fiodomain.c_str());

            fio_400_assert(!fa.domainOnly, "fio_address", fa.fioaddress, "Invalid FIO address",
                           ErrorInvalidFioNameFormat);

            // check if domain exists.
            auto domains_iter = domains.find(domainHash);
            fio_400_assert(domains_iter != domains.end(), "fio_address", fa.fioaddress, "FIO Domain not registered",
                           ErrorDomainNotRegistered);

            //check if the domain is expired.
            uint32_t domain_expiration = domains_iter->expiration;
            uint32_t present_time = now();
            fio_400_assert(present_time <= domain_expiration, "fio_address", fa.fioaddress, "FIO Domain expired",
                           ErrorDomainExpired);

            // check if fioname is available
            auto fioname_iter = fionames.find(nameHash);
            fio_400_assert(fioname_iter != fionames.end(), "fio_address", fa.fioaddress,
                           "FIO address not registered", ErrorFioNameNotRegistered);

            //set the expiration on this new fioname
            uint64_t expiration_time = fioname_iter->expiration;
            uint64_t bundleeligiblecountdown = fioname_iter->bundleeligiblecountdown;


            //begin new fees, logic for Mandatory fees.
            uint64_t endpoint_hash = string_to_uint64_hash("renew_fio_address");

            auto fees_by_endpoint = fiofees.get_index<"byendpoint"_n>();
            auto fee_iter = fees_by_endpoint.find(endpoint_hash);
            //if the fee isnt found for the endpoint, then 400 error.
            fio_400_assert(fee_iter != fees_by_endpoint.end(), "endpoint_name", "register_fio_address",
                           "FIO fee not found for endpoint", ErrorNoEndpoint);

            uint64_t reg_amount = fee_iter->suf_amount;
            uint64_t fee_type = fee_iter->type;

            //if its not a mandatory fee then this is an error.
            fio_400_assert(fee_type == 0, "fee_type", to_string(fee_type),
                           "register_fio_address unexpected fee type for endpoint register_fio_address, expected 0",
                           ErrorNoEndpoint);

            fio_400_assert(max_fee >= reg_amount, "max_fee", to_string(max_fee), "Fee exceeds supplied maximum.",
                           ErrorMaxFeeExceeded);

            asset reg_fee_asset;

            reg_fee_asset.symbol = symbol("FIO",9);
            reg_fee_asset.amount = reg_amount;
            print(reg_fee_asset.amount);

            fio_fees(actor, reg_fee_asset);
            process_rewards(tpid, reg_amount, get_self());

            //end new fees, logic for Mandatory fees.

            //update the index table.

            uint64_t new_expiration_time = get_time_plus_one_year(expiration_time);

            fionames.modify(fioname_iter, _self, [&](struct fioname &a) {
                a.expiration = new_expiration_time;
                a.bundleeligiblecountdown = 10000 + bundleeligiblecountdown;
            });

            nlohmann::json json = {{"status",        "OK"},
                                   {"expiration",    new_expiration_time},
                                   {"fee_collected", reg_amount}};

            send_response(json.dump().c_str());
        }

        /*
         * TESTING ONLY, REMOVE for main net launch!
         * This action will create a domain of the specified name and the domain will be
         * expired.
         */
        void expdomain (const name &actor, const string &domain){
            uint64_t domainHash = string_to_uint64_hash(domain.c_str());
            uint64_t expiration_time = get_now_minus_years(5);

            auto iter4 = domains.find(domainHash);

            if (iter4 == domains.end()) {
                domains.emplace(_self, [&](struct domain &d) {
                    d.name = domain;
                    d.domainhash = domainHash;
                    d.expiration = expiration_time;
                    d.account = actor.value;
                });
            }
        }

        /*
         * TESTING ONLY this action should be removed for main net launch!!!
         * This action will add the specified number of expired addresses to the specified domain using the
         * specified address prefix, values will also be added into the keynames table using the address prefix.
         */
        [[eosio::action]]
        void expaddresses(const name &actor, const string &domain, const string &address_prefix, uint64_t number_addresses_to_add) {

            uint64_t nameHash ;
            uint64_t domainHash = string_to_uint64_hash(domain.c_str());
            uint64_t expiration_time = get_now_minus_years(1);

            int countAdded = 0;
            for (int i=0;i<10000;i++) {

                string name;
                if (i==0) {
                   name = address_prefix + "." + domain;
                }else {
                    name = address_prefix + to_string(i*2) + "." + domain;
                }

                int yearsago = 1;
                if (i>7){
                    yearsago = i / 7;
                }

                expiration_time = get_now_minus_years(yearsago);
                nameHash = string_to_uint64_hash(name.c_str());
                auto iter1 = fionames.find(nameHash);
                if (iter1 == fionames.end()) {
                    //set up a couple of expired names in the fionames table.
                    fionames.emplace(_self, [&](struct fioname &a) {
                        a.name = name;
                        a.addresses = vector<string>(20, ""); // TODO: Remove prior to production
                        a.namehash = nameHash;
                        a.domain = domain;
                        a.domainhash = domainHash;
                        a.expiration = expiration_time;
                        a.owner_account = actor.value;
                        a.bundleeligiblecountdown = 10000;
                    });
                    countAdded++;
                }

                if (countAdded == number_addresses_to_add){
                    print("created ",countAdded, " in the domain ",domain,"\n");
                    break;
                }
            }

        }



        /*
         * This action will look for expired domains, then look for expired addresses, it will burn a total
         * of 100 addresses each time called. please see the code for the logic of identifying expired domains
         * and addresses.
         *   Dev note on testing
         *   to make an expired domain.
         *   cleos -u http://localhost:8889 push action -j fio.system expdomain '{"actor":"r41zuwovtn44","domain":"expired"}' --permission r41zuwovtn44@active
         *   to create expired addresses under the specified domain.
         *   cleos -u http://localhost:8889 push action -j fio.system expaddresses '{"actor":"r41zuwovtn44","domain":"expired","address_prefix":"eddieexp","number_addresses_to_add":"5"}' --permission r41zuwovtn44@active
         *   scenarios that need tested.
         *   1) create an expired domain with fewer than 100 expired addresses within it. run the burnexpired
         *   2) create an expired domain with over 100 expired addresses within it. run the burnexpired repeatedly until all are removed.
         *   3) create an expired address under a non expired domain. run the burn expired.
         *   4) create an expired domain with a few expired addresses. create an expired address under a non expired domain. run burnexpired.
         *   5) create an expired domain with over 100 addresses, create over 100 expired addresses in a non expired domain. run burnexpired repeatedly until all are removed.
         *
         */
        [[eosio::action]]
        void burnexpired() {

            //this is the burn list holding the list of address hashes that should be destroyed.
            std::vector <uint64_t> burnlist;
            std::vector <uint64_t> domainburnlist;

            //we look back 20 years for expired things.
            int numbertoburn = 100;
            int windowmaxyears = 20;
            //number of seconds in a day.
            uint64_t secondsperday = 86400;
            //amount of time to wait to burn a domain beyond its expriation date
            uint64_t domainwaitforburndays = 90 * secondsperday;
            //amount of time to wait to burn an address beyond its expriation date
            uint64_t addresswaitforburndays = 90 * secondsperday;
            //the time now, use this everywhere to avoid any odd behaviors during execution..
            uint64_t nowtime = now();
            //the minimum expiration to look for in searching for expired items
            uint32_t minexpiration = get_now_minus_years(windowmaxyears);

            //fio names by expiration.
            auto nameexpidx = fionames.get_index<"byexpiration"_n>();
            //fio domains by expiration
            auto domainexpidx = domains.get_index<"byexpiration"_n>();
            auto fionamesbydomainhashidx = fionames.get_index<"bydomain"_n>();

            //using this instead of now time will place everything in the to be burned list, for testing only.
            uint64_t kludgedNow = get_now_plus_years(10); // This is for testing only

            //first find all domains with expiration greater than or equal to minexpiration.
            //since the index returns values in ascending order we get the oldest expired first.
            //this is a good order to burn them in which is oldest to youngest.
            auto domainiter = domainexpidx.lower_bound(minexpiration);

            //loop over all of the entries,until we find one with expire time plus wait time > now time.
            //if its less than or equal to nowtime then it needs burned.
            while (domainiter != domainexpidx.end()) {
                uint64_t expire = domainiter->expiration;
                uint64_t domainnamehash = domainiter->domainhash;

                if ((expire + domainwaitforburndays) > nowtime) //check for done searching.
                //if ((expire + domainwaitforburndays) > kludgedNow) //this is for testing only
                {
                    break;
                } else {   //add up to 100 addresses, add all addresses in domain until 100 is hit, or all are added.
                    auto domainhash = domainiter->domainhash;
                    auto nmiter = fionamesbydomainhashidx.find(domainhash);

                    while (nmiter != fionamesbydomainhashidx.end()) {
                        //look at all addresses in this domain, add until 100
                        burnlist.push_back(nmiter->namehash);
                        if (burnlist.size() >= numbertoburn) {
                            break;
                        }
                        nmiter++;
                    }

                    //if we processed all the addresses inside a domain then add the domain itself to the list
                    //to be burned. since its in the fionames table.
                    if (nmiter == fionamesbydomainhashidx.end()) {
                        burnlist.push_back(domainnamehash);
                        domainburnlist.push_back(domainnamehash);
                    }

                    //since we can add the domain, check one more time for macx number to burn inserted.
                    if (burnlist.size() >= numbertoburn) {
                        break;
                    }
                }
                domainiter++;
            }

            //check if we have enough to remove already, if not move on to the addresses
            if (burnlist.size() < numbertoburn) {

                //add addresses to the burn list until 100 total. if 100 total continue the loop. or exahaust the expired addresses
                auto nameiter = nameexpidx.lower_bound(minexpiration);

                while (nameiter != nameexpidx.end()) {
                    uint64_t expire = nameiter->expiration;
                    if ((expire + addresswaitforburndays) > nowtime)
                        //if ((expire + addresswaitforburndays) > kludgedNow) //this is for testing only.
                    {
                        break;
                    } else {
                        //if its in the burn list already, dont re-add. since we did expired domains first, we can
                        //get duplicate names attempted to be inserted, keep the duplicates out.
                        if (!(std::find(burnlist.begin(), burnlist.end(), nameiter->namehash) != burnlist.end())) {
                            burnlist.push_back(nameiter->namehash);
                            if (burnlist.size() >= numbertoburn) {
                                break;
                            }
                        }
                    }
                    nameiter++;
                }
            }

            //do the burning.
            for (int i = 0; i < burnlist.size(); i++) {
                uint64_t burner = burnlist[i];
                //to call erase we need to have the primary key, get the list of primary keys out of keynames
                vector <uint64_t> ids;

                //remove the items from the fionames
                auto fionamesiter = fionames.find(burner);
                if (fionamesiter != fionames.end()) {
                    fionames.erase(fionamesiter);
                }

            }

            for (int i = 0; i < domainburnlist.size(); i++) {
                uint64_t burner = burnlist[i];

                auto domainsiter = domains.find(burner);
                if (domainsiter != domains.end()) {
                    domains.erase(domainsiter);
                }
            }

            //done with burning, return the result.
            nlohmann::json json = {{"status", "OK"},
                                   {"items_burned",burnlist.size()}
            };

            send_response(json.dump().c_str());
        }



        /***
         * Given a fio user name, chain name and chain specific address will attach address to the user's FIO fioname.
         *
         * @param fioaddress The FIO user name e.g. "adam.fio"
         * @param tokencode The chain name e.g. "btc"
         * @param pubaddress The chain specific user address
         */
        [[eosio::action]]
        void
        addaddress(const string &fio_address, const string &token_code, const string &public_address, uint64_t max_fee,
                   const name &actor,const string &tpid) {
             if(!tpid.empty()) {
               process_tpid(tpid, actor);
             }

            FioAddress fa;
            getFioAddressStruct(fio_address, fa);
            addaddress_errors(token_code, public_address, fa);

            uint64_t fee_amount = chain_data_update(fio_address, token_code, public_address, max_fee, fa, actor, false, tpid);

            nlohmann::json json = {{"status",        "OK"},
                                   {"fee_collected", fee_amount}};
            send_response(json.dump().c_str());

        } //addaddress

        [[eosio::action]]
        void
        setdomainpub(const string &fio_domain, const bool public_domain, uint64_t max_fee, const name &actor,
                     const string &tpid) {
            if (!tpid.empty()) {
                process_tpid(tpid, actor);
            }

            FioAddress fa;
            uint32_t present_time = now();
            getFioAddressStruct(fio_domain, fa);
            register_errors(fa, true);

            uint64_t domainHash = string_to_uint64_hash(fio_domain.c_str());
            auto domain_iter = domains.find(domainHash);

            fio_400_assert(domain_iter != domains.end(), "fio_domain", fa.fioaddress, "Invalid FIO domain",
                           ErrorDomainNotRegistered);
            fio_400_assert(fa.domainOnly, "fio_domain", fa.fioaddress, "Invalid FIO domain",
                           ErrorInvalidFioNameFormat);

            uint64_t expiration = domain_iter->expiration;
            fio_400_assert(present_time <= expiration, "fio_domain", fa.fiodomain, "FIO Domain expired",
                           ErrorDomainExpired);

            domains.modify(domain_iter, _self, [&](struct domain &a) {
                a.public_domain = public_domain;
            });

            uint64_t endpoint_hash = string_to_uint64_hash("set_fio_domain_public");

            auto fees_by_endpoint = fiofees.get_index<"byendpoint"_n>();
            auto fee_iter = fees_by_endpoint.find(endpoint_hash);
            uint64_t fee_type = fee_iter->type;
            int64_t reg_amount = fee_iter->suf_amount;

            fio_400_assert(fee_iter != fees_by_endpoint.end(), "endpoint_name", "register_fio_domain",
                           "FIO fee not found for endpoint", ErrorNoEndpoint);

            uint64_t fee_amount = fee_iter->suf_amount;
            fio_400_assert(max_fee >= fee_amount, "max_fee", to_string(max_fee), "Fee exceeds supplied maximum.",
                           ErrorMaxFeeExceeded);

            asset reg_fee_asset;
            //NOTE -- question here, should we always record the transfer for the fees, even when its zero,
            //or should we do as this code does and not do a transaction when the fees are 0.
            reg_fee_asset.symbol = symbol("FIO", 9);
            reg_fee_asset.amount = reg_amount;
            print(reg_fee_asset.amount);
            //ADAM how to set thisreg_fee_asset = asset::from_string(to_string(reg_amount));
            fio_fees(actor, reg_fee_asset);
            process_rewards(tpid, reg_amount, get_self());

            nlohmann::json json = {{"status",        "OK"},
                                   {"expiration",    expiration},
                                   {"fee_collected", fee_amount}};
            send_response(json.dump().c_str());
        }

        /**
         *
         * Separate out the management of platform-specific identities from the fio names
         * and domains. bind2eosio, the space restricted variant of "Bind to EOSIO"
         * takes a platform-specific account name and a wallet generated public key.
         *
         * First it verifie that either tsi is a new account and none othe exists, or this
         * is an existing eosio account and it is indeed bound to this key. If it is a new,
         * unbound account name, then bind name to the key and add it to the list.
         *
         **/

        [[eosio::action]]
        void bind2eosio(name account, const string &client_key, bool existing) {
            // The caller of this contract must have the private key in their wallet for the FIO.SYSTEM account
            //NOTE -- ed removed the require authorization when integrating the account management into
            //        the smart contract layer, the fio.token contract needs to invoke this action from
            //        the fio.token contract, this requires that either the fio.token account get the authorization
            //        of fio.system, or that this line of code be removed. this needs more thought in the future.
            // require_auth(name(FIO_SYSTEM));

            auto other = accountmap.find(account.value);
            if (other != accountmap.end()) {
                eosio_assert_message_code(existing && client_key == other->clientkey, "EOSIO account already bound",
                                          ErrorPubAddressExist);
                // name in the table and it matches
            } else {
                eosio_assert_message_code(!existing, "existing EOSIO account not bound to a key", ErrorPubAddressExist);
                accountmap.emplace(_self, [&](struct eosio_name &p) {
                    p.account = account.value;
                    p.clientkey = client_key;
                    p.keyhash = string_to_uint64_hash(client_key.c_str());
                });
            }
            print ("bind of account is done processing","\n");
        }

        void removename() {
            print("Begin removename()");
        }

        void removedomain() {
            print("Begin removedomain()");
        }

        void rmvaddress() {
            print("Begin rmvaddress()");
        }

        void decrcounter(const string &fio_address) {

            string tstr = fio_address;
            uint64_t hashval = string_to_uint64_hash(tstr.c_str());

            auto fioname_iter = fionames.find(hashval);
            fio_400_assert(fioname_iter != fionames.end(), "fio_address", fio_address,
                           "FIO address not registered", ErrorFioNameAlreadyRegistered);

            uint64_t bundleeligiblecountdown = fioname_iter->bundleeligiblecountdown;

            if (bundleeligiblecountdown > 0) {
                fionames.modify(fioname_iter, _self, [&](struct fioname &a) {
                    a.bundleeligiblecountdown = (bundleeligiblecountdown - 1);
                });
            }
        }
    }; // class FioNameLookup

    EOSIO_DISPATCH(FioNameLookup, (regaddress)(addaddress)(regdomain)(renewdomain)(renewaddress)(expdomain)(
            expaddresses)(setdomainpub)(burnexpired)(removename)(removedomain)(rmvaddress)(decrcounter)
    (bind2eosio))
}
