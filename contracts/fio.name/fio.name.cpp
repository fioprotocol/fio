/** FioName Token implementation file
 *  Description: FioName smart contract allows issuance of unique domains and names for easy public address resolution
 *  @author Adam Androulidakis
 *  @file fio.name.hpp
 *  @copyright Dapix
 *
 *  Changes:
 *  Adam Androulidakis 9-18-2018
 *  Adam Androulidakis 9-6-2018
 *  Ciju John 9-5-2018
 *  Adam Androulidakis  8-31-2018
 *  Ciju John  8-30-2018
 *  Adam Androulidakis  8-29-2018
 *  Ed Rotthoff 10-26-2018
 */

#include <eosiolib/asset.hpp>
#include "fio.name.hpp"
#include <fio.common/fio.common.hpp>

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
            ErrorDomainExpired =          109,   // Fioname not yet registered.
            ErrorFioNameExpired =            110;   // Fioname not yet registered.

    class FioNameLookup : public contract { 
        private:
        configs config;
        domains_table domains;
        fionames_table fionames;
        account_name owner;
        keynames_table keynames;
		trxfees_singleton trxfees;

        const account_name TokenContract = eosio::string_to_name(TOKEN_CONTRACT);

        // Currently supported chains
        enum  class chain_type {
               FIO=0, EOS=1, BTC=2, ETH=3, XMR=4, BRD=5, BCH=6, NONE=7
        };
        const std::vector<std::string> chain_str {"FIO", "EOS", "BTC", "ETH", "XMR", "BRD", "BCH"};

    public:
        FioNameLookup(account_name self)
        : contract(self), config(self, self), domains(self, self), fionames(self, self), keynames(self, self),
            trxfees(FeeContract,FeeContract)
            {}


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
            if (fioname.empty()) { // domain register
                // check for domain availability
                auto domains_iter = domains.find(domainHash);
                eosio_assert_message_code(domains_iter == domains.end(), "Domain is already registered.", ErrorDomainAlreadyRegistered);
                // check if callee has requisite dapix funds.
                
                //get the expiration for this new domain.
                uint32_t expiration_time = get_now_plus_one_year();

                // Issue, create and transfer nft domain token
                // Add domain entry in domain table
                domains.emplace(_self, [&](struct domain &d) {
                    d.name=domain;
                    d.domainhash=domainHash;
                    d.expiration=expiration_time;
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
                uint32_t expiration = get_now_plus_one_year();
                
                // check if callee has requisite dapix funds.
                // DO SOMETHING
                
                // Issue, create and transfer fioname token
                // DO SOMETHING
                
                // Add fioname entry in fionames table
                fionames.emplace(_self, [&](struct fioname &a){
                    a.name=newname;
                    a.namehash=nameHash;
                    a.domain=domain;
                    a.domainhash=domainHash;
                    a.expiration=expiration;
                    a.addresses = vector<string>(chain_str.size(), "");
                });

                registerFee = fees.nameregister;
            } // else

            // collect fees
            // check for funds is implicitly done as part of the funds transfer.
            print("Collecting registration fees: ", registerFee);
            action(permission_level{requestor, N(active)},
                   TokenContract, N(transfer),
                   make_tuple(requestor, _self, registerFee,
                              string("Registration fees. Thank you."))
            ).send();
        }

        /***
         * Convert chain name to chain type.
         *
         * @param chain The chain name e.g. "BTC"
         * @return chain_type::NONE if no match.
         */
        inline chain_type str_to_chain_type(const string &chain) {

            print("size: ", chain_str.size(), "\n");
            for (size_t i = 0; i < chain_str.size(); i++) {
                print("chain: ", chain, ", chain_str: ", chain_str[i], "\n");
                if (chain == chain_str[i]) {
                    print("Found supported chain.", "\n");
                    return static_cast<chain_type>(i);
                }
            }
            return chain_type::NONE;
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
         * Validate chain is in the supported chains list.
         * @param chain The chain to validate, expected to be in lower case.s
         */
        inline void assert_valid_chain(const string &chain) {
            assert(str_to_chain_type(chain) != chain_type::NONE);
        }

        /***
         * Given a fio user name, chain name and chain specific address will attach address to the user's FIO fioname.
         *
         * @param fio_user_name The FIO user name e.g. "adam.fio"
         * @param chain The chain name e.g. "btc"
         * @param address The chain specific user address
         */
        [[eosio::action]]
        void addaddress(const string &fio_user_name, const string &chain, const string &address) {
            eosio_assert_message_code(!fio_user_name.empty(), "FIO user name cannot be empty..", ErrorFioNameEmpty);
            eosio_assert_message_code(!chain.empty(), "Chain cannot be empty..", ErrorChainEmpty);
            eosio_assert_message_code(!address.empty(), "Chain address cannot be empty..", ErrorChainAddressEmpty);
			// Verify the address does not have a whitespace
            eosio_assert_message_code(address.find(" "), "Chain address cannot contain whitespace..", ErrorChainContainsWhiteSpace);
			
            // validate chain is supported. This is a case insensitive check.
            string my_chain=chain;
            transform(my_chain.begin(), my_chain.end(), my_chain.begin(), ::toupper);
            print("Validating chain support: ", my_chain, "\n");
            chain_type c_type= str_to_chain_type(my_chain);
            eosio_assert_message_code(c_type != chain_type::NONE, "Supplied chain isn't supported..", ErrorChainNotSupported);

            // validate fio fioname exists
            uint64_t nameHash = ::eosio::string_to_uint64_t(fio_user_name.c_str());
            print("Name: ", fio_user_name, ", nameHash: ", nameHash, "..");
            auto fioname_iter = fionames.find(nameHash);
            eosio_assert_message_code(fioname_iter != fionames.end(), "fioname not registered..", ErrorFioNameNotRegistered);
            
            //check that the name isnt expired
            uint32_t name_expiration = fioname_iter->expiration;
            uint32_t present_time = now();
            
            print("name_expiration: ", name_expiration, ", present_time: ", present_time, "\n");
            eosio_assert_message_code(present_time <= name_expiration, "fioname is expired.", ErrorFioNameExpired);

            //parse the domain and check that the domain is not expired.
            string domain = nullptr;
            size_t pos = fio_user_name.find('.');
            if (pos == string::npos) {
               eosio_assert(true,"could not find domain name in fio name.");
            } else {
                domain = fio_user_name.substr(pos + 1, string::npos);
            }
            uint64_t domainHash = ::eosio::string_to_uint64_t(domain.c_str());
            print("Domain: ", domain, ", domainHash: ", domainHash, "..");
            
            auto domains_iter = domains.find(domainHash);
            eosio_assert_message_code(domains_iter != domains.end(), "Domain not yet registered.", ErrorDomainNotRegistered);
            
            uint32_t expiration = domains_iter->expiration;
            eosio_assert_message_code(present_time <= expiration, "Domain is expired.", ErrorDomainExpired);
            

            // insert/update <chain, address> pair
            fionames.modify(fioname_iter, _self, [&](struct fioname &a) {
                a.addresses[static_cast<size_t>(c_type)] = address;
            });

            // insert/update key into key-name table for reverse lookup
            auto idx = keynames.get_index<N(bykey)>();
            auto keyhash = ::eosio::string_to_uint64_t(address.c_str());
            auto matchingItem = idx.lower_bound(keyhash);

            // Advance to the first entry matching the specified address and chain
            while (matchingItem != idx.end() && matchingItem->keyhash == keyhash  && matchingItem->chaintype != static_cast<uint64_t>(c_type)) {
                matchingItem++;
            }
            if (matchingItem == idx.end() || matchingItem->keyhash != keyhash) {
                keynames.emplace(_self, [&](struct key_name &k) {
                    k.id = keynames.available_primary_key();        // use next available primary key
                    k.key = address;                                // persist key
                    k.keyhash = keyhash;                            // persist key hash
                    k.chaintype = static_cast<uint64_t>(c_type);    // specific chain type
                    k.name = fioname_iter->name;                    // FIO name
                    k.expiration=name_expiration;
                });
            } else {
                idx.modify(matchingItem, _self, [&](struct key_name &k) {
                    k.name = fioname_iter->name;    // FIO name
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

    }; // class FioNameLookup


    EOSIO_ABI( FioNameLookup, (registername)(addaddress)(removename)(removedomain)(rmvaddress) )
}
