/** Fio multi signature implementation file
 *  Description: eosio msig is the smart contract that creates and manages multi signature operations for the
 *  FIO protocol. this allows the proposal of multi signature transactions including the list of required
 *  approvers. approvers may approve and dis-approve of the proposed multi signature transaction. the proposer
 *  of the multi signature transaction may choose to cancel the proposal. The approvers may choose to invalidate
 *  themselves from all future required approvers for all present and future proposals.
 *  @author Ed Rotthoff
 *  @modifedby
 *  @file eosio.msig.cpp
 *  @license FIO Foundation ( https://github.com/fioprotocol/fio/blob/master/LICENSE ) Dapix
 */
#include <eosio.msig/eosio.msig.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/permission.hpp>
#include <eosiolib/crypto.hpp>
#include "fio.common/fio.accounts.hpp"
#include "fio.common/fioerror.hpp"
using namespace fioio;
namespace eosio {

    time_point current_time_point() {
        const static time_point ct{microseconds{static_cast<int64_t>( current_time())}};
        return ct;
    }





    /**********
     * This action permits the invoker to propose a multi signature operation. The operaton must be
     * signed by the list of requested accounts, once the accounts sign the approval then the tx
     * will be executed.
     * @param proposer this is the account proposing the multi signature operation.
     * @param proposal_name this is the name of the proposal being proposed.
     * @param requested  this is the list of the accounts requested to approve the multi signature operation.
     * @param trx this is the list of transactions to be executed when the multi signature transaction is satisfied.
     */
    void multisig::propose(ignore<name> proposer,
                           ignore<name> proposal_name,
                           ignore<std::vector<permission_level>> requested,
                           ignore<uint64_t> max_fee,
                           ignore<transaction> trx
                           ) {



        name _proposer;
        name _proposal_name;
        std::vector <permission_level> _requested;
        uint64_t _maxfee;
        transaction_header _trx_header;

        _ds >> _proposer >> _proposal_name >> _requested >> _maxfee;

        const char *trx_pos = _ds.pos();
        size_t size = _ds.remaining();
        _ds >> _trx_header;

        require_auth(_proposer);
        check(_trx_header.expiration >= eosio::time_point_sec(current_time_point()), "transaction expired");
        //check( trx_header.actions.size() > 0, "transaction must have at least one action" );

        proposals proptable(_self, _proposer.value);
        check(proptable.find(_proposal_name.value) == proptable.end(), "proposal with the same name exists");

        //get the sizes of tx.
        uint64_t sizep = transaction_size();

        bool isTopProd = false;
        auto tpiter = topprods.find(_proposer.value);
        if (tpiter != topprods.end()){
            isTopProd = true;
        }


        //collect fee if its not a fio system account.
        if(!(   _proposer == fioio::MSIGACCOUNT ||
                _proposer == fioio::WRAPACCOUNT ||
                _proposer == fioio::SYSTEMACCOUNT ||
                _proposer == fioio::ASSERTACCOUNT ||
                _proposer == fioio::REQOBTACCOUNT ||
                _proposer == fioio::FeeContract ||
                _proposer == fioio::AddressContract ||
                _proposer == fioio::TPIDContract ||
                _proposer == fioio::TokenContract ||
                _proposer == fioio::FOUNDATIONACCOUNT ||
                _proposer == fioio::TREASURYACCOUNT ||
                _proposer == fioio::FIOSYSTEMACCOUNT ||
                _proposer == fioio::FIOACCOUNT ||
                isTopProd)
                ) {
            //collect fees.
            eosio::action{
                    permission_level{_proposer, "active"_n},
                    fioio::FeeContract, "bytemandfee"_n,
                    std::make_tuple(std::string("msig_propose"), _proposer, _maxfee, sizep)
            }.send();
        }

        auto packed_requested = pack(_requested);
        auto res = ::check_transaction_authorization(trx_pos, size,
                                                     (const char *) 0, 0,
                                                     packed_requested.data(), packed_requested.size()
        );
        check(res > 0, "transaction authorization failed");

        std::vector<char> pkd_trans;
        pkd_trans.resize(size);
        memcpy((char *) pkd_trans.data(), trx_pos, size);
        proptable.emplace(_proposer, [&](auto &prop) {
            prop.proposal_name = _proposal_name;
            prop.packed_transaction = pkd_trans;
        });

        approvals apptable(_self, _proposer.value);
        apptable.emplace(_proposer, [&](auto &a) {
            a.proposal_name = _proposal_name;
            a.requested_approvals.reserve(_requested.size());
            for (auto &level : _requested) {
                a.requested_approvals.push_back(approval{level, time_point{microseconds{0}}});
            }
        });

        if (PROPOSERAM > 0) {
            action(
                    permission_level{SYSTEMACCOUNT, "active"_n},
                    "eosio"_n,
                    "incram"_n,
                    std::make_tuple(_proposer, PROPOSERAM)
            ).send();
        }

    }

