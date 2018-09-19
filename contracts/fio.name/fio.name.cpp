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
 *  Adam Androulidakis  8-29-2019
 */

#include "fio.name.hpp"

namespace fioio{
	
	 
    class FioNameLookup : public contract { 
		private:
        configs config;
        domains_table domains;
        fionames_table fionames;
		account_name owner;
		

        // Currently supported chains
        enum  class chain_type {
               FIO=0, EOS=1, BTC=2, ETH=3, XMR=4, BRD=5, BCH=6, NONE=7
        };
        const std::vector<std::string> chain_str {"FIO", "EOS", "BTC", "ETH", "XMR", "BRD", "BCH"};

    public:
        FioNameLookup(account_name self)
        : contract(self), config(self, self), domains(self, self), fionames(self, self) {owner=self;}

    
        // @abi action
        void registername(const string &name) {
            require_auth(owner);
			string newname = name;
			
			// make fioname lowercase before hashing
			transform(newname.begin(), newname.end(), newname.begin(), ::tolower);	
			
			
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

            uint64_t domainHash = ::eosio::string_to_name(domain.c_str());
            if (fioname.empty()) { // domain register
                // check for domain availability
                print(", Domain hash: ", domainHash, "\n");
                auto domains_iter = domains.find(domainHash);
                eosio_assert(domains_iter == domains.end(), "Domain is already registered.");
                // check if callee has requisite dapix funds.

                // Issue, create and transfer nft domain token
                domains.emplace(_self, [&](struct domain &d) {
                    d.name=domain;
                    d.domainhash=domainHash;
                });
                // Add domain entry in domain table
            } else { // fioname register
                
				// check if domain exists.
                auto domains_iter = domains.find(domainHash);
                eosio_assert(domains_iter != domains.end(), "Domain not yet registered.");
                
				// check if domain permission is valid.
                
				// check if fioname is available
				uint64_t nameHash = ::eosio::string_to_name(newname.c_str());
                print("Name hash: ", nameHash, ", Domain has: ", domainHash, "\n");
                auto fioname_iter = fionames.find(nameHash);
                eosio_assert(fioname_iter == fionames.end(), "Fioname is already registered.");
                

				
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
                    a.addresses = vector<string>(chain_str.size(), "");
                });
            } // else
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
         * Given a fio user name, chain name and chain specific address will attach address to the user's FIO fioname.
         *
         * @param fio_user_name The FIO user name e.g. "adam.fio"
         * @param chain The chain name e.g. "btc"
         * @param address The chain specific user address
         */
        // @abi action
        void addaddress(const string &fio_user_name, const string &chain, const string &address) {
            eosio_assert(!fio_user_name.empty(), "FIO user name cannot be empty.");
            eosio_assert(!chain.empty(), "Chain cannot be empty.");
            eosio_assert(!address.empty(), "Chain address cannot be empty.");
			// Verify the address does not have a whitespace
			eosio_assert(address.find(" "), "Chain cannot contain whitespace");
			
			// DO SOMETHING
			
            // validate chain is supported. This is a case insensitive check.
            string my_chain=chain;
            transform(my_chain.begin(), my_chain.end(), my_chain.begin(), ::toupper);
            print("Validating chain support: ", my_chain, "\n");
            chain_type c_type= str_to_chain_type(my_chain);
            eosio_assert(c_type != chain_type::NONE, "Supplied chain isn't supported..");

            // validate fio fioname exists
            uint64_t nameHash = ::eosio::string_to_name(fio_user_name.c_str());
            auto fioname_iter = fionames.find(nameHash);
            eosio_assert(fioname_iter != fionames.end(), "fioname not registered.");

            // insert/update <chain, address> pair
            fionames.modify(fioname_iter, _self, [&](struct fioname &a) {
                a.addresses[static_cast<size_t>(c_type)] = address;
            });
        
		}
		
		
		void removename() {
		}
		
		void removedomain() {	
		}
		
		void rmvaddress() {
		}
		
		
    }; // class FioNameLookup
	

    EOSIO_ABI( FioNameLookup, (registername)(addaddress)(removename)(removedomain)(rmvaddress) )
}
