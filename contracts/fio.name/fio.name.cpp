/** FioName Token implementation file
 *  Description: FioName smart contract allows issuance of unique domains and names for easy public address resolution
 *  @author Adam Androulidakis
 *  @file fio.name.hpp
 *  @copyright defined in eos/LICENSE.txt
 *
 *  Changes:
 *  
 *  Adam Androulidakis  8-31-2018
 *  Ciju John  8-30-2018
 *  Adam Androulidakis  8-29-2019
 */

#include "fio.name.hpp"

namespace fio {

    class FioName : public contract //, token(_self, _self)
	{
    private:
        configs config;
        domains_table domains;
        fionames_table fionames;
		account_name owner;
		

    public:
        FioName(account_name self)
        : contract(self), config(self, self), domains(self, self), fionames(self, self) {owner=self;}

        // @abi action
        void registerName (const string &name) {
            require_auth(owner);

			
            string domain = nullptr;
            string domainName = nullptr;
            
			//determine if there is dot in the name entered
			// Weak input validation, only enter string like Adam or Adam.Fio
			// TODO: Stronger input validation
			int pos = name.find('.');
			
            if (pos == string::npos) {
                domain = name;
            }
			else {
                domainName = name.substr(0, pos);
                domain = name.substr(pos + 1, string::npos);
            }

            print("Name: ", domainName, ", Domain: ", domain, "\n");
			// print("Name: ", (domainName), ", Domain: ", (domain), "\n");

            uint64_t domainHash = ::eosio::string_to_name(domain.c_str());
            if (domainName.empty()) { // domain 
                
				// check for domain availability
                print(", Domain hash: ", domainHash, "\n");
                auto domains_iter = domains.find(domainHash);
                eosio_assert(domains_iter == domains.end(), "Domain is already registered.");

                // check if callee has requisite FIO funds.
				// Do something
				
				
				
                // Issue, create and transfer nft domain token
				
				
                domains.emplace(_self, [&](struct domain &d) {
                    d.name=domain;
                    d.domainhash=domainHash;
                });
                // Add domain entry in domain table
            } 
			// If a domain is not entered, add a name
			else { // fioname 
				
				
                // check if domain exists.
                auto domains_iter = domains.find(domainHash);
                eosio_assert(domains_iter != domains.end(), "Domain not yet registered.");
				
                // check if domain permission is valid.
				// does callee account have perms??
				
				
				// Maybe do checking and assigning under registerName ?
				
                // check if fioname is available
                uint64_t accountHash = ::eosio::string_to_name(domainName.c_str());
				
				string pubKey =  "FIO:EOS78vWPM6dHgoaYXxU3jVtVCb6fRQjy3otLNuZ2WmkAGEmCkfQwL\nBTC:16ZYw7zXbCRAgkA2oPEij5wR9UKaSG5dbb\nBCH:16ZYw7zXbCRAgkA2oPEij5wR9UKaSG5dbb\n"; //temporary device - value is not yet passed to method
				uint64_t pubKeyHash = ::eosio::string_to_name(pubKey.c_str()); 
				
                print("Name hash: ", accountHash, ", Domain has: ", domainHash, "\n");
                auto fioname_iter = fionames.find(accountHash);
                eosio_assert(fioname_iter == fionames.end(), "Name is already registered.");
                
				// check if callee has requisite FIO funds.
				// DO SOMETHING
				
                // Issue, create and transfer fioname token
                // DO SOMETHING
				
				// Add account entry in fionames table
                fionames.emplace(_self, [&](struct fioname &a){
                    a.name=domainName;
                    a.namehash=accountHash;
                    a.domain=domain;
                    a.domainhash=domainHash;
					a.pubKey = pubKey;
					a.pubKeyHash = pubKeyHash;
		
                });
            }
        } //

        // @abi action
      	void addkey(const string &key, const string &chain) {
			// Do something
			eosio_assert(1,"This function has not yet been implemented");
        }
    };
    EOSIO_ABI( FioName, (registerName)(addkey) )
}
