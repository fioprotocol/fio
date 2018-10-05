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
        transaction_contexts_table transaction_contexts;
        transaction_logs_table transaction_logs;
        pending_requests_table pending_requests;

        // data format of transactions, tail numeral indicates number of operands
        string trx_type_dta_NO_REQ_RPRT_1=  R"({"obtid":"%s"})";                // ${trx_type::NO_REQ_RPRT}: obtid
        string trx_type_dta_REQ_2=          R"({"reqid":"%d","memo":"%s"})";    // ${trx_type::REQ}: requestid, memo
        string trx_type_dta_REQ_CANCEL_1=   R"("memo":"%s"})";                  // ${trx_type::REQ_CANCEL}
        string trx_type_dta_REQ_RPRT_2=     R"({"obtid":"%s","memo":"%s"})";    // ${trx_type::REQ_RPRT}: obtid, memo
        string trx_type_dta_RCPT_VRFY_2=    R"("memo":"%s"})";                  // ${trx_type::RCPT_VRFY}

        // User printable supported chains strings.
        const std::vector<std::string> chain_str {
                "FIO",
                "EOS",
                "BTC",
                "ETH",
                "XMR",
                "BRD",
                "BCH"
        };


    public:
        FioFinance(account_name self)
                : contract(self), config(self, self), contract_state(self,self), transaction_contexts(self, self), transaction_logs(self,self),
                pending_requests(self, self)
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

        inline static std::string format(const std::string format, ...)
        {
            va_list args;
            va_start (args, format);
            size_t len = static_cast<size_t>(std::vsnprintf(NULL, 0, format.c_str(), args));
            va_end (args);
            std::vector<char> vec(len + 1);
            va_start (args, format);
            std::vsnprintf(&vec[0], len + 1, format.c_str(), args);
            va_end (args);
            return &vec[0];
        }

        inline bool is_double(const std::string& s)
        {
            try
            {
                std::stod(s);
            }
            catch(...)
            {
                return false;
            }
            return true;
        }

        /***
         * Requestor initiates funds request.
         */
        // @abi action
        void requestfunds(uint64_t requestid, const name& requestor, const name& requestee, const string &chain,
                const string& asset, const string quantity, const string &memo) {
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
            auto matchingItem = pending_requests.lower_bound(requestid);

            // Advance to the first entry matching the specified requestid and from
            while (matchingItem != pending_requests.end() && matchingItem->requestid == requestid &&
                   matchingItem->originator != requestor) {
                matchingItem++;
            }

            // assert identical request from same user doesn't exist
            assert(matchingItem == pending_requests.end() ||
                   !(matchingItem->requestid == requestid && matchingItem->originator == requestor));

            print("Validate quantity is double");
            assert(is_double(quantity));

            // get the current odt from contract state
            auto currentState = contract_state.get_or_default(contr_state());
            currentState.current_fioappid++; // increment the fioappid

            print("Adding transaction context record");
            transaction_contexts.emplace(_self, [&](struct trxcontext &ctx) {
               ctx.fioappid = currentState.current_fioappid;
               ctx.originator = requestor;
               ctx.receiver = requestee;
               ctx.chain = chain;
               ctx.asset = asset;
               ctx.quantity = quantity;
            });

            print("Adding transaction log");
            transaction_logs.emplace(_self, [&](struct trxlog &log) {
                log.fioappid = currentState.current_fioappid;
                log.type = static_cast<uint16_t>(trx_type::REQ);
                log.status = static_cast<uint16_t>(trx_sts::REQ);
                log.time = now();

                string data = FioFinance::format(trx_type_dta_REQ_2, requestid, memo.c_str());
                log.data=data;
            });

            print("Adding pending request id: ", requestid, "\n");
            // Add fioname entry in fionames table
            pending_requests.emplace(_self, [&](struct fundsrequest &fr) {
                fr.requestid = requestid;
                fr.fioappid = currentState.current_fioappid;
                fr.originator = requestor;
                fr.receiver = requestee;
            });

            // Persist contract state
            contract_state.set(currentState, _self);
        }

        /***
         * Requestor cancel pending funds request
         */
        // @abi action
        void cancelrqst(uint64_t requestid, const name& requestor, const string memo) {
            print("Validating authority ", requestor, "\n");
            require_auth(requestor); // we need requesters authorization

            print("Validating pending request exists: ", requestid, "\n");
            // validate a pending request exists for this requester with this requestid
            auto matchingItem = pending_requests.lower_bound(requestid);

            // Advance to the first entry matching the specified vID
            while (matchingItem != pending_requests.end() && matchingItem->requestid == requestid &&
                   matchingItem->originator != requestor) {
                matchingItem++;
            }

            // assert on match found
            assert(matchingItem != pending_requests.end() && matchingItem->requestid == requestid &&
                   matchingItem->originator == requestor);

            print("Adding transaction log");
            transaction_logs.emplace(_self, [&](struct trxlog &log) {
                log.fioappid = matchingItem->fioappid;
                log.type = static_cast<uint16_t>(trx_type::REQ_CANCEL);
                log.status = static_cast<uint16_t>(trx_sts::REQ_CANCEL);
                log.time = now();

                string data = FioFinance::format(trx_type_dta_REQ_CANCEL_1, memo.c_str());
                log.data=data;
            });

            // drop pending request
            print("Removing pending request: ", matchingItem->requestid, "\n");
            pending_requests.erase(matchingItem);
        }