    /**********
     * This action is used by those approving a multi signature proposal.
     * @param proposer  this is the account name of the proposer of the proposal.
     * @param proposal_name  this is the proposal name
     * @param level  this is the actor and permission approving the multi signature operation
     *              ex: '{"actor": "partner22222", "permission": "active"}'
     * @param proposal_hash this is optional, the sha256 hash of the packed transaction being proposed
     */
    void multisig::approve(name proposer, name proposal_name, permission_level level, const uint64_t &max_fee,
                           const eosio::binary_extension<eosio::checksum256> &proposal_hash) {
        require_auth(level);


        if (proposal_hash) {
            proposals proptable(_self, proposer.value);
            auto &prop = proptable.get(proposal_name.value, "proposal not found");
            assert_sha256(prop.packed_transaction.data(), prop.packed_transaction.size(), *proposal_hash);
        }

        //collect fees.
        eosio::action{
                permission_level{level.actor, "active"_n},
                fioio::FeeContract, "mandatoryfee"_n,
                std::make_tuple(std::string("msig_approve"), level.actor, max_fee)
        }.send();

        approvals apptable(_self, proposer.value);
        auto apps_it = apptable.find(proposal_name.value);
        if (apps_it != apptable.end()) {
            auto itr = std::find_if(apps_it->requested_approvals.begin(), apps_it->requested_approvals.end(),
                                    [&](const approval &a) { return a.level == level; });
            check(itr != apps_it->requested_approvals.end(), "approval is not on the list of requested approvals");

            apptable.modify(apps_it, proposer, [&](auto &a) {
                a.provided_approvals.push_back(approval{level, current_time_point()});
                a.requested_approvals.erase(itr);
            });
        } else {
            old_approvals old_apptable(_self, proposer.value);
            auto &apps = old_apptable.get(proposal_name.value, "proposal not found");

            auto itr = std::find(apps.requested_approvals.begin(), apps.requested_approvals.end(), level);
            check(itr != apps.requested_approvals.end(), "approval is not on the list of requested approvals");

            old_apptable.modify(apps, proposer, [&](auto &a) {
                a.provided_approvals.push_back(level);
                a.requested_approvals.erase(itr);
            });
        }

        if (APPROVERAM > 0) {
            action(
                    permission_level{SYSTEMACCOUNT, "active"_n},
                    "eosio"_n,
                    "incram"_n,
                    std::make_tuple(level.actor, APPROVERAM)
            ).send();
        }

        fio_400_assert(transaction_size() <= MAX_TRX_SIZE, "transaction_size", std::to_string(transaction_size()),
          "Transaction is too large", ErrorTransaction);
    }

    /***********
     * This action will indicate the dis-approval of the specified proposal by the signer of the transaction.
     * @param proposer  this is the account name of the proposer of the multi signature operation.
     * @param proposal_name  the proposal name of the multi signature operation.
     * @param level this is the actor and permission dis-approving the multi signature operation
     *              ex: '{"actor": "partner22222", "permission": "active"}'
     */
    void multisig::unapprove(name proposer, name proposal_name, permission_level level, const uint64_t &max_fee) {
        require_auth(level);

        //collect fees.
        eosio::action{
                permission_level{level.actor, "active"_n},
                fioio::FeeContract, "mandatoryfee"_n,
                std::make_tuple(std::string("msig_unapprove"), level.actor, max_fee)
        }.send();

        approvals apptable(_self, proposer.value);
        auto apps_it = apptable.find(proposal_name.value);
        if (apps_it != apptable.end()) {
            auto itr = std::find_if(apps_it->provided_approvals.begin(), apps_it->provided_approvals.end(),
                                    [&](const approval &a) { return a.level == level; });
            check(itr != apps_it->provided_approvals.end(), "no approval previously granted");
            apptable.modify(apps_it, proposer, [&](auto &a) {
                a.requested_approvals.push_back(approval{level, current_time_point()});
                a.provided_approvals.erase(itr);
            });
        } else {
            old_approvals old_apptable(_self, proposer.value);
            auto &apps = old_apptable.get(proposal_name.value, "proposal not found");
            auto itr = std::find(apps.provided_approvals.begin(), apps.provided_approvals.end(), level);
            check(itr != apps.provided_approvals.end(), "no approval previously granted");
            old_apptable.modify(apps, proposer, [&](auto &a) {
                a.requested_approvals.push_back(level);
                a.provided_approvals.erase(itr);
            });
        }

        fio_400_assert(transaction_size() <= MAX_TRX_SIZE, "transaction_size", std::to_string(transaction_size()),
          "Transaction is too large", ErrorTransaction);
    }


