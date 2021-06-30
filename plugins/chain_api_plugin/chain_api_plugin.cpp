/**
 *  @file
 *  @copyright defined in fio/LICENSE
 */
#include <eosio/chain_api_plugin/chain_api_plugin.hpp>
#include <eosio/chain/exceptions.hpp>

#include <fc/io/json.hpp>

namespace eosio {

    static appbase::abstract_plugin &_chain_api_plugin = app().register_plugin<chain_api_plugin>();

    using namespace eosio;

    class chain_api_plugin_impl {
    public:
        chain_api_plugin_impl(controller &db)
                : db(db) {}

        controller &db;
    };


    chain_api_plugin::chain_api_plugin() {}

    chain_api_plugin::~chain_api_plugin() {}

    void chain_api_plugin::set_program_options(options_description &, options_description &) {}

    void chain_api_plugin::plugin_initialize(const variables_map &) {}

    struct async_result_visitor : public fc::visitor<fc::variant> {
        template<typename T>
        fc::variant operator()(const T &v) const {
            return fc::variant(v);
        }
    };

#define CALL(api_name, api_handle, api_namespace, call_name, http_response_code) \
{std::string("/v1/" #api_name "/" #call_name), \
   [api_handle](string, string body, url_response_callback cb) mutable { \
          api_handle.validate(); \
          try { \
             if (body.empty()) body = "{}"; \
             fc::variant result( api_handle.call_name(fc::json::from_string(body).as<api_namespace::call_name ## _params>()) ); \
             cb(http_response_code, std::move(result)); \
          } catch (...) { \
             http_plugin::handle_exception(#api_name, #call_name, body, cb); \
          } \
       }}

#define CALL_ASYNC(api_name, api_handle, api_namespace, call_name, call_result, http_response_code) \
{std::string("/v1/" #api_name "/" #call_name), \
   [api_handle](string, string body, url_response_callback cb) mutable { \
      if (body.empty()) body = "{}"; \
      api_handle.validate(); \
      api_handle.call_name(fc::json::from_string(body).as<api_namespace::call_name ## _params>(),\
         [cb, body](const fc::static_variant<fc::exception_ptr, call_result>& result){\
            if (result.contains<fc::exception_ptr>()) {\
               try {\
                  result.get<fc::exception_ptr>()->dynamic_rethrow_exception();\
               } catch (...) {\
                  http_plugin::handle_exception(#api_name, #call_name, body, cb);\
               }\
            } else {\
               cb(http_response_code, result.visit(async_result_visitor()));\
            }\
         });\
   }\
}

#define CHAIN_RO_CALL(call_name, http_response_code) CALL(chain, ro_api, chain_apis::read_only, call_name, http_response_code)
#define CHAIN_RW_CALL(call_name, http_response_code) CALL(chain, rw_api, chain_apis::read_write, call_name, http_response_code)
#define CHAIN_RO_CALL_ASYNC(call_name, call_result, http_response_code) CALL_ASYNC(chain, ro_api, chain_apis::read_only, call_name, call_result, http_response_code)
#define CHAIN_RW_CALL_ASYNC(call_name, call_result, http_response_code) CALL_ASYNC(chain, rw_api, chain_apis::read_write, call_name, call_result, http_response_code)

    void chain_api_plugin::plugin_startup() {
        ilog("starting chain_api_plugin");
        my.reset(new chain_api_plugin_impl(app().get_plugin<chain_plugin>().chain()));
        auto ro_api = app().get_plugin<chain_plugin>().get_read_only_api();
        auto rw_api = app().get_plugin<chain_plugin>().get_read_write_api();

        auto &_http_plugin = app().get_plugin<http_plugin>();
        ro_api.set_shorten_abi_errors(!_http_plugin.verbose_errors());

        _http_plugin.add_api({
                                     CHAIN_RO_CALL(get_info, 200l),
                                     CHAIN_RO_CALL(get_activated_protocol_features, 200),
                                     CHAIN_RO_CALL(get_block, 200),
                                     CHAIN_RO_CALL(get_block_header_state, 200),
                                     CHAIN_RO_CALL(get_account, 200),
                                     CHAIN_RO_CALL(get_code, 200),
                                     CHAIN_RO_CALL(get_code_hash, 200),
                                     CHAIN_RO_CALL(get_abi, 200),
                                     CHAIN_RO_CALL(get_raw_code_and_abi, 200),
                                     CHAIN_RO_CALL(get_raw_abi, 200),
                                     CHAIN_RO_CALL(get_table_rows, 200),
                                     CHAIN_RO_CALL(get_table_by_scope, 200),
                                     CHAIN_RO_CALL(get_currency_balance, 200),
                                     CHAIN_RO_CALL(get_currency_stats, 200),
                                     CHAIN_RO_CALL(get_producers, 200),
                                     CHAIN_RO_CALL(get_producer_schedule, 200),
                                     CHAIN_RO_CALL(get_scheduled_transactions, 200),
                                     CHAIN_RO_CALL(abi_json_to_bin, 200),
                                     CHAIN_RO_CALL(abi_bin_to_json, 200),
                                     CHAIN_RO_CALL(get_required_keys, 200),
                                     CHAIN_RO_CALL(get_transaction_id, 200),
                                     CHAIN_RO_CALL(get_fio_balance, 200),
                                     CHAIN_RO_CALL(get_actor, 200),
                                     CHAIN_RO_CALL(get_fio_names, 200),
                                     CHAIN_RO_CALL(get_fio_domains, 200),
                                     CHAIN_RO_CALL(get_fio_addresses, 200),
                                     CHAIN_RO_CALL(get_oracle_fees, 200),
                                     CHAIN_RO_CALL(get_locks, 200),
                                     CHAIN_RO_CALL(get_fee, 200),
                                     CHAIN_RO_CALL(get_actions, 200),
                                     CHAIN_RO_CALL(avail_check, 200),
                                     CHAIN_RO_CALL(serialize_json, 200),
                                     CHAIN_RO_CALL(get_pub_address, 200),
                                     CHAIN_RO_CALL(get_pub_addresses, 200),
                                     CHAIN_RO_CALL(get_pending_fio_requests, 200),
                                     CHAIN_RO_CALL(get_received_fio_requests, 200),
                                     CHAIN_RO_CALL(get_cancelled_fio_requests, 200),
                                     CHAIN_RO_CALL(get_obt_data, 200),
                                     CHAIN_RO_CALL(get_whitelist, 200),
                                     CHAIN_RO_CALL(check_whitelist, 200),
                                     CHAIN_RO_CALL(get_sent_fio_requests, 200),
                                     CHAIN_RW_CALL_ASYNC(push_block, chain_apis::read_write::push_block_results, 202),
                                     CHAIN_RW_CALL_ASYNC(push_transaction,
                                                         chain_apis::read_write::push_transaction_results, 202),
                                     CHAIN_RW_CALL_ASYNC(push_transactions,
                                                         chain_apis::read_write::push_transactions_results, 202),
                                     CHAIN_RW_CALL_ASYNC(send_transaction,
                                                         chain_apis::read_write::send_transaction_results, 202),
                                     CHAIN_RW_CALL_ASYNC(register_fio_address,
                                                         chain_apis::read_write::register_fio_address_results, 202),
                                     CHAIN_RW_CALL_ASYNC(register_fio_domain,
                                                         chain_apis::read_write::register_fio_domain_results, 202),
                                     CHAIN_RW_CALL_ASYNC(add_pub_address,
                                                         chain_apis::read_write::add_pub_address_results, 202),
                                     CHAIN_RW_CALL_ASYNC(remove_pub_address,
                                                         chain_apis::read_write::remove_pub_address_results, 202),
                                     CHAIN_RW_CALL_ASYNC(remove_all_pub_addresses,
                                                         chain_apis::read_write::remove_all_pub_addresses_results, 202),
                                     CHAIN_RW_CALL_ASYNC(transfer_tokens_pub_key,
                                                         chain_apis::read_write::transfer_tokens_pub_key_results, 202),
                                     CHAIN_RW_CALL_ASYNC(transfer_locked_tokens,
                                                         chain_apis::read_write::transfer_locked_tokens_results, 202),
                                     CHAIN_RW_CALL_ASYNC(burn_expired,
                                                         chain_apis::read_write::burn_expired_results, 202),
                                     CHAIN_RW_CALL_ASYNC(compute_fees,
                                                         chain_apis::read_write::compute_fees_results, 202),
                                     CHAIN_RW_CALL_ASYNC(unregister_producer,
                                                         chain_apis::read_write::unregister_producer_results, 202),
                                     CHAIN_RW_CALL_ASYNC(register_producer,
                                                         chain_apis::read_write::register_producer_results, 202),
                                     CHAIN_RW_CALL_ASYNC(unregister_proxy,
                                                         chain_apis::read_write::unregister_proxy_results, 202),
                                     CHAIN_RW_CALL_ASYNC(register_proxy,
                                                         chain_apis::read_write::register_proxy_results, 202),
                                     CHAIN_RW_CALL_ASYNC(burn_fio_address,
                                                         chain_apis::read_write::burn_fio_address_results, 202),
                                     CHAIN_RW_CALL_ASYNC(transfer_fio_domain,
                                                         chain_apis::read_write::transfer_fio_domain_results, 202),
                                     CHAIN_RW_CALL_ASYNC(transfer_fio_address,
                                                         chain_apis::read_write::transfer_fio_address_results, 202),
                                     CHAIN_RW_CALL_ASYNC(vote_producer,
                                                         chain_apis::read_write::vote_producer_results, 202),
                                     CHAIN_RW_CALL_ASYNC(proxy_vote,
                                                         chain_apis::read_write::proxy_vote_results, 202),
                                     CHAIN_RW_CALL_ASYNC(submit_bundled_transaction,
                                                         chain_apis::read_write::submit_bundled_transaction_results,
                                                         202),
                                     CHAIN_RW_CALL_ASYNC(submit_fee_ratios,
                                                         chain_apis::read_write::submit_fee_ratios_results, 202),
                                     CHAIN_RW_CALL_ASYNC(submit_fee_multiplier,
                                                         chain_apis::read_write::submit_fee_multiplier_results, 202),
                                     CHAIN_RW_CALL_ASYNC(renew_fio_domain,
                                                         chain_apis::read_write::renew_fio_domain_results, 202),
                                     CHAIN_RW_CALL_ASYNC(renew_fio_address,
                                                         chain_apis::read_write::renew_fio_address_results, 202),
                                     CHAIN_RW_CALL_ASYNC(record_obt_data,
                                                         chain_apis::read_write::record_obt_data_results, 202),
                                     CHAIN_RW_CALL_ASYNC(reject_funds_request,
                                                         chain_apis::read_write::reject_funds_request_results, 202),
                                     CHAIN_RW_CALL_ASYNC(cancel_funds_request,
                                                         chain_apis::read_write::cancel_funds_request_results, 202),
                                     CHAIN_RW_CALL_ASYNC(new_funds_request,
                                                         chain_apis::read_write::new_funds_request_results, 202),
                                     CHAIN_RW_CALL_ASYNC(set_fio_domain_public,
                                                         chain_apis::read_write::set_fio_domain_public_results, 202),
                                     CHAIN_RW_CALL_ASYNC(pay_tpid_rewards,
                                                         chain_apis::read_write::pay_tpid_rewards_results, 202),
                                     CHAIN_RW_CALL_ASYNC(claim_bp_rewards,
                                                         chain_apis::read_write::claim_bp_rewards_results, 202),
                                     CHAIN_RW_CALL_ASYNC(add_bundled_transactions,
                                                         chain_apis::read_write::add_bundled_transactions_results, 202),
                                     CHAIN_RW_CALL_ASYNC(wrap_fio_tokens,
                                                         chain_apis::read_write::wrap_fio_tokens_results, 202)
                             });
    }

    void chain_api_plugin::plugin_shutdown() {}

}
