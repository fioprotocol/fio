/** FioName Token implementation file
 *  Description: FioName smart contract allows issuance of unique domains and names for easy public address resolution
 *  @author Adam Androulidakis
 *  @file fio.name.hpp
 *  @copyright Dapix
 *
 *  Changes:
 *  Adam Androulidakis 12-10-2018 - MAS-114
 *  Adam Androulidakis 9-18-2018
 *  Adam Androulidakis 9-6-2018
 *  Ciju John 9-5-2018
 *  Adam Androulidakis  8-31-2018
 *  Ciju John  8-30-2018
 *  Adam Androulidakis  8-29-2018
 *  Ed Rotthoff 10-26-2018
 *  Phil Mesnier 12-26-2018
 *  Casey Gardiner 3/5/2019
 *  Phil Mesnier 3/5/2019
 */

#include <eosiolib/asset.hpp>
#include "fio.name.hpp"
#include <fio.common/fio.common.hpp>
#include <fio.common/json.hpp>
#include <eosio/chain/fioio/fioerror.hpp>
#include <eosio/chain/fioio/fio_common_validator.hpp>
#include <eosio/chain/fioio/chain_control.hpp>
#include <climits>
#include <eosiolib/types.hpp>

namespace fioio{

    class FioNameLookup : public contract {
        private:
        domains_table domains;
        chains_table chains;
        fionames_table fionames;
        keynames_table keynames;
        trxfees_singleton trxfees;
        fiopubs_table fiopubs;
        eosio_names_table eosionames;
        config appConfig;

        const account_name TokenContract = eosio::string_to_name(TOKEN_CONTRACT);

    public:
        FioNameLookup(account_name self)
           : contract(self), domains(self, self), fionames(self, self), keynames(self, self), trxfees(FeeContract,FeeContract), fiopubs(self, self), eosionames(self, self), chains(self, self)
        {
            configs_singleton configsSingleton(FeeContract,FeeContract);
            appConfig = configsSingleton.get_or_default(config());
        }

        [[eosio::action]]
        void registername(const string &fioname, const account_name &actor) {

            require_auth(actor); // check for requestor authority; required for fee transfer

            // Split the fio name and domain portions
            FioAddress fa;
            getFioAddressStruct(fioname, fa);

            print ("fioname = ", fioname, " fa = ", fa.fiopubaddress, "\n");
            int res = fa.domainOnly ? isFioNameValid(fa.fiodomain)*10 : isFioNameValid(fa.fioname);
            print("fioname: ", fa.fioname, ", Domain: ", fa.fiodomain, " error code = ", res, "\n");

            string value = fa.fiopubaddress + " error = " + to_string(res);
            fio_400_assert(res == 0, "fio_name", value, "Invalid FIO name format", ErrorInvalidFioNameFormat);

            uint64_t domainHash = ::eosio::string_to_uint64_t(fa.fiodomain.c_str());
            asset registerFee;
            string registerMemo;
            auto fees = trxfees.get_or_default(trxfee());
            uint32_t expiration_time = 0;

            if (fa.domainOnly) { // domain register
                // check for domain availability
                auto domains_iter = domains.find(domainHash);
                fio_400_assert(domains_iter == domains.end(), "fio_name", fa.fiopubaddress, "FIO domain already registered", ErrorDomainAlreadyRegistered);
                // check if callee has requisite dapix funds.

                //get the expiration for this new domain.
                expiration_time = get_now_plus_one_year();

                // Issue, create and transfer nft domain token
                // Add domain entry in domain table
                domains.emplace(_self, [&](struct domain &d) {
                    d.name = fa.fiodomain;
                    d.domainhash = domainHash;
                    d.expiration = expiration_time;
                });
                // Add domain entry in domain table

                registerFee = fees.domregiter;
            } else { // fioname register

// check if domain exists.
                auto domains_iter = domains.find(domainHash);
                fio_400_assert(domains_iter != domains.end(), "fio_name", fa.fiopubaddress, "FIO Domain not registered", ErrorDomainNotRegistered);


                // TODO check if domain permission is valid.

                //check if the domain is expired.
                uint32_t domain_expiration = domains_iter->expiration;
                uint32_t present_time = now();
                fio_400_assert(present_time <= domain_expiration,"fio_name", fa.fiopubaddress, "FIO Domain expired", ErrorDomainExpired);

                // check if fioname is available
                uint64_t nameHash = ::eosio::string_to_uint64_t(fa.fiopubaddress.c_str());
                print("Name hash: ", nameHash, ", Domain has: ", domainHash, "\n");
                auto fioname_iter = fionames.find(nameHash);
                fio_400_assert(fioname_iter == fionames.end(), "fio_name", fa.fiopubaddress,"FIO name already registered", ErrorFioNameAlreadyRegistered);

                //set the expiration on this new fioname
                expiration_time = get_now_plus_one_year();

                // check if callee has requisite dapix funds.
                // DO SOMETHING

                // Issue, create and transfer fioname token
                // DO SOMETHING

                // Add fioname entry in fionames table
                fionames.emplace(_self, [&](struct fioname &a){
                    a.name = fa.fiopubaddress;
                    a.namehash = nameHash;
                    a.domain = fa.fiodomain;
                    a.domainhash = domainHash;
                    a.expiration = expiration_time;
                    a.addresses[1] = fa.fiopubaddress;
                });

                registerFee = fees.nameregister;
            } // else



            if (appConfig.pmtson) {
                // collect fees
                // check for funds is implicitly done as part of the funds transfer.
                print("Collecting registration fees: ", registerFee);
                action(permission_level{actor, N(active)},
                       TokenContract, N(transfer),
                       make_tuple(actor, _self, registerFee,
                                  string("Registration fees. Thank you."))
                ).send();
            }
            else {
                print("Payments currently disabled.");
            }

            nlohmann::json json = {{"status","OK"},{"fio_name",fa.fiopubaddress},{"expiration",expiration_time}};
            send_response(json.dump().c_str());
        }