    /*******
     * This action will perform the cancellation of the specified proposal.
     * @param proposer this is the account name of the proposer of the multi signature operation.
     * @param proposal_name this is the proposal name of the multi signature operation.
     * @param canceler this is the canceller of the operation, this must be the same as the proposer.
     *                 unless the proposal transaction is expired then the canceler may be other
     *                 than the proposer.
     */
    void multisig::cancel(name proposer, name proposal_name, name canceler, const uint64_t &max_fee) {
        require_auth(canceler);

        proposals proptable(_self, proposer.value);
        auto &prop = proptable.get(proposal_name.value, "proposal not found");

        if (canceler != proposer) {
            check(unpack<transaction_header>(prop.packed_transaction).expiration <
                  eosio::time_point_sec(current_time_point()), "cannot cancel until expiration");
        }
        //collect fees.
        eosio::action{
                permission_level{canceler, "active"_n},
                fioio::FeeContract, "mandatoryfee"_n,
                std::make_tuple(std::string("msig_cancel"), canceler, max_fee)
        }.send();

        proptable.erase(prop);

        //remove from new table
        approvals apptable(_self, proposer.value);
        auto apps_it = apptable.find(proposal_name.value);
        if (apps_it != apptable.end()) {
            apptable.erase(apps_it);
        } else {
            old_approvals old_apptable(_self, proposer.value);
            auto apps_it = old_apptable.find(proposal_name.value);
            check(apps_it != old_apptable.end(), "proposal not found");
            old_apptable.erase(apps_it);
        }

        fio_400_assert(transaction_size() <= MAX_TRX_SIZE, "transaction_size", std::to_string(transaction_size()),
          "Transaction is too large", ErrorTransaction);
    }

    /******
     * This action will attempt to execute the specified multi signature transaction.
     * if the necessary approvals are not present this operation will fail, but may be
     * called again once the necessary approvals are present.
     * @param proposer the account name of the proposer of this multi signature transaction.
     * @param proposal_name the proposal name of this multi signature transaction
     * @param executer the account name of the executor of this multi signature transaction.
     */
    void multisig::exec(name proposer, name proposal_name, const uint64_t &max_fee, name executer) {
        require_auth(executer);

        proposals proptable(_self, proposer.value);
        auto &prop = proptable.get(proposal_name.value, "proposal not found");
        transaction_header trx_header;
        datastream<const char *> ds(prop.packed_transaction.data(), prop.packed_transaction.size());
        ds >> trx_header;
        check(trx_header.expiration >= eosio::time_point_sec(current_time_point()), "transaction expired");

        //collect fees.
        eosio::action{
                permission_level{executer, "active"_n},
                fioio::FeeContract, "mandatoryfee"_n,
                std::make_tuple(std::string("msig_exec"), executer, max_fee)
        }.send();

        approvals apptable(_self, proposer.value);
        auto apps_it = apptable.find(proposal_name.value);
        std::vector <permission_level> approvals;
        invalidations inv_table(_self, _self.value);
        if (apps_it != apptable.end()) {
            approvals.reserve(apps_it->provided_approvals.size());
            for (auto &p : apps_it->provided_approvals) {
                auto it = inv_table.find(p.level.actor.value);
                if (it == inv_table.end() || it->last_invalidation_time < p.time) {
                    approvals.push_back(p.level);
                }
            }
            apptable.erase(apps_it);
        } else {
            old_approvals old_apptable(_self, proposer.value);
            auto &apps = old_apptable.get(proposal_name.value, "proposal not found");
            for (auto &level : apps.provided_approvals) {
                auto it = inv_table.find(level.actor.value);
                if (it == inv_table.end()) {
                    approvals.push_back(level);
                }
            }
            old_apptable.erase(apps);
        }
        auto packed_provided_approvals = pack(approvals);
        auto res = ::check_transaction_authorization(prop.packed_transaction.data(), prop.packed_transaction.size(),
                                                     (const char *) 0, 0,
                                                     packed_provided_approvals.data(), packed_provided_approvals.size()
        );
        check(res > 0, "transaction authorization failed");

        send_deferred((uint128_t(proposer.value) << 64) | proposal_name.value, executer.value,
                      prop.packed_transaction.data(), prop.packed_transaction.size());

        proptable.erase(prop);


        fio_400_assert(transaction_size() <= MAX_TRX_SIZE, "transaction_size", std::to_string(transaction_size()),
          "Transaction is too large", ErrorTransaction);
    }

    /*****
     * this action will invalidate the specified account as part of any/all multi signature transactions.
     * when invalidated this account will no longer be considered as necessary for the completion of
     * a proposed muti signature transaction. only the owner of the specified account may call invalidate
     * for thier account. this invalidates the specified account for all proposed multi signature transactions.
     * @param account the account to invalidate
     */
    void multisig::invalidate(name account,const uint64_t &max_fee) {
        require_auth(account);
        //collect fees.
        eosio::action{
                permission_level{account, "active"_n},
                fioio::FeeContract, "mandatoryfee"_n,
                std::make_tuple(std::string("msig_invalidate"), account, max_fee)
        }.send();
        invalidations inv_table(_self, _self.value);
        auto it = inv_table.find(account.value);
        if (it == inv_table.end()) {
            inv_table.emplace(account, [&](auto &i) {
                i.account = account;
                i.last_invalidation_time = current_time_point();
            });
        } else {
            inv_table.modify(it, account, [&](auto &i) {
                i.last_invalidation_time = current_time_point();
            });
        }

        fio_400_assert(transaction_size() <= MAX_TRX_SIZE, "transaction_size", std::to_string(transaction_size()),
          "Transaction is too large", ErrorTransaction);
    }

} /// namespace eosio

EOSIO_DISPATCH( eosio::multisig, (propose)(approve)(unapprove)(cancel)(exec)
(invalidate))
