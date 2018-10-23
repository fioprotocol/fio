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
        keynames_table keynames;
        

        // Currently supported chains
        enum  class chain_type {
               FIO=0, EOS=1, BTC=2, ETH=3, XMR=4, BRD=5, BCH=6, NONE=7
        };
        const std::vector<std::string> chain_str {"FIO", "EOS", "BTC", "ETH", "XMR", "BRD", "BCH"};

    public:
        FioNameLookup(account_name self)
        : contract(self), config(self, self), domains(self, self), fionames(self, self),
          keynames(self, self) {
            owner=self;
        }
        
        
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
                
                //EXPIRATION get the expiration.
                uint32_t expiration_time = get_now_plus_one_year();

                // Issue, create and transfer nft domain token
                domains.emplace(_self, [&](struct domain &d) {
                    d.name=domain;
                    d.domainhash=domainHash;
                    d.expiration=expiration_time;
                });
                // Add domain entry in domain table
            } else { // fioname register
                
                // check if domain exists.
                auto domains_iter = domains.find(domainHash);
                eosio_assert(domains_iter != domains.end(), "Domain not yet registered.");
                
                // check if domain permission is valid.
                
                //EXPIRATION check if the domain is expired.
                uint32_t domain_expiration = domains_iter->expiration;
                uint32_t present_time = now();
                eosio_assert(present_time > domain_expiration,"Domain has expired.");
                
                // check if fioname is available
                uint64_t nameHash = ::eosio::string_to_name(newname.c_str());
                print("Name hash: ", nameHash, ", Domain has: ", domainHash, "\n");
                auto fioname_iter = fionames.find(nameHash);
                eosio_assert(fioname_iter == fionames.end(), "Fioname is already registered.");
                
                //EXPIRATION
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
         * This method will return now plus one year.
         * the result is the present block time seconds incremented by secondss per year.
         * EXPIRATION
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
            
            
            // EXPIRATION  this is a change added new code for expiration 10/19/2018
            //may need refactored into common methods
            //check that the name isnt expired
            uint32_t name_expiration = fioname_iter->expiration;
            uint32_t present_time = now();
            eosio_assert(present_time > name_expiration, "fioname is expired.");
            
            
            //get the domain and check that it is not expired.
            string domain = nullptr;
            size_t pos = fio_user_name.find('.');
            if (pos == string::npos) {
               eosio_assert(true,"could not find domain name in fio name.");
            } else {
                domain = fio_user_name.substr(pos + 1, string::npos);
            }
            uint64_t domainHash = ::eosio::string_to_name(domain.c_str());
            
            auto domains_iter = domains.find(domainHash);
            eosio_assert(domains_iter != domains.end(), "Domain not yet registered.");
            
            uint32_t expiration = domains_iter->expiration;
            eosio_assert(present_time > expiration, "Domain is expired.");
            //end EXPIRATION
            

            // insert/update <chain, address> pair
            fionames.modify(fioname_iter, _self, [&](struct fioname &a) {
                a.addresses[static_cast<size_t>(c_type)] = address;
            });

            // insert key into key-name table for reverse lookup
            keynames.emplace(_self, [&](struct key_name &k){
                k.id = keynames.available_primary_key();                // use next available primary key
                k.key = address;                                        // persist key
                k.keyhash = ::eosio::string_to_name(address.c_str());   // persist key hash
                k.chaintype = static_cast<uint64_t>(c_type);            // specific chain type
                k.name = fioname_iter->name;                            // FIO name
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