//        string trx_type_dta_REQ_RPRT_2=     R"({"obtid":"%s","memo":"%s"})";    // ${trx_type::REQ_RPRT}: obtid, memo
        /***
         * Requestee approve pending funds request
         */
        // @abi action
        void approverqst(const uint64_t fioappid, const name& requestee, const string& obtid, const string memo) {
            print("Validating authority ", requestee, "\n");
            require_auth(requestee); // we need requesters authorization

            print("Validating pending request exists: ", obtid, "\n");
            // validate request fioappid exists and is for this requestee
            auto idx = pending_requests.get_index<N(byfioappid)>();
            auto matchingItem = idx.lower_bound(fioappid);
            assert(matchingItem != idx.end());
            assert(matchingItem->receiver == requestee);

            print("Adding transaction log");
            transaction_logs.emplace(_self, [&](struct trxlog &log) {
                log.fioappid = matchingItem->fioappid;
                log.type = static_cast<uint16_t>(trx_type::REQ_RPRT);
                log.status = static_cast<uint16_t>(trx_sts::RPRT);
                log.time = now();

                string data = FioFinance::format(trx_type_dta_REQ_RPRT_2, obtid.c_str(), memo.c_str());
                log.data=data;
            });

            // drop pending request
            print("Removing pending request: ", matchingItem->requestid, "\n");
//            pending_requests.erase(matchingItem);

//            // approve request by moving it to approved requests table
//            print("Approving pending request: ", idx->obtid, "\n");
//            fundsrequest request = *idx;
//            pending_requests.erase(idx);
//            request.final_time = now();
//            request.state = REQUEST_APPROVED;
//            request.final_detail = "approved";
//            finalized_requests.emplace(_self, [&](struct fundsrequest &fr){
//                fr = request;
//            });
        }
//
//        /***
//         * Requestee reject pending funds request
//         */
//        // @abi action
//        void rejectrqst(uint64_t obtid, const name& requestee, const string& reject_detail) {
//            print("Validating authority ", requestee, "\n");
//            require_auth(requestee); // we need requesters authorization
//
//            print("Validating pending request exists: ", obtid, "\n");
//            // validate request obtid exists and is for this requestee
//            auto idx = pending_requests.find(obtid);
//            assert(idx != pending_requests.end());
//            assert(idx->requestee == requestee);
//
//            // approve request by moving it to approved requests table
//            print("Rejecting pending request: ", idx->obtid, "\n");
//            fundsrequest request = *idx;
//            pending_requests.erase(idx);
//            request.final_time = now();
//            request.state = REQUEST_APPROVED; // TBD REQUEST_REJECTED
//            request.final_detail = reject_detail;
//            finalized_requests.emplace(_self, [&](struct fundsrequest &fr){
//                fr = request;
//            });
//
//        }


    }; // class FioFinance


    EOSIO_ABI( FioFinance, (requestfunds)(cancelrqst)(approverqst) )
//    EOSIO_ABI( FioFinance, (requestfunds)(cancelrqst)(approverqst)(rejectrqst) )
}
