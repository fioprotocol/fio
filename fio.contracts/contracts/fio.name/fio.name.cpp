/** FioName Token implementation file
 *  Description: FioName smart contract allows issuance of unique domains and names for easy public address resolution
 *  @author Adam Androulidakis, Casey Gardiner, Ciju John, Ed Rotthoff, Phil Mesnier
 *  @file fio.name.hpp
 *  @copyright Dapix
 */

#include "fio.name.hpp"
#include <fio.fee/fio.fee.hpp>
#include <fio.common/fio.common.hpp>
#include <fio.common/fiotime.hpp>
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
        bundlevoters_table bundlevoters;
        tpids_table tpids;
        eosiosystem::voters_table voters;
        config appConfig;

    public:
        using contract::contract;

        FioNameLookup(name s, name code, datastream<const char *> ds) : contract(s, code, ds),
                                                                        domains(_self, _self.value),
                                                                        fionames(_self, _self.value),
                                                                        fiofees(FeeContract, FeeContract.value),
                                                                        bundlevoters(FeeContract, FeeContract.value),
                                                                        accountmap(_self, _self.value),
                                                                        chains(_self, _self.value),
                                                                        tpids(TPIDContract, TPIDContract.value),
                                                                        voters(SystemContract, SystemContract.value) {
            configs_singleton configsSingleton(FeeContract, FeeContract.value);
            appConfig = configsSingleton.get_or_default(config());
        }

        inline name accountmgnt(const name &actor, const string &owner_fio_public_key) {
            require_auth(actor);

            name owner_account_name;

            if (owner_fio_public_key.length() == 0) {
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

                eosio_assert(owner_account.length() == 12, "Length of account name should be 12");

                bool accountExists = is_account(owner_account_name);
                auto other = accountmap.find(owner_account_name.value);

                if (other == accountmap.end()) { //the name is not in the table.
                    fio_400_assert(!accountExists, "owner_account", owner_account,
                                   "Account exists on FIO chain but is not bound in accountmap",
                                   ErrorPubAddressExist);

                    const auto owner_pubkey = abieos::string_to_public_key(owner_fio_public_key);

                    eosiosystem::key_weight pubkey_weight = {
                            .key = owner_pubkey,
                            .weight = 1,
                    };

                    const auto owner_auth = authority{1, {pubkey_weight}, {}, {}};

                    INLINE_ACTION_SENDER(call::eosio, newaccount)
                            ("eosio"_n, {{_self, "active"_n}},
                             {_self, owner_account_name, owner_auth, owner_auth}
                            );

                    uint64_t nmi = owner_account_name.value;

                    accountmap.emplace(_self, [&](struct eosio_name &p) {
                        p.account = nmi;
                        p.clientkey = owner_fio_public_key;
                        p.keyhash = string_to_uint128_hash(owner_fio_public_key.c_str());
                    });
                } else {
                    fio_400_assert(accountExists, "owner_account", owner_account,
                                   "Account does not exist on FIO chain but is bound in accountmap",
                                   ErrorPubAddressExist);
                    eosio_assert_message_code(owner_fio_public_key == other->clientkey, "FIO account already bound",
                                              ErrorPubAddressExist);
                }
            }
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

        inline double getBundledAmount() {
            int totalcount = 0;
            double returnvalue = 0;

            if (bundlevoters.end() == bundlevoters.begin()) {
                return 10000;
            }

            for (const auto &itr : bundlevoters) {
                returnvalue += itr.bundledbvotenumber;
                totalcount++;
            }

            if (totalcount == 0) {
                return 10000;
            }
            return returnvalue / totalcount;
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

            uint32_t expiration_time = 0;
            uint128_t nameHash = string_to_uint128_hash(fa.fioaddress.c_str());
            uint128_t domainHash = string_to_uint128_hash(fa.fiodomain.c_str());

            fio_400_assert(!fa.domainOnly, "fio_address", fa.fioaddress, "Invalid FIO address",
                           ErrorInvalidFioNameFormat);

            auto domainsbyname = domains.get_index<"byname"_n>();
            auto domains_iter = domainsbyname.find(domainHash);

            fio_400_assert(domains_iter != domainsbyname.end(), "fio_address", fa.fioaddress,
                           "FIO Domain not registered",
                           ErrorDomainNotRegistered);

            bool isPublic = domains_iter->is_public;
            uint64_t domain_owner = domains_iter->account;

            if (!isPublic) {
                fio_400_assert(domain_owner == owneractor.value, "fio_address", fa.fioaddress,
                               "FIO Domain is not public. Only owner can create FIO Addresses.",
                               ErrorInvalidFioNameFormat);
            }

            uint32_t domain_expiration = domains_iter->expiration;
            uint32_t present_time = now();
            fio_400_assert(present_time <= domain_expiration, "fio_address", fa.fioaddress, "FIO Domain expired",
                           ErrorDomainExpired);

            auto namesbyname = fionames.get_index<"byname"_n>();
            auto fioname_iter = namesbyname.find(nameHash);
            fio_400_assert(fioname_iter == namesbyname.end(), "fio_address", fa.fioaddress,
                           "FIO address already registered", ErrorFioNameAlreadyRegistered);

            expiration_time = get_now_plus_one_year();
            auto key_iter = accountmap.find(actor.value);

            fio_400_assert(key_iter != accountmap.end(), "actor", to_string(actor.value),
                           "Actor is not bound in the account map.", ErrorActorNotInFioAccountMap);

            uint64_t ownerHash = string_to_uint64_hash(key_iter->clientkey.c_str());
            print("OWNER:", actor, "...Value:", actor.value, "...Key:", key_iter->clientkey, "...hash:", ownerHash,
                  "\n");

            uint64_t id = fionames.available_primary_key();
            fionames.emplace(_self, [&](struct fioname &a) {
                a.id = id;
                a.name = fa.fioaddress;
                a.addresses = vector<string>(300, ""); // TODO: Remove prior to production
                a.namehash = nameHash;
                a.domain = fa.fiodomain;
                a.domainhash = domainHash;
                a.expiration = expiration_time;
                a.owner_account = actor.value;
                a.bundleeligiblecountdown = getBundledAmount();
            });

            uint64_t fee_amount = chain_data_update(fa.fioaddress, "FIO", key_iter->clientkey, max_fee, fa, actor,
                                                    true, tpid);

            return expiration_time;
        }

        uint32_t fio_domain_update(const string &fio_domain, const string &owner_fio_public_key, const name &actor,
                                   const FioAddress &fa) {

            uint128_t domainHash = string_to_uint128_hash(fio_domain.c_str());
            uint32_t expiration_time;

            fio_400_assert(fa.domainOnly, "fio_address", fa.fioaddress, "Invalid FIO domain",
                           ErrorInvalidFioNameFormat);

            auto domainsbyname = domains.get_index<"byname"_n>();
            auto domains_iter = domainsbyname.find(domainHash);

            fio_400_assert(domains_iter == domainsbyname.end(), "fio_name", fa.fioaddress,
                           "FIO domain already registered", ErrorDomainAlreadyRegistered);

            expiration_time = get_now_plus_one_year();

            uint64_t id = domains.available_primary_key();

            domains.emplace(_self, [&](struct domain &d) {
                d.id = id;
                d.name = fa.fiodomain;
                d.domainhash = domainHash;
                d.expiration = expiration_time;
                d.account = actor.value;
            });
            return expiration_time;
        }

        uint64_t
        chain_data_update(const string &fioaddress, const string &tokencode,
                          const string &pubaddress, uint64_t max_fee, const FioAddress &fa,
                          const name &actor, const bool isFIO, const string &tpid) {

            uint128_t nameHash = string_to_uint128_hash(fa.fioaddress.c_str());
            uint128_t domainHash = string_to_uint128_hash(fa.fiodomain.c_str());

            auto namesbyname = fionames.get_index<"byname"_n>();
            auto fioname_iter = namesbyname.find(nameHash);
            fio_404_assert(fioname_iter != namesbyname.end(), "FIO Address not found", ErrorFioNameNotRegistered);

            uint32_t name_expiration = fioname_iter->expiration;
            uint32_t present_time = now();

            uint64_t account = fioname_iter->owner_account;
            fio_403_assert(account == actor.value, ErrorSignature);
            fio_400_assert(present_time <= name_expiration, "fio_address", fioaddress,
                           "FIO Address expired", ErrorFioNameExpired);

            auto domainsbyname = domains.get_index<"byname"_n>();
            auto domains_iter = domainsbyname.find(domainHash);

            fio_404_assert(domains_iter != domainsbyname.end(), "FIO Domain not found", ErrorDomainNotFound);

            uint32_t expiration = domains_iter->expiration;
            fio_400_assert(present_time <= expiration, "domain", fa.fiodomain, "FIO Domain expired",
                           ErrorDomainExpired);

            uint64_t chainhash = string_to_uint64_hash(tokencode.c_str());
            auto chain_iter = chains.find(chainhash);

            if (chain_iter == chains.end()) {
                fio_400_assert(tokencode.length() <= 10, "token_code", tokencode, "Invalid token code format",
                               ErrorTokenCodeInvalid);
                fio_400_assert(tokencode.length() > 0, "token_code", tokencode, "Invalid token code format",
                               ErrorTokenCodeInvalid);

                chains.emplace(_self, [&](struct chainList &a) {
                    a.id = chains.available_primary_key();
                    a.chainname = tokencode;
                    a.chainhash = chainhash;
                });
                chain_iter = chains.find(chainhash);
            }

            namesbyname.modify(fioname_iter, _self, [&](struct fioname &a) {
                a.addresses[static_cast<size_t>((chain_iter)->by_index())] = pubaddress;
            });

            //begin new fees, bundle eligible fee logic

            uint128_t endpoint_hash = string_to_uint128_hash("add_pub_address");
            auto fees_by_endpoint = fiofees.get_index<"byendpoint"_n>();
            auto fee_iter = fees_by_endpoint.find(endpoint_hash);

            //if the fee isnt found for the endpoint, then 400 error.

            fio_400_assert(fee_iter != fees_by_endpoint.end(), "endpoint_name", "add_pub_address",
                           "FIO fee not found for endpoint", ErrorNoEndpoint);

            int64_t reg_amount = fee_iter->suf_amount;
            uint64_t fee_type = fee_iter->type;

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

                namesbyname.modify(fioname_iter, _self, [&](struct fioname &a) {
                    a.bundleeligiblecountdown = (bundleeligiblecountdown - 1);
                });
            } else {
                fee_amount = fee_iter->suf_amount;
                fio_400_assert(max_fee >= fee_amount, "max_fee", to_string(max_fee), "Fee exceeds supplied maximum.",
                               ErrorMaxFeeExceeded);

                asset reg_fee_asset;
                //NOTE -- question here, should we always record the transfer for the fees, even when its zero,
                //or should we do as this code does and not do a transaction when the fees are 0.
                reg_fee_asset.symbol = symbol("FIO", 9);
                reg_fee_asset.amount = reg_amount;
                print(reg_fee_asset.amount);

                fio_fees(actor, reg_fee_asset);
                process_rewards(tpid, reg_amount, get_self());
                if (reg_amount > 0) {
                    INLINE_ACTION_SENDER(eosiosystem::system_contract, updatepower)
                            ("eosio"_n, {{_self, "active"_n}},
                             {actor, true}
                            );
                }
            }
            return fee_amount;
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

        [[eosio::action]]
        void
        regaddress(const string &fio_address, const string &owner_fio_public_key, uint64_t max_fee, const name &actor,
                   const string &tpid) {
            name owner_account_name = accountmgnt(actor, owner_fio_public_key);

            FioAddress fa;
            getFioAddressStruct(fio_address, fa);
            register_errors(fa, false);
            name nm = name{owner_account_name};

            uint64_t expiration_time = fio_address_update(actor, nm, max_fee, fa, tpid);

            struct tm timeinfo;
            fioio::convertfiotime(expiration_time, &timeinfo);
            std::string timebuffer = fioio::tmstringformat(timeinfo);

            uint128_t endpoint_hash = string_to_uint128_hash("register_fio_address");

            auto fees_by_endpoint = fiofees.get_index<"byendpoint"_n>();
            auto fee_iter = fees_by_endpoint.find(endpoint_hash);
            fio_400_assert(fee_iter != fees_by_endpoint.end(), "endpoint_name", "register_fio_address",
                           "FIO fee not found for endpoint", ErrorNoEndpoint);

            uint64_t reg_amount = fee_iter->suf_amount;
            uint64_t fee_type = fee_iter->type;

            fio_400_assert(fee_type == 0, "fee_type", to_string(fee_type),
                           "register_fio_address unexpected fee type for endpoint register_fio_address, expected 0",
                           ErrorNoEndpoint);

            fio_400_assert(max_fee >= reg_amount, "max_fee", to_string(max_fee), "Fee exceeds supplied maximum.",
                           ErrorMaxFeeExceeded);

            asset reg_fee_asset;
            reg_fee_asset.symbol = symbol("FIO", 9);
            reg_fee_asset.amount = reg_amount;
            print(reg_fee_asset.amount);

            fio_fees(actor, reg_fee_asset);
            processbucketrewards(tpid, reg_amount, get_self());

            nlohmann::json json = {{"status",        "OK"},
                                   {"expiration",    timebuffer},
                                   {"fee_collected", reg_amount}};
            send_response(json.dump().c_str());
        }

        [[eosio::action]]
        void
        regdomain(const string &fio_domain, const string &owner_fio_public_key,
                  uint64_t max_fee, const name &actor, const string &tpid) {
            name owner_account_name = accountmgnt(actor, owner_fio_public_key);

            FioAddress fa;
            getFioAddressStruct(fio_domain, fa);
            register_errors(fa, true);

            name nm = name{owner_account_name};

            uint32_t expiration_time = fio_domain_update(fio_domain, owner_fio_public_key, actor, fa);

            struct tm timeinfo;
            fioio::convertfiotime(expiration_time, &timeinfo);
            std::string timebuffer = fioio::tmstringformat(timeinfo);

            uint128_t endpoint_hash = string_to_uint128_hash("register_fio_domain");

            auto fees_by_endpoint = fiofees.get_index<"byendpoint"_n>();
            auto fee_iter = fees_by_endpoint.find(endpoint_hash);
            fio_400_assert(fee_iter != fees_by_endpoint.end(), "endpoint_name", "register_fio_domain",
                           "FIO fee not found for endpoint", ErrorNoEndpoint);

            uint64_t reg_amount = fee_iter->suf_amount;
            uint64_t fee_type = fee_iter->type;

            fio_400_assert(fee_type == 0, "fee_type", to_string(fee_type),
                           "register_fio_address unexpected fee type for endpoint register_fio_domain, expected 0",
                           ErrorNoEndpoint);

            fio_400_assert(max_fee >= reg_amount, "max_fee", to_string(max_fee), "Fee exceeds supplied maximum.",
                           ErrorMaxFeeExceeded);

            asset reg_fee_asset;

            reg_fee_asset.symbol = symbol("FIO", 9);
            reg_fee_asset.amount = reg_amount;
            print(reg_fee_asset.amount);

            fio_fees(actor, reg_fee_asset);
            processbucketrewards(tpid, reg_amount, get_self());

            nlohmann::json json = {{"status",        "OK"},
                                   {"expiration",    timebuffer},
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
            FioAddress fa;
            getFioAddressStruct(fio_domain, fa);
            register_errors(fa, true);

            uint128_t domainHash = string_to_uint128_hash(fio_domain.c_str());
            uint32_t expiration_time;

            fio_400_assert(fa.domainOnly, "fio_address", fa.fioaddress, "Invalid FIO domain",
                           ErrorInvalidFioNameFormat);

            auto domainsbyname = domains.get_index<"byname"_n>();
            auto domains_iter = domainsbyname.find(domainHash);

            fio_400_assert(domains_iter != domainsbyname.end(), "fio_domain", fa.fioaddress,
                           "FIO domain not found", ErrorDomainNotRegistered);

            expiration_time = domains_iter->expiration;

            uint128_t endpoint_hash = string_to_uint128_hash("renew_fio_domain");

            auto fees_by_endpoint = fiofees.get_index<"byendpoint"_n>();
            auto fee_iter = fees_by_endpoint.find(endpoint_hash);
            fio_400_assert(fee_iter != fees_by_endpoint.end(), "endpoint_name", "register_fio_domain",
                           "FIO fee not found for endpoint", ErrorNoEndpoint);

            uint64_t reg_amount = fee_iter->suf_amount;
            uint64_t fee_type = fee_iter->type;

            fio_400_assert(fee_type == 0, "fee_type", to_string(fee_type),
                           "register_fio_address unexpected fee type for endpoint register_fio_domain, expected 0",
                           ErrorNoEndpoint);

            fio_400_assert(max_fee >= reg_amount, "max_fee", to_string(max_fee), "Fee exceeds supplied maximum.",
                           ErrorMaxFeeExceeded);

            asset reg_fee_asset;

            reg_fee_asset.symbol = symbol("FIO", 9);
            reg_fee_asset.amount = reg_amount;
            print(reg_fee_asset.amount);

            fio_fees(actor, reg_fee_asset);
            processbucketrewards(tpid, reg_amount, get_self());

            uint64_t new_expiration_time = get_time_plus_one_year(expiration_time);

            struct tm timeinfo;
            fioio::convertfiotime(new_expiration_time, &timeinfo);
            std::string timebuffer = fioio::tmstringformat(timeinfo);

            domainsbyname.modify(domains_iter, _self, [&](struct domain &a) {
                a.expiration = new_expiration_time;
            });

            nlohmann::json json = {{"status",        "OK"},
                                   {"expiration",    timebuffer},
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
            FioAddress fa;
            getFioAddressStruct(fio_domain, fa);
            register_errors(fa, false);

            uint128_t nameHash = string_to_uint128_hash(fa.fioaddress.c_str());
            uint128_t domainHash = string_to_uint128_hash(fa.fiodomain.c_str());

            fio_400_assert(!fa.domainOnly, "fio_address", fa.fioaddress, "Invalid FIO address",
                           ErrorInvalidFioNameFormat);

            auto domainsbyname = domains.get_index<"byname"_n>();
            auto domains_iter = domainsbyname.find(domainHash);

            fio_400_assert(domains_iter != domainsbyname.end(), "fio_address", fa.fioaddress,
                           "FIO Domain not registered",
                           ErrorDomainNotRegistered);

            uint32_t domain_expiration = domains_iter->expiration;
            uint32_t present_time = now();
            fio_400_assert(present_time <= domain_expiration, "fio_address", fa.fioaddress, "FIO Domain expired",
                           ErrorDomainExpired);

            auto namesbyname = fionames.get_index<"byname"_n>();
            auto fioname_iter = namesbyname.find(nameHash);
            fio_400_assert(fioname_iter != namesbyname.end(), "fio_address", fa.fioaddress,
                           "FIO address not registered", ErrorFioNameNotRegistered);

            uint64_t expiration_time = fioname_iter->expiration;
            uint64_t bundleeligiblecountdown = fioname_iter->bundleeligiblecountdown;


            uint128_t endpoint_hash = string_to_uint128_hash("renew_fio_address");

            auto fees_by_endpoint = fiofees.get_index<"byendpoint"_n>();
            auto fee_iter = fees_by_endpoint.find(endpoint_hash);

            fio_400_assert(fee_iter != fees_by_endpoint.end(), "endpoint_name", "register_fio_address",
                           "FIO fee not found for endpoint", ErrorNoEndpoint);

            uint64_t reg_amount = fee_iter->suf_amount;
            uint64_t fee_type = fee_iter->type;

            fio_400_assert(fee_type == 0, "fee_type", to_string(fee_type),
                           "register_fio_address unexpected fee type for endpoint register_fio_address, expected 0",
                           ErrorNoEndpoint);

            fio_400_assert(max_fee >= reg_amount, "max_fee", to_string(max_fee), "Fee exceeds supplied maximum.",
                           ErrorMaxFeeExceeded);

            asset reg_fee_asset;

            reg_fee_asset.symbol = symbol("FIO", 9);
            reg_fee_asset.amount = reg_amount;
            print(reg_fee_asset.amount);
            processbucketrewards(tpid, reg_amount, get_self());

            uint64_t new_expiration_time = get_time_plus_one_year(expiration_time);

            struct tm timeinfo;
            fioio::convertfiotime(new_expiration_time, &timeinfo);
            std::string timebuffer = fioio::tmstringformat(timeinfo);

            namesbyname.modify(fioname_iter, _self, [&](struct fioname &a) {
                a.expiration = new_expiration_time;
                a.bundleeligiblecountdown = getBundledAmount() + bundleeligiblecountdown;
            });

            nlohmann::json json = {{"status",        "OK"},
                                   {"expiration",    timebuffer},
                                   {"fee_collected", reg_amount}};

            send_response(json.dump().c_str());
        }

        /*
         * TESTING ONLY, REMOVE for main net launch!
         * This action will create a domain of the specified name and the domain will be
         * expired.
         */
        void expdomain(const name &actor, const string &domain) {
            uint128_t domainHash = string_to_uint128_hash(domain.c_str());
            uint64_t expiration_time = get_now_minus_years(5);

            auto domainsbyname = domains.get_index<"byname"_n>();
            auto iter4 = domainsbyname.find(domainHash);

            if (iter4 == domainsbyname.end()) {
                uint64_t id = domains.available_primary_key();
                domains.emplace(_self, [&](struct domain &d) {
                    d.id = id;
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
        void expaddresses(const name &actor, const string &domain, const string &address_prefix,
                          uint64_t number_addresses_to_add) {

            uint128_t nameHash;
            uint64_t domainHash = string_to_uint128_hash(domain.c_str());
            uint64_t expiration_time = get_now_minus_years(1);

            int countAdded = 0;
            for (int i = 0; i < 10000; i++) {

                string name;
                if (i == 0) {
                    name = address_prefix + ":" + domain;
                } else {
                    name = address_prefix + to_string(i * 2) + ":" + domain;
                }

                int yearsago = 1;
                if (i > 7) {
                    yearsago = i / 7;
                }

                expiration_time = get_now_minus_years(yearsago);
                nameHash = string_to_uint128_hash(name.c_str());
                auto namesbyname = fionames.get_index<"byname"_n>();
                auto iter1 = namesbyname.find(nameHash);
                if (iter1 == namesbyname.end()) {
                    uint64_t id = fionames.available_primary_key();

                    fionames.emplace(_self, [&](struct fioname &a) {
                        a.id = id;
                        a.name = name;
                        a.addresses = vector<string>(20, ""); // TODO: Remove prior to production
                        a.namehash = nameHash;
                        a.domain = domain;
                        a.domainhash = domainHash;
                        a.expiration = expiration_time;
                        a.owner_account = actor.value;
                        a.bundleeligiblecountdown = getBundledAmount();
                    });
                    countAdded++;
                }

                if (countAdded == number_addresses_to_add) {
                    print("created ", countAdded, " in the domain ", domain, "\n");
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

            std::vector<uint128_t> burnlist;
            std::vector<uint128_t> domainburnlist;

            int numbertoburn = 100;
            int windowmaxyears = 20;
            uint64_t secondsperday = 86400;
            uint64_t domainwaitforburndays = 90 * secondsperday;
            uint64_t addresswaitforburndays = 90 * secondsperday;
            uint64_t nowtime = now();
            uint32_t minexpiration = get_now_minus_years(windowmaxyears);

            //using this instead of now time will place everything in the to be burned list, for testing only.
            uint64_t kludgedNow = get_now_plus_years(10); // This is for testing only

            auto domainexpidx = domains.get_index<"byexpiration"_n>();
            auto domainiter = domainexpidx.lower_bound(minexpiration);

            while (domainiter != domainexpidx.end()) {
                uint64_t expire = domainiter->expiration;
                uint128_t domainnamehash = domainiter->domainhash;

                if ((expire + domainwaitforburndays) > nowtime){
                    break;
                } else {
                    auto domainhash = domainiter->domainhash;
                    auto fionamesbydomainhashidx = fionames.get_index<"bydomain"_n>();
                    auto nmiter = fionamesbydomainhashidx.lower_bound(domainhash);
                    bool processed_all_in_domain = false;

                    while (nmiter != fionamesbydomainhashidx.end()) {
                        if (nmiter->domainhash == domainhash) {
                            burnlist.push_back(nmiter->namehash);

                            if (burnlist.size() >= numbertoburn) {
                                break;
                            }
                            nmiter++;
                        } else {
                            processed_all_in_domain = true;
                            break;
                        }
                    }

                    if (processed_all_in_domain) {
                        //print(" adding domain to burn list ",domainiter->name," expiration ",domainiter->expiration,"\n");
                        //print(" adding domain to domain burn list", domainnamehash, "\n");
                        domainburnlist.push_back(domainnamehash);
                    }
                    if (burnlist.size() >= numbertoburn) {
                        break;
                    }
                }
                domainiter++;
            }

            if (burnlist.size() < numbertoburn) {

                auto nameexpidx = fionames.get_index<"byexpiration"_n>();
                auto nameiter = nameexpidx.lower_bound(minexpiration);

                while (nameiter != nameexpidx.end()) {
                    uint64_t expire = nameiter->expiration;
                    if ((expire + addresswaitforburndays) > nowtime){
                        break;
                    } else {
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
                uint128_t burner = burnlist[i];
                vector <uint64_t> ids;

                auto namesbyname = fionames.get_index<"byname"_n>();
                auto fionamesiter = namesbyname.find(burner);
                if (fionamesiter != namesbyname.end()) {
                    namesbyname.erase(fionamesiter);
                }
            }

            for (int i = 0; i < domainburnlist.size(); i++) {
                uint128_t burner = domainburnlist[i];

                auto domainsbyname = domains.get_index<"byname"_n>();
                auto domainsiter = domainsbyname.find(burner);

                if (domainsiter != domainsbyname.end()) {
                    domainsbyname.erase(domainsiter);
                }
            }

            nlohmann::json json = {{"status",       "OK"},
                                   {"items_burned", (burnlist.size() + domainburnlist.size())}
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
                   const name &actor, const string &tpid) {

            FioAddress fa;
            getFioAddressStruct(fio_address, fa);
            addaddress_errors(token_code, public_address, fa);

            uint64_t fee_amount = chain_data_update(fio_address, token_code, public_address, max_fee, fa, actor, false,
                                                    tpid);

            nlohmann::json json = {{"status",        "OK"},
                                   {"fee_collected", fee_amount}};
            send_response(json.dump().c_str());
        } //addaddress

        [[eosio::action]]
        void
        setdomainpub(const string &fio_domain, const uint8_t is_public, uint64_t max_fee, const name &actor,
                     const string &tpid) {

            FioAddress fa;
            uint32_t present_time = now();
            getFioAddressStruct(fio_domain, fa);
            register_errors(fa, true);

            uint128_t domainHash = string_to_uint128_hash(fio_domain.c_str());
            auto domainsbyname = domains.get_index<"byname"_n>();
            auto domain_iter = domainsbyname.find(domainHash);

            fio_400_assert(domain_iter != domainsbyname.end(), "fio_domain", fa.fioaddress, "Invalid FIO domain",
                           ErrorDomainNotRegistered);
            fio_400_assert(fa.domainOnly, "fio_domain", fa.fioaddress, "Invalid FIO domain",
                           ErrorInvalidFioNameFormat);

            uint64_t expiration = domain_iter->expiration;
            fio_400_assert(present_time <= expiration, "fio_domain", fa.fiodomain, "FIO Domain expired",
                           ErrorDomainExpired);

            domainsbyname.modify(domain_iter, _self, [&](struct domain &a) {
                a.is_public = is_public;
            });

            uint128_t endpoint_hash = string_to_uint128_hash("set_fio_domain_public");

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
            fio_fees(actor, reg_fee_asset);
            process_rewards(tpid, reg_amount, get_self());
            if (reg_amount > 0) {
                //MAS-522 remove staking from voting.
                INLINE_ACTION_SENDER(eosiosystem::system_contract, updatepower)
                        ("eosio"_n, {{_self, "active"_n}},
                         {actor, true}
                        );
            }

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
                    p.keyhash = string_to_uint128_hash(client_key.c_str());
                });
            }
            print("bind of account is done processing", "\n");
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
            uint128_t hashval = string_to_uint128_hash(tstr.c_str());

            auto namesbyname = fionames.get_index<"byname"_n>();
            auto fioname_iter = namesbyname.find(hashval);
            fio_400_assert(fioname_iter != namesbyname.end(), "fio_address", fio_address,
                           "FIO address not registered", ErrorFioNameAlreadyRegistered);

            uint64_t bundleeligiblecountdown = fioname_iter->bundleeligiblecountdown;

            if (bundleeligiblecountdown > 0) {
                namesbyname.modify(fioname_iter, _self, [&](struct fioname &a) {
                    a.bundleeligiblecountdown = (bundleeligiblecountdown - 1);
                });
            }
        }
    };

    EOSIO_DISPATCH(FioNameLookup, (regaddress)(addaddress)(regdomain)(renewdomain)(renewaddress)(expdomain)(
            expaddresses)(setdomainpub)(burnexpired)(removename)(removedomain)(rmvaddress)(decrcounter)
    (bind2eosio))
}