        /***
         * This method will return now plus one year.
         * the result is the present block time, which is number of seconds since 1970
         * incremented by secondss per year.
         */
        inline uint32_t get_now_plus_one_year() {
            //set the expiration for this domain.
            //get the blockchain now time, time in seconds since 1970
            //add number of seconds in a year to this to get the expiration.
            uint32_t present_time = now();
            uint32_t incremented_time = present_time + DAYTOSECONDS;
            return incremented_time;
        }

        /***
         * Given a fio user name, chain name and chain specific address will attach address to the user's FIO fioname.
         *
         * @param fioaddress The FIO user name e.g. "adam.fio"
         * @param tokencode The chain name e.g. "btc"
         * @param pubaddress The chain specific user address
         */
        [[eosio::action]]
        void addaddress(const string &fioaddress, const string &tokencode, const string &pubaddress, const account_name &actor) {
            fio_400_assert(!fioaddress.empty(), "fioaddress", fioaddress, "FIO address cannot be empty..", ErrorDomainAlreadyRegistered);
            fio_400_assert(!tokencode.empty(), "tokencode", tokencode, "Chain cannot be empty..", ErrorChainEmpty);

            // Chain input validation
            fio_400_assert(!pubaddress.empty(), "pubaddress", pubaddress, "Chain address cannot be empty..", ErrorChainAddressEmpty);
            fio_400_assert(pubaddress.find(" "), "pubaddress", pubaddress, "Chain address cannot contain whitespace..", ErrorChainContainsWhiteSpace);

            string my_chain = tokencode;

            fio_400_assert(isChainNameValid(my_chain), "tokencode", tokencode, "Invalid chain format", ErrorInvalidFioNameFormat);

            uint64_t chainhash = ::eosio::string_to_uint64_t(my_chain.c_str());
            auto chain_iter = chains.find(chainhash);

            uint64_t next_idx = (chains.begin() == chains.end() ? 0 : (chain_iter)->id + 1 );

            if( chain_iter == chains.end() ){
                chains.emplace(_self, [&](struct chainList &a){
                    a.id = next_idx;
                    a.chainname = my_chain;
                    a.chainhash = chainhash;
                });
                chain_iter = chains.find(chainhash);
            }

            // Check to see what index to store the address
            //int chainIndex = approvedTokens.getIndexFromChain(my_chain);
            //fio_400_assert(chainIndex != -1, "tokencode", tokencode, "Invalid chain name", ErrorInvalidFioNameFormat);

            // validate fio FIO Address exists
            uint64_t nameHash = ::eosio::string_to_uint64_t(fioaddress.c_str());
            //print("Name: ", fio_address, ", nameHash: ", nameHash, "..");
            auto fioname_iter = fionames.find(nameHash);

            fio_404_assert(fioname_iter != fionames.end(), "FIO Address not registered..", ErrorFioNameNotRegistered);

            //check that the name is not expired
            uint32_t name_expiration = fioname_iter->expiration;
            uint32_t present_time = now();

            //print("name_expiration: ", name_expiration, ", present_time: ", present_time, "\n");
            fio_400_assert(present_time <= name_expiration, "fioaddress", fioaddress, "FIO Address is expired.", ErrorFioNameExpired);

            //parse the domain and check that the domain is not expired.
            string domain = nullptr;
            size_t pos = fioaddress.find('.');

            if (pos == string::npos) {                                          // TODO Refactor since its used so much
                eosio_assert(true,"could not find domain name in fio name.");
            } else {
                domain = fioaddress.substr(pos + 1, string::npos);
            }

            uint64_t domainHash = ::eosio::string_to_uint64_t(domain.c_str());

            auto domains_iter = domains.find(domainHash);
            fio_404_assert(domains_iter != domains.end(), "FIO Domain not yet register ed.", ErrorDomainNotRegistered);

            uint32_t expiration = domains_iter->expiration;
            fio_400_assert(present_time <= expiration, "domain", domain, "FIO Domain is expired.", ErrorDomainExpired);

            // insert/update <chain, address> pair
            fionames.modify(fioname_iter, _self, [&](struct fioname &a) {
                a.addresses[(chain_iter)->by_index()] = pubaddress;
            });

            // insert/update key into key-name table for reverse lookup
            auto idx = keynames.get_index<N(bykey)>();
            auto keyhash = ::eosio::string_to_uint64_t(pubaddress.c_str());
            auto matchingItem = idx.lower_bound(keyhash);

            // Advance to the first entry matching the specified address and chain
            while (matchingItem != idx.end() && matchingItem->keyhash == keyhash ) {
                matchingItem++;
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

            if (appConfig.pmtson) {
                // collect fees; Check for funds is implicitly done as part of the funds transfer.
                auto fees = trxfees.get_or_default(trxfee());
                asset registerFee = fees.upaddress;
                //print("Collecting registration fees: ", registerFee);
                action(permission_level{actor, N(active)},
                       TokenContract, N(transfer),
                       make_tuple(actor, _self, registerFee,
                                  string("Registration fees. Thank you."))
                ).send();
            }
            //else {
            //print("Payments currently disabled.");
            //}
            nlohmann::json json = {{"status","OK"},{"fioaddress",fioaddress},{"tokencode",tokencode},{"pubaddress",pubaddress},{"actor",actor}};
            send_response(json.dump().c_str());
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

    /***
     * The provided Base58 encoded public address and public key will be listed directly
     * into the fiopubs table.
     *
     * @param pub_address The Base58 Encoded FIO Public Address
     * @param pub_key The public key to be indexed with the FIO Public Address
     */
    [[eosio::action]]
    void addfiopubadd(const string &pub_address, const string &pub_key) {

      eosio_assert_message_code(!pub_address.empty(), "Public Address field cannot be empty", ErrorPubAddressEmpty);
      eosio_assert_message_code(!pub_key.empty(), "Public Key field cannot be empty", ErrorPubKeyEmpty);

      // The caller of this contract must have the private key in their wallet for the FIO.SYSTEM account
       require_auth(::eosio::string_to_name(FIO_SYSTEM));

       string pub = pub_address;
       string key = pub_key;
       //The indexes need to be calculated correctly here.
       //Public Key and Public Address must be hashed uniquely to 12 characters to represent indexes.
       uint64_t fiopubindex = string_to_uint64_t(pub.c_str());
       uint64_t pubkeyindex = string_to_uint64_t(key.c_str());

       ///////////////////////////////////////////////////
       //To do: Keep emplacing new entries or replace new ?

       auto fiopubadd_iter = fiopubs.find(fiopubindex);
       if (fiopubadd_iter != fiopubs.end()) {
          print("Found pub address: pub address: ", fiopubadd_iter->fiopub, ", pub key: ", fiopubadd_iter->pubkey, "\n");
          print("Found pub address: n pub address hash: ", fiopubindex, ",n pub key hash: ", pubkeyindex, "o pub address hash: ", fiopubadd_iter->fiopubindex, ",o pub key hash: ", fiopubadd_iter->pubkeyindex, "\n");
       } else {
         print("No pub address match found.");
       }
       eosio_assert_message_code(fiopubadd_iter == fiopubs.end(), "FIO Public address exists.", ErrorPubAddressExist);

       fiopubs.emplace(_self, [&](struct fiopubaddr &f) {
                                 f.fiopub = pub_address; // The public address
                                 f.pubkey = pub_key; // The public Key
                                 f.fiopubindex = fiopubindex; // The index of the public address
                                 f.pubkeyindex = pubkeyindex; // The index of the public key
                              });

       // json response
       nlohmann::json json = {{"status","OK"},{"pub_address",pub_address},{"pub_key",pub_key}};
       send_response(json.dump().c_str());
    } // addfiopubadd

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
   void bind2eosio (name account, const string& client_key, bool existing) {
    // The caller of this contract must have the private key in their wallet for the FIO.SYSTEM account
      require_auth(::eosio::string_to_name(FIO_SYSTEM));

      auto other = eosionames.find(account);
      if (other != eosionames.end()) {
         eosio_assert_message_code(existing && client_key == other->clientkey, "EOSIO account already bound", ErrorPubAddressExist);
            // name in the table and it matches
      } else {
         eosio_assert_message_code(!existing, "existing EOSIO account not bound to a key", ErrorPubAddressExist);
         eosionames.emplace(_self, [&](struct eosio_name &p) {
                                      p.account = account;
                                      p.clientkey = client_key;
                                   });
      }
   }

    }; // class FioNameLookup

   EOSIO_ABI( FioNameLookup, (registername)(addaddress)(removename)(removedomain)(rmvaddress)(addfiopubadd)(bind2eosio))
}
