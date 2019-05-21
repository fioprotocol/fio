/** FioName Token implementation file
 *  Description: FioName smart contract allows issuance of unique domains and names for easy public address resolution
 *  @author Adam Androulidakis, Casey Gardiner, Ciju John, Ed Rotthoff, Phil Mesnier
 *  @file fio.name.hpp
 *  @copyright Dapix
 */

#include "fio.name.hpp"
#include <fio.fee/fio.fee.hpp>
#include <fio.common/fio.common.hpp>
#include <eosiolib/asset.hpp>

namespace fioio {

    class [[eosio::contract("FioNameLookup")]]  FioNameLookup : public eosio::contract {

    private:
        domains_table domains;
        chains_table chains;
        fionames_table fionames;
        keynames_table keynames;
        fiofee_table fiofees;
        eosio_names_table eosionames;
        config appConfig;

        const name TokenContract = name("fio.token");

    public:
        using contract::contract;

        FioNameLookup(name s, name code, datastream<const char *> ds) : contract(s, code, ds),
                                                                        domains(_self, _self.value),
                                                                        fionames(_self, _self.value),
                                                                        keynames(_self, _self.value),
                                                                        fiofees(FeeContract, FeeContract.value),
                                                                        eosionames(_self, _self.value),
                                                                        chains(_self, _self.value) {
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

                auto other = eosionames.find(actor.value);

                fio_400_assert(other != eosionames.end(), "owner_account", actor.to_string(),
                               "Account is not bound on the fio chain ",
                               ErrorPubAddressExist);
                fio_400_assert(accountExists, "owner_account", actor.to_string(),
                               "Account does not yet exist on the fio chain ",
                               ErrorPubAddressExist);


                owner_account_name = actor;
            } else {
                //check the owner_fio_public_key, if its empty then go and lookup the actor in the eosionames table
                //and use this as the owner_fio_public_key.
                eosio_assert(owner_fio_public_key.length() == 53, "Length of publik key should be 53");

                string pubkey_prefix("FIO");
                auto result = mismatch(pubkey_prefix.begin(), pubkey_prefix.end(),
                                       owner_fio_public_key.begin());
                eosio_assert(result.first == pubkey_prefix.end(),
                             "Public key should be prefix with EOS");
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

                //see if the payee_actor is in the eosionames table.
                eosio_assert(owner_account.length() == 12, "Length of account name should be 12");

                bool accountExists = is_account(owner_account_name);

                auto other = eosionames.find(owner_account_name.value);

                if (other == eosionames.end()) { //the name is not in the table.
                    // if account does exist on the chain this is an error. DANGER account was created without binding!
                    fio_400_assert(!accountExists, "owner_account", owner_account,
                                   "Account exists on FIO chain but is not bound in eosionames",
                                   ErrorPubAddressExist);

                    //the account does not exist on the fio chain yet, and the binding does not exists
                    //yet, so create the account and then and add it to the eosionames table.
                    const auto owner_pubkey = abieos::string_to_public_key(owner_fio_public_key);

                    eosiosystem::key_weight pubkey_weight = {
                            .key = owner_pubkey,
                            .weight = 1,
                    };

                    const auto owner_auth = authority{1, {pubkey_weight}, {}, {}};
                    const auto rbprice = rambytes_price(3 * 1024);

                    // Create account.
                    action(
                            permission_level{get_self(), "active"_n},
                            "eosio"_n,
                            "newaccount"_n,
                            std::make_tuple(get_self(), owner_account_name, owner_auth, owner_auth)
                    ).send();


                    // Buy ram for account.
                    INLINE_ACTION_SENDER(eosiosystem::system_contract, buyram)
                            ("eosio"_n, {{_self, "active"_n}},
                             {_self, owner_account_name, rbprice});

                    // Replace lost ram.
                    INLINE_ACTION_SENDER(eosiosystem::system_contract, buyram)
                            ("eosio"_n, {{_self, "active"_n}},
                             {_self, _self, rbprice});


                    print("created the account!!!!", owner_account_name, "\n");

                    uint64_t nmi = owner_account_name.value;
                    eosionames.emplace(_self, [&](struct eosio_name &p) {
                        p.account = nmi;
                        p.clientkey = owner_fio_public_key;
                        p.keyhash = string_to_uint64_hash(owner_fio_public_key.c_str());
                    });

                    print("performed bind of the account!!!!", owner_account_name, "\n");
                } else {
                    //if account does not on the chain this is an error. DANGER binding was recorded without the associated account.
                    fio_400_assert(accountExists, "owner_account", owner_account,
                                   "Account does not exist on FIO chain but is bound in eosionames",
                                   ErrorPubAddressExist);
                    //if the payee public key doesnt match whats in the eosionames table this is an error,it means there is a collision on hashing!
                    eosio_assert_message_code(owner_fio_public_key == other->clientkey, "FIO account already bound",
                                              ErrorPubAddressExist);
                }
            }//end else, the owner_fio_public_key has a value and is not empty.
            return owner_account_name;
        }


