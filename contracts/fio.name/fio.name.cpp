/** FioName Token implementation file
 *  Description: FioName smart contract allows issuance of unique domains and names for easy public address resolution
 *  @author Adam Androulidakis
 *  @file fio.name.hpp
 *  @copyright Dapix
 */

#include <eosiolib/asset.hpp>
#include "fio.name.hpp"
#include <fio.common/fio.common.hpp>
#include <fio.common/json.hpp>

#include <climits>

namespace fioio{

    // error codes
    static const uint64_t
            ErrorDomainAlreadyRegistered =   100,   // Domain is already registered.
            ErrorDomainNotRegistered =       101,   // Domain not yet registered.
            ErrorFioNameAlreadyRegistered =  102,   // Fioname is already registered.
            ErrorFioNameEmpty =              103,   // FIO user name is empty.
            ErrorChainEmpty =                104,   // Chain name is empty.
            ErrorChainAddressEmpty =         105,   // Chain address is empty.
            ErrorChainContainsWhiteSpace =   106,   // Chain address contains whitespace.
            ErrorChainNotSupported =         107,   // Chain isn't supported.
            ErrorFioNameNotRegistered =      108,   // Fioname not yet registered.
            ErrorDomainExpired =             109,   // Fioname not yet registered.
            ErrorFioNameExpired =            110,   // Fioname not yet registered.
            ErrorFioPubKeyEmpty =            111;   // FIO Public Key is empty.


    class FioNameLookup : public contract { 
        private:
        domains_table domains;
        fionames_table fionames;
        keynames_table keynames;
		trxfees_singleton trxfees;
		chainList chainlist;
		int chainlistsize;
        config appConfig;

        const account_name TokenContract = eosio::string_to_name(TOKEN_CONTRACT);

    public:
        FioNameLookup(account_name self)
        : contract(self), domains(self, self), fionames(self, self), keynames(self, self),
            trxfees(FeeContract,FeeContract), chainlist(self, self)
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
                eosio_assert_message_code(domains_iter == domains.end(), "Domain is already registered.", ErrorDomainAlreadyRegistered);
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
                eosio_assert_message_code(domains_iter != domains.end(), "Domain not yet registered.", ErrorDomainNotRegistered);
                
                // TODO check if domain permission is valid.
                
                //check if the domain is expired.
                uint32_t domain_expiration = domains_iter->expiration;
                uint32_t present_time = now();
                eosio_assert(present_time <= domain_expiration,"Domain has expired.");
                
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
                    a.addresses = vector<string>(chainlistsize , ""); // TODO Max size now?
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
            uint32_t incremented_time = present_time + 31561920;
            return incremented_time;
        }

        /***
         * Given a fio user name, chain name and chain specific address will attach address to the user's FIO fioname.
         *
         * @param fio_address The FIO user name e.g. "adam.fio"
         * @param chain The chain name e.g. "btc"
         * @param pub_address The chain specific user address
         * @param pub_address The FIO public key of owner. Has to match signature.
         */
        [[eosio::action]]
        void addaddress(const string &fio_address, const string &chain, const string &pub_address) {
            // TODO 400 Responses
            eosio_assert_message_code(!fio_address.empty(), "FIO user name cannot be empty..", ErrorFioNameEmpty);
            eosio_assert_message_code(!chain.empty(), "Chain cannot be empty..", ErrorChainEmpty);
            eosio_assert_message_code(!pub_address.empty(), "Chain address cannot be empty..", ErrorChainAddressEmpty);

            // Verify the address does not have a whitespace
            eosio_assert_message_code(pub_address.find(" "), "Chain address cannot contain whitespace..", ErrorChainContainsWhiteSpace);

            // TODO MAS-73 Chain input validation 400 Responses

            string my_chain = chain;
            transform(my_chain.begin(), my_chain.end(), my_chain.begin(), ::toupper);
            uint64_t chainHash = ::eosio::string_to_uint64_t(my_chain.c_str());
            auto chain_iter = chainlist.find(chainHash);

            uint64_t next_idx = (chainlist.begin() == chainlist.end() ? 0 : (chain_iter--)->index + 1);

            if( chain_iter == chainlist.end() ){
                chainlist.emplace(_self, [&](struct chain_pair &a){
                    a.index = next_idx;
                    a.chain_name = chain;
                });
            }

            //Chain List Size Update w/ size checking
            if ( next_idx > chainlistsize ){
                chainlistsize = next_idx;
            }

            // validate fio fioname exists
            uint64_t nameHash = ::eosio::string_to_uint64_t(fio_address.c_str());
            //print("Name: ", fio_address, ", nameHash: ", nameHash, "..");
            auto fioname_iter = fionames.find(nameHash);
            eosio_assert_message_code(fioname_iter != fionames.end(), "fioname not registered..", ErrorFioNameNotRegistered); // TODO 404 Response
            
            //check that the name is not expired
            uint32_t name_expiration = fioname_iter->expiration;
            uint32_t present_time = now();
            
            //print("name_expiration: ", name_expiration, ", present_time: ", present_time, "\n");
            eosio_assert_message_code(present_time <= name_expiration, "fioname is expired.", ErrorFioNameExpired); // TODO 400 Response

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
            eosio_assert_message_code(domains_iter != domains.end(), "Domain not yet registered.", ErrorDomainNotRegistered); // TODO 404 Response

            uint32_t expiration = domains_iter->expiration;
            eosio_assert_message_code(present_time <= expiration, "Domain is expired.", ErrorDomainExpired); // TODO 400 Response

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

            //if (appConfig.pmtson) {
                // collect fees; Check for funds is implicitly done as part of the funds transfer.
                //auto fees = trxfees.get_or_default(trxfee());
                //asset registerFee = fees.upaddress;
                //print("Collecting registration fees: ", registerFee);
                //action(permission_level{requestor, N(active)},
                //       TokenContract, N(transfer),
                //       make_tuple(requestor, _self, registerFee,
                //                  string("Registration fees. Thank you."))
                //).send();
            //}
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

    }; // class FioNameLookup


    EOSIO_ABI( FioNameLookup, (registername)(addaddress)(removename)(removedomain)(rmvaddress) )
}
