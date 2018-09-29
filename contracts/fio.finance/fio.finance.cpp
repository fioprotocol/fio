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
        statecontainer contract_state;
        pending_requests_table pending_requests;
        finalized_requests_table finalized_requests;

        // Currently supported chains
        enum  class chain_type {
            FIO=0, EOS=1, BTC=2, ETH=3, XMR=4, BRD=5, BCH=6, NONE=7
        };
        const std::vector<std::string> chain_str {"FIO", "EOS", "BTC", "ETH", "XMR", "BRD", "BCH"};

    public:
        FioFinance(account_name self)
                : contract(self), config(self, self), contract_state(self,self), pending_requests(self, self), finalized_requests(self, self)
        { }

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
         * Requestor initiates funds request.
         */
        // @abi action
        void requestfunds(uint32_t requestid, const name& requestor, const name& requestee, const string &chain, const asset& quantity, const string &memo) {
            print("Validating authority ", requestor, "\n");
            require_auth(requestor); // we need requesters authorization
            is_account(requestee); // ensure requestee exists

            // validate chain is supported. This is a case insensitive check.
            string my_chain = chain;
            transform(my_chain.begin(), my_chain.end(), my_chain.begin(), ::toupper);
            print("Validating chain support: ", my_chain, "\n");
            assert_valid_chain(my_chain);

            // Validate requestid is unique for this user
            print("Validating requestid uniqueness", "\n");
            auto idx = pending_requests.get_index<N(byrequestid)>();
            auto matchingItem = idx.lower_bound(requestid);

            // Advance to the first entry matching the specified requestid and from
            while (matchingItem != idx.end() && matchingItem->requestid == requestid &&
                   matchingItem->requestor != requestor) {
                matchingItem++;
            }

            // assert identical request from same user doesn't exist
            assert(matchingItem == idx.end() ||
                   !(matchingItem->requestid == requestid && matchingItem->requestor == requestor));

            // get the current odt from contract state
            auto currentState = contract_state.get_or_default(contr_state());
            currentState.current_obt++; // increment the odt

            print("Adding pending request id:: ", requestid, "\n");
            // Add fioname entry in fionames table
            pending_requests.emplace(_self, [&](struct fundsrequest &fr) {
                fr.requestid = requestid;
                fr.obtid = currentState.current_obt;
                fr.requestor = requestor;
                fr.requestee = requestee;
                fr.chain = chain;
                fr.quantity = quantity;
                fr.memo = memo;
                fr.request_time = now();
                fr.final_time = UINT_MAX;
                fr.state = REQUEST_PENDING;
            });

            // Persist contract state
            contract_state.set(currentState, _self);
        }

        /***
         * Requestor cancel pending funds request
         */
        // @abi action
        void cancelrqst(uint32_t requestid, const name& requestor) {
            print("Validating authority ", requestor, "\n");
            require_auth(requestor); // we need requesters authorization

            print("Validating pending request exists: ", requestid, "\n");
            // validate a pending request exists for this requester with this requestid
            auto idx = pending_requests.get_index<N(byrequestid)>();
            auto matchingItem = idx.lower_bound(requestid);

            // Advance to the first entry matching the specified vID
            while (matchingItem != idx.end() && matchingItem->requestid == requestid &&
                   matchingItem->requestor != requestor) {
                matchingItem++;
            }

            // assert on match found
            assert(matchingItem != idx.end() && matchingItem->requestid == requestid &&
                   matchingItem->requestor == requestor);

            // if match found drop request
            print("Canceling pending request: ", matchingItem->requestid, "\n");
            idx.erase(matchingItem);
        }

        /***
         * Requestee approve pending funds request
         */
        // @abi action
        void approverqst(uint64_t obtid, const name& requestee) {
            print("Validating authority ", requestee, "\n");
            require_auth(requestee); // we need requesters authorization

            print("Validating pending request exists: ", obtid, "\n");
            // validate request obtid exists and is for this requestee
            auto idx = pending_requests.find(obtid);
            assert(idx != pending_requests.end());
            assert(idx->requestee == requestee);

            // approve request by moving it to approved requests table
            print("Approving pending request: ", idx->obtid, "\n");
            fundsrequest request = *idx;
            pending_requests.erase(idx);
            request.final_time = now();
            request.state = REQUEST_APPROVED;
            request.final_detail = "approved";
            finalized_requests.emplace(_self, [&](struct fundsrequest &fr){
                fr = request;
            });
        }

        /***
         * Requestee reject pending funds request
         */
        // @abi action
        void rejectrqst(uint64_t obtid, const name& requestee, const string& reject_detail) {
            print("Validating authority ", requestee, "\n");
            require_auth(requestee); // we need requesters authorization

            print("Validating pending request exists: ", obtid, "\n");
            // validate request obtid exists and is for this requestee
            auto idx = pending_requests.find(obtid);
            assert(idx != pending_requests.end());
            assert(idx->requestee == requestee);

            // approve request by moving it to approved requests table
            print("Rejecting pending request: ", idx->obtid, "\n");
            fundsrequest request = *idx;
            pending_requests.erase(idx);
            request.final_time = now();
            request.state = REQUEST_APPROVED;
            request.final_detail = reject_detail;
            finalized_requests.emplace(_self, [&](struct fundsrequest &fr){
                fr = request;
            });

        }


    }; // class FioFinance


    EOSIO_ABI( FioFinance, (requestfunds)(cancelrqst)(approverqst)(rejectrqst) )
}
