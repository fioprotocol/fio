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
 */

#include <eosiolib/asset.hpp>
#include "fio.name.hpp"
#include <fio.common/fio.common.hpp>
#include <fio.common/json.hpp>
#include <eosio/chain/fioio/fioerror.hpp>
#include <climits>

namespace fioio{

    class FioNameLookup : public contract {
        private:
        domains_table domains;
        fionames_table fionames;
        keynames_table keynames;
        trxfees_singleton trxfees;
        fiopubs_table fiopubs;
        config appConfig;

        chaintable chainlist;
        int chainlistsize;

        const account_name TokenContract = eosio::string_to_name(TOKEN_CONTRACT);

    public:
        FioNameLookup(account_name self)
        : contract(self), domains(self, self), fionames(self, self), keynames(self, self), fiopubs(self, self), chainlist(self, self),
            trxfees(FeeContract,FeeContract)
        {
            configs_singleton configsSingleton(FeeContract,FeeContract);
            appConfig = configsSingleton.get_or_default(config());
        }


        [[eosio::action]]
        void registername(const string &name, const account_name &requestor) {
            require_auth(requestor); // check for requestor authority; required for fee transfer

            string newname = name;

	      		// make fioname lowercase before hashing
			      transform(newname.begin(), newname.end(), newname.begin(), ::tolower);

            //parse the domain from the name.
            string domain = nullptr;
            string fioname = domain;

            size_t pos = newname.find('.');
            if (pos == string::npos) {
                domain = name;
            } else {
                fioname = name.substr(0, pos);
                domain = name.substr(pos + 1, string::npos);
            }

            print("fioname: ", fioname, ", Domain: ", domain, "\n");

            uint64_t domainHash = ::eosio::string_to_uint64_t(domain.c_str());
            asset registerFee;
            string registerMemo;
            auto fees = trxfees.get_or_default(trxfee());
            uint32_t expiration_time = 0;

            if (fioname.empty()) { // domain register
                // check for domain availability
                auto domains_iter = domains.find(domainHash);
                fio_400_assert(domains_iter == domains.end(), "name", name, "already registered", ErrorDomainAlreadyRegistered);
                // check if callee has requisite dapix funds.

                //get the expiration for this new domain.
                expiration_time = get_now_plus_one_year();

                // Issue, create and transfer nft domain token
                // Add domain entry in domain table
                domains.emplace(_self, [&](struct domain &d) {
                    d.name = domain;
                    d.domainhash = domainHash;
                    d.expiration = expiration_time;
                });
                // Add domain entry in domain table

                registerFee = fees.domregiter;
            } else { // fioname register

				// check if domain exists.
                auto domains_iter = domains.find(domainHash);
                fio_400_assert(domains_iter != domains.end(), "name", name, "Domain not yet registered.", ErrorDomainNotRegistered);

                // TODO check if domain permission is valid.

                //check if the domain is expired.
                uint32_t domain_expiration = domains_iter->expiration;
                uint32_t present_time = now();
                eosio_assert_message_code (present_time <= domain_expiration,"Domain has expired.", ErrorDomainExpired);

                // check if fioname is available
                uint64_t nameHash = ::eosio::string_to_uint64_t(newname.c_str());
                print("Name hash: ", nameHash, ", Domain has: ", domainHash, "\n");
                auto fioname_iter = fionames.find(nameHash);
                eosio_assert_message_code(fioname_iter == fionames.end(), "Fioname is already registered.", ErrorFioNameAlreadyRegistered);

                //set the expiration on this new fioname
                expiration_time = get_now_plus_one_year();

                // check if callee has requisite dapix funds.
                // DO SOMETHING

                // Issue, create and transfer fioname token
                // DO SOMETHING

                // Add fioname entry in fionames table
                fionames.emplace(_self, [&](struct fioname &a){
                    a.name = newname;
                    a.namehash = nameHash;
                    a.domain = domain;
                    a.domainhash = domainHash;
                    a.expiration = expiration_time;
                    a.addresses = vector<string>(chainlistsize , "");
                });

                registerFee = fees.nameregister;
            } // else

            if (appConfig.pmtson) {
                // collect fees
                // check for funds is implicitly done as part of the funds transfer.
                print("Collecting registration fees: ", registerFee);
                action(permission_level{requestor, N(active)},
                       TokenContract, N(transfer),
                       make_tuple(requestor, _self, registerFee,
                                  string("Registration fees. Thank you."))
                ).send();
            }
            else {
                print("Payments currently disabled.");
            }

            nlohmann::json json = {{"status","OK"},{"fio_name",newname},{"expiration",expiration_time}};
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
         * @param fio_address The FIO user name e.g. "adam.fio"
         * @param chain The chain name e.g. "btc"
         * @param address The chain specific user address
         */
        [[eosio::action]]
        void addaddress(const string &fio_address, const string &chain, const string &pub_address, const account_name &requestor) {

            fio_400_assert(!fio_address.empty(), "fio_address", fio_address, "FIO address cannot be empty..", ErrorDomainAlreadyRegistered);
            fio_400_assert(!chain.empty(), "chain", chain, "Chain cannot be empty..", ErrorChainEmpty);

            // Chain input validation
            fio_400_assert(!pub_address.empty(), "pub_address", pub_address, "Chain address cannot be empty..", ErrorChainAddressEmpty);
            fio_400_assert(pub_address.find(" "), "pub_address", pub_address, "Chain address cannot contain whitespace..", ErrorChainContainsWhiteSpace);

            string my_chain = chain;

            transform(my_chain.begin(), my_chain.end(), my_chain.begin(), ::toupper);

            if(my_chain.find_first_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ") != std::string::npos) {
                fio_400_assert(false, "chain", chain, "Invalid chain format", ErrorInvalidFioNameFormat);
            }

            uint64_t chainHash = ::eosio::string_to_uint64_t(my_chain.c_str());
            auto chainhash = ::eosio::string_to_uint64_t(my_chain.c_str());
            auto chain_iter = chainlist.find(chainHash);

            uint64_t next_idx = (chainlist.begin() == chainlist.end() ? 0 : (chain_iter--)->index + 1);

            if( chain_iter == chainlist.end() ){
                chainlist.emplace(_self, [&](struct chainpair &a){
                    a.index = next_idx;
                    a.chainname = chain;
                    a.chainhash = chainhash;
                });
            }

            //Chain List Size Update w/ size checking
            if ( next_idx > chainlistsize ){
                chainlistsize = next_idx;
            }

            // validate fio FIO Address exists
            uint64_t nameHash = ::eosio::string_to_uint64_t(fio_address.c_str());
            //print("Name: ", fio_address, ", nameHash: ", nameHash, "..");
            auto fioname_iter = fionames.find(nameHash);

            fio_404_assert(fioname_iter != fionames.end(), "FIO Address not registered..", ErrorFioNameNotRegistered);

            //check that the name is not expired
            uint32_t name_expiration = fioname_iter->expiration;
            uint32_t present_time = now();

            //print("name_expiration: ", name_expiration, ", present_time: ", present_time, "\n");
            fio_400_assert(present_time <= name_expiration, "fio_address", fio_address, "FIO Address is expired.", ErrorFioNameExpired);

            //parse the domain and check that the domain is not expired.
            string domain = nullptr;
            size_t pos = fio_address.find('.');

            if (pos == string::npos) {                                          // TODO Refactor since its used so much
                eosio_assert(true,"could not find domain name in fio name.");
            } else {
                domain = fio_address.substr(pos + 1, string::npos);
            }

            uint64_t domainHash = ::eosio::string_to_uint64_t(domain.c_str());
            //print("Domain: ", domain, ", domainHash: ", domainHash, "..");

            auto domains_iter = domains.find(domainHash);
            fio_404_assert(domains_iter != domains.end(), "FIO Domain not yet registered.", ErrorDomainNotRegistered);

            uint32_t expiration = domains_iter->expiration;
            fio_400_assert(present_time <= expiration, "domain", domain, "FIO Domain is expired.", ErrorDomainExpired);

            // insert/update <chain, address> pair
            fionames.modify(fioname_iter, _self, [&](struct fioname &a) {
                a.addresses[static_cast<size_t>(next_idx)] = fio_address;
            });

            // insert/update key into key-name table for reverse lookup
            auto idx = keynames.get_index<N(bykey)>();
            auto keyhash = ::eosio::string_to_uint64_t(pub_address.c_str());
            auto matchingItem = idx.lower_bound(keyhash);

            // Advance to the first entry matching the specified address and chain
            //while (matchingItem != idx.end() && matchingItem->keyhash == keyhash  && matchingItem->chaintype != static_cast<uint64_t>(c_type)) {
            //    matchingItem++;
            //}

            if (matchingItem == idx.end() || matchingItem->keyhash != keyhash) {
                keynames.emplace(_self, [&](struct key_name &k) {
                    k.id = keynames.available_primary_key();        // use next available primary key
                    k.key = pub_address;                            // persist key
                    k.keyhash = keyhash;                            // persist key hash
                    k.chaintype = static_cast<uint64_t>(next_idx);  // specific chain type
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
                action(permission_level{requestor, N(active)},
                       TokenContract, N(transfer),
                       make_tuple(requestor, _self, registerFee,
                                  string("Registration fees. Thank you."))
                ).send();
            }
            //else {
            //print("Payments currently disabled.");
            //}
            nlohmann::json json = {{"status","OK"},{"fio_address",fio_address},{"chain",chain},{"pub_address",pub_address}};
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
      eosio_assert_message_code(fiopubadd_iter == fiopubs.end(), "FIO Public address exists.", ErrorPubAddressExist);

      fiopubs.emplace(_self, [&](struct fiopubaddr &f) {
        f.fiopub = pub_address; // The public address
        f.pubkey = pub_key; // The public Key
        f.fiopubindex = fiopubindex; // The index of the public address
        f.pubkeyindex = pubkeyindex; // The index of the public key
      });

      // Test json response
      nlohmann::json json = {{"status","OK"},{"[pub_address]",pub_address},{"pub_key",pub_key}};
      send_response(json.dump().c_str());

    } // addfiopubadd

    }; // class FioNameLookup




    EOSIO_ABI( FioNameLookup, (registername)(addaddress)(removename)(removedomain)(rmvaddress)(addfiopubadd) )
}
