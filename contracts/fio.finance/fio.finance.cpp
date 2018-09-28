/** Fio Finance implementation file
 *  Description: FioFinance smart contract supports funds request and approval.
 *  @author Ciju John
 *  @file fio.finance.cpp
 *  @copyright Dapix
 *
 *  Changes:
 */

#include <eosiolib/asset.hpp>
#include "fio.finance.hpp"

#include <climits>

namespace fioio{


    class FioFinance : public contract {
		private:
        configs config;
        account_name owner;
        pending_requests_table pending_requests;
        approved_requests_table approved_requests;

        // Currently supported chains
        enum  class chain_type {
               FIO=0, EOS=1, BTC=2, ETH=3, XMR=4, BRD=5, BCH=6, NONE=7
        };
        const std::vector<std::string> chain_str {"FIO", "EOS", "BTC", "ETH", "XMR", "BRD", "BCH"};

    public:
        FioFinance(account_name self)
                : contract(self), config(self, self), pending_requests(self, self), approved_requests(self, self)
        {owner=self;}

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
         * Validate chain is in the supported chains list.
         * @param chain The chain to validate, expected to be in lower case.s
         */
        inline void assert_valid_chain(const string &chain) {
            assert(str_to_chain_type(chain) != chain_type::NONE);
        }

        /***
         * User initaites a funds request.
         */
        // @abi action
        void requestfunds(uint32_t requestid, const name& from, const name& to, const string &chain, const asset& quantity, const string &memo) {
            require_auth(from); // we need requesters authorization
            is_account(to); // ensure requestee exists

            // validate chain is supported. This is a case insensitive check.
            string my_chain = chain;
            transform(my_chain.begin(), my_chain.end(), my_chain.begin(), ::toupper);
            print("Validating chain support: ", my_chain, "\n");
            assert_valid_chain(my_chain);

            // Validate requestid is unique for this user
            print("Validating requestid uniqueness");
            auto idx = pending_requests.get_index<N(byrequestid)>();
            auto matchingItem = idx.lower_bound(requestid);

            // Advance to the first entry matching the specified requestid and from
            while (matchingItem != idx.end() && matchingItem->requestid == requestid &&
                   matchingItem->from != from) {
                matchingItem++;
            }

            // assert identical request from same user doesn't exist
            assert(matchingItem == idx.end() ||
                   !(matchingItem->requestid == requestid && matchingItem->from == from));

            print("Adding pending request id:: ", requestid);
            // Add fioname entry in fionames table
            pending_requests.emplace(_self, [&](struct fundsrequest &fr) {
                fr.requestid = requestid;
                fr.obtid = 0; // TBD
                fr.from = from;
                fr.to = to;
                fr.chain = chain;
                fr.quantity = quantity;
                fr.memo = memo;
                fr.request_time = now();
                fr.aproval_time = UINT_MAX;
            });
        }

        /***
         * Cancel pending funds request
         */
        // @abi action
        void cancelrqst(uint16_t requestid, const name& from) {
            require_auth(from); // we need requesters authorization

            // validate a pending request exists for this requester with this requestid
            auto idx = pending_requests.get_index<N(byrequestid)>();
            auto matchingItem = idx.lower_bound(requestid);

            // Advance to the first entry matching the specified vID
            while (matchingItem != idx.end() && matchingItem->requestid == requestid &&
                   matchingItem->from != from) {
                matchingItem++;
            }

            // assert on match found
            assert(matchingItem != idx.end() && matchingItem->requestid == requestid &&
                                 matchingItem->from == from);

            // if match found drop request
            print("Deleteing pending request id:: ", matchingItem->requestid);
            idx.erase(matchingItem);
        }

        /***
         * approve pending funds request
         */
        // @abi action
        void approverqst(uint64_t obtid, const name& requestee) {
            require_auth(requestee); // we need requesters authorization
            // validate request obtid exists and is for this requestee
            auto idx = pending_requests.find(obtid);
            assert(idx != pending_requests.end());
            assert(idx->to == requestee);

            // approve request by moving it to approved requests table
            fundsrequest request = *idx;
            pending_requests.erase(idx);
            request.aproval_time = now();
            approved_requests.emplace(_self, [&](struct fundsrequest &fr){
                fr = request;
            });
        }

    }; // class FioFinance


    EOSIO_ABI( FioFinance, (requestfunds)(cancelrqst)(approverqst) )
}
