
#include "dapix.namelookup.hpp"

namespace dapix {

    class namelookup : public contract {
    private:
        configs config;
        domains_table domains;
        accounts_table accounts;

        // Currently supported chains
        enum  class chain_type {
            BTC=0, ETH=1, EOS=2, NONE=3
        };
        const std::vector<std::string> chain_str {"BTC", "ETH", "EOS"};

    public:
        namelookup(account_name self)
        : contract(self), config(self, self), domains(self, self), accounts(self, self) {}

        /***
         * Purchase the given name.
         *
         * @param name The name to be purchased. Can be a domain e.g. "fio" or a user name "adam.fio".
         */
        // @abi action
        void purchase(const string &name) {
            require_auth(_self);

            string domain = nullptr;
            string accountName = domain;
            int pos = name.find('.');
            if (pos == string::npos) {
                domain = name;
            } else {
                accountName = name.substr(0, pos);
                domain = name.substr(pos + 1, string::npos);
            }

            print("Account: ", accountName, ", Domain: ", domain, "\n");

            uint64_t domainHash = ::eosio::string_to_name(domain.c_str());
            if (accountName.empty()) { // domain purchase
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
            } else { // account purchase
                // check if domain exists.
                auto domains_iter = domains.find(domainHash);
                eosio_assert(domains_iter != domains.end(), "Domain not yet registered.");
                // check if domain permission is valid.
                // check if account is available

                uint64_t nameHash = ::eosio::string_to_name(name.c_str());
                print("Name hash: ", nameHash, ", Domain has: ", domainHash, "\n");
                auto account_iter = accounts.find(nameHash);
                eosio_assert(account_iter == accounts.end(), "Account is already registered.");
                // check if callee has requisite dapix funds.
                // Issue, create and transfer account token
                // Add account entry in accounts table
                accounts.emplace(_self, [&](struct account &a){
                    a.name=accountName;
                    a.namehash=nameHash;
                    a.domain=domain;
                    a.domainhash=domainHash;
                    a.keys = vector<string>(chain_str.size(), "");
                });
            }
        }

        /***
         * Convert chain name to chain type. Returns chain_type::NONE if no match.
         *
         * @param chain The chain name e.g. "BTC"
         * @return
         */
        inline chain_type str_to_chain_type(const string &chain) {

            print("size: ", chain_str.size(), "\n");
            for (int i = 0; i < chain_str.size(); i++) {
                print("chain: ", chain, ", chain_str: ", chain_str[i], "\n");
                if (chain == chain_str[i]) {
                    print("Found supported chain.", "\n");
                    return static_cast<chain_type>(i);
                }
            }
            return chain_type::NONE;
        }

        /***
         * Given a fio user name, chain name and chain specific key will attach key to the user's FIO account.
         *
         * @param fio_user_name The FIO user name e.g. "adam.fio"
         * @param chain The chain name e.g. "btc"
         * @param key The chain specific user key
         */
        // @abi action
        void addkey(const string &fio_user_name, const string &chain, const string &key) {
            eosio_assert(!fio_user_name.empty(), "FIO user name cannot be empty.");
            eosio_assert(!chain.empty(), "Chain cannot be empty.");
            eosio_assert(!key.empty(), "Chain key cannot be empty.");

            // validate chain is supported. This is a case insensitive check.
            string my_chain=chain;
            transform(my_chain.begin(), my_chain.end(), my_chain.begin(), ::toupper);
            print("Validating chain support: ", my_chain, "\n");
            chain_type c_type= str_to_chain_type(my_chain);
            eosio_assert(c_type != chain_type::NONE, "Supplied chain isn't supported..");

            // validate fio account exists
            uint64_t nameHash = ::eosio::string_to_name(fio_user_name.c_str());
            auto account_iter = accounts.find(nameHash);
            eosio_assert(account_iter != accounts.end(), "Account not registered.");

            // insert/update <chain, key> pair
            accounts.modify(account_iter, _self, [&](struct account &a) {
                a.keys[static_cast<int>(c_type)] = key;
            });
        }
    };

    EOSIO_ABI( namelookup, (purchase)(addkey) )
}