        inline void fio_fees(const name &actor, const asset &fee) const {
            if (appConfig.pmtson) {
                name fiosystem = name("fio.system");

                action(permission_level{actor, "active"_n},
                       TokenContract, "transfer"_n,
                       make_tuple(actor, fiosystem, fee,
                                  string("FIO API fees. Thank you."))
                ).send();

            } else {
                print("Payments currently disabled.");
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

        uint32_t fio_address_update(const name &actor, uint64_t max_fee, const FioAddress &fa) {
            uint32_t expiration_time = 0;
            uint64_t nameHash = string_to_uint64_hash(fa.fioaddress.c_str());
            uint64_t domainHash = string_to_uint64_hash(fa.fiodomain.c_str());

            fio_400_assert(!fa.domainOnly, "fio_address", fa.fioaddress, "Invalid FIO address",
                           ErrorInvalidFioNameFormat);

            // check if domain exists.
            auto domains_iter = domains.find(domainHash);
            fio_400_assert(domains_iter != domains.end(), "fio_address", fa.fioaddress, "FIO Domain not registered",
                           ErrorDomainNotRegistered);

            // TODO check if domain permission is valid.

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

            // Add fioname entry in fionames table
            fionames.emplace(_self, [&](struct fioname &a) {
                a.name = fa.fioaddress;
                a.addresses = vector<string>(20, ""); // TODO: Remove prior to production
                a.namehash = nameHash;
                a.domain = fa.fiodomain;
                a.domainhash = domainHash;
                a.expiration = expiration_time;
                a.account = actor.value;
                a.bundleeligiblecountdown = 10000;
            });
            addaddress(fa.fioaddress, "FIO", actor.to_string(), max_fee, actor);

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
            // insert/update key into key-name table for reverse lookup
            auto idx = keynames.get_index<"bykey"_n>();
            auto keyhash = string_to_uint64_hash(owner_fio_public_key.c_str());
            auto matchingItem = idx.lower_bound(keyhash);
            auto domain_iter = domains.find(domainHash);

            uint32_t domain_expiration = domain_iter->expiration;
            // TODO: Is there a fee for adding a domain ?

            // Advance to the first entry matching the specified address and chain
            while (matchingItem != idx.end() && matchingItem->keyhash == keyhash) {
                matchingItem++;
            }

            if (matchingItem == idx.end() || matchingItem->keyhash != keyhash) {
                keynames.emplace(_self, [&](struct key_name &k) {
                    k.id = keynames.available_primary_key();        // use next available primary key
                    k.key = owner_fio_public_key;                             // persist key
                    k.keyhash = keyhash;                            // persist key hash
                    k.chaintype = 0;                       // specific chain type
                    k.name = domain_iter->name;                    // FIO name
                    k.expiration = domain_expiration;
                });
            } else {
                idx.modify(matchingItem, _self, [&](struct key_name &k) {
                    k.name = domain_iter->name;    // FIO name
                });
            }
            return expiration_time;
        }
        //adddomain

        uint64_t
        chain_data_update(const string &fioaddress, const string &tokencode, const string &pubaddress, uint64_t max_fee,
                          const FioAddress &fa, const name &actor) {
            uint64_t nameHash = string_to_uint64_hash(fa.fioaddress.c_str());
            uint64_t domainHash = string_to_uint64_hash(fa.fiodomain.c_str());

            auto fioname_iter = fionames.find(nameHash);
            fio_404_assert(fioname_iter != fionames.end(), "FIO Address not found", ErrorFioNameNotRegistered);

            //check that the name is not expired
            uint32_t name_expiration = fioname_iter->expiration;
            uint32_t present_time = now();

            uint64_t account = fioname_iter->account;
            //      print("account: ", account, " actor: ", actor, "\n");
            fio_403_assert(account == actor.value, ErrorSignature);

            //print("name_expiration: ", name_expiration, ", present_time: ", present_time, "\n");
            fio_400_assert(present_time <= name_expiration, "fioaddress", fioaddress,
                           "FIO Address or FIO Domain expired", ErrorFioNameExpired);

            auto domains_iter = domains.find(domainHash);
            fio_404_assert(domains_iter != domains.end(), "FIO Domain not found", ErrorDomainNotFound);

            uint32_t expiration = domains_iter->expiration;
            fio_400_assert(present_time <= expiration, "domain", fa.fiodomain, "FIO Address or FIO Domain expired",
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

            // insert/update key into key-name table for reverse lookup
            auto idx = keynames.get_index<"bykey"_n>();
            auto keyhash = string_to_uint64_hash(pubaddress.c_str());
            auto matchingItem = idx.lower_bound(keyhash);
            auto oldItem = idx.lower_bound(oldkeyhash);

            // Advance to the first entry matching the specified address and chain
            while (matchingItem != idx.end() && matchingItem->keyhash == keyhash) {
                matchingItem++;
            }

            if ((oldItem)->chaintype == (chain_iter)->by_index() && oldItem != idx.end() &&
                fioaddress == (oldItem)->name) {
                idx.erase(oldItem);
            }

            if (matchingItem == idx.end() || matchingItem->keyhash != keyhash) {
                keynames.emplace(_self, [&](struct key_name &k) {
                    k.id = keynames.available_primary_key();        // use next available primary key
                    k.key = pubaddress;                             // persist key
                    k.keyhash = keyhash;                            // persist key hash
                    k.chaintype = (chain_iter)->by_index();                       // specific chain type
                    k.name = fioname_iter->name;                    // FIO name
                    k.expiration = name_expiration;
                });
            } else {
                idx.modify(matchingItem, _self, [&](struct key_name &k) {
                    k.name = fioname_iter->name;    // FIO name
                });
            }

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


                //NOTE -- question here, should we always record the transfer for the fees, even when its zero,
                //or should we do as this code does and not do a transaction when the fees are 0.
                asset reg_fee_asset;
                //ADAM how to set thisreg_fee_asset = asset::from_string(to_string(reg_amount));
                fio_fees(actor, reg_fee_asset);
            }

            return fee_amount;

            //end new fees, bundle eligible fee logic
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


        /********* CONTRACT ACTIONS ********/

        [[eosio::action]]
        void
        regaddress(const string &fio_address, const string &owner_fio_public_key, uint64_t max_fee, const name &actor) {
            name owner_account_name = accountmgnt(actor, owner_fio_public_key);
            // Split the fio name and domain portions
            FioAddress fa;
            getFioAddressStruct(fio_address, fa);
            register_errors(fa, false);

            name nm = name{owner_account_name};

            uint32_t expiration_time = fio_address_update(nm, max_fee, fa);

            //begin new fees, logic for Mandatory fees.
            uint64_t endpoint_hash = string_to_uint64_hash("register_fio_address");

            auto fees_by_endpoint = fiofees.get_index<"byendpoint"_n>();
            auto fee_iter = fees_by_endpoint.find(endpoint_hash);
            //if the fee isnt found for the endpoint, then 400 error.
            fio_400_assert(fee_iter != fees_by_endpoint.end(), "endpoint_name", "register_fio_address",
                           "FIO fee not found for endpoint", ErrorNoEndpoint);

            int64_t reg_amount = fee_iter->suf_amount;
            uint64_t fee_type = fee_iter->type;

            //if its not a mandatory fee then this is an error.
            fio_400_assert(fee_type == 0, "fee_type", to_string(fee_type),
                           "register_fio_address unexpected fee type for endpoint register_fio_address, expected 0",
                           ErrorNoEndpoint);

            fio_400_assert(max_fee >= reg_amount, "max_fee", to_string(max_fee), "Fee exceeds supplied maximum.",
                           ErrorMaxFeeExceeded);

            asset reg_fee_asset;
            //ADAM how to set this reg_fee_asset = asset::from_string(to_string(reg_amount));
            fio_fees(actor, reg_fee_asset);

            //end new fees, logic for Mandatory fees.

            nlohmann::json json = {{"status",        "OK"},
                                   {"expiration",    expiration_time},
                                   {"fee_collected", reg_amount}};
            send_response(json.dump().c_str());

        }

        [[eosio::action]]
        void
        regdomain(const string &fio_domain, const string &owner_fio_public_key, uint64_t max_fee, const name &actor) {
            name owner_account_name = accountmgnt(actor, owner_fio_public_key);
            // Split the fio name and domain portions
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
            fio_fees(actor, reg_fee_asset);

            //end new fees, logic for Mandatory fees.

            nlohmann::json json = {{"status",        "OK"},
                                   {"expiration",    expiration_time},
                                   {"fee_collected", reg_amount}};

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
                   const name &actor) {
            FioAddress fa;
            getFioAddressStruct(fio_address, fa);
            addaddress_errors(token_code, public_address, fa);

            uint64_t fee_amount = chain_data_update(fio_address, token_code, public_address, max_fee, fa, actor);


            nlohmann::json json = {{"status",        "OK"},
                                   {"fee_collected", fee_amount}};
            send_response(json.dump().c_str());

        } //addaddress

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

            auto other = eosionames.find(account.value);
            if (other != eosionames.end()) {
                eosio_assert_message_code(existing && client_key == other->clientkey, "EOSIO account already bound",
                                          ErrorPubAddressExist);
                // name in the table and it matches
            } else {
                eosio_assert_message_code(!existing, "existing EOSIO account not bound to a key", ErrorPubAddressExist);
                eosionames.emplace(_self, [&](struct eosio_name &p) {
                    p.account = account.value;
                    p.clientkey = client_key;
                    p.keyhash = string_to_uint64_hash(client_key.c_str());
                });
            }
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

    EOSIO_DISPATCH(FioNameLookup, (regaddress)(addaddress)(regdomain)(removename)(removedomain)(rmvaddress)(decrcounter)
    (bind2eosio))
}
