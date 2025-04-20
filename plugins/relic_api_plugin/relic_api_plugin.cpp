/**
 *  @file
 *  @copyright defined in fio/LICENSE
 */
#include <eosio/relic_api_plugin/relic_api_plugin.hpp>
#include <eosio/chain/exceptions.hpp>

#include <fc/io/json.hpp>

namespace eosio {

    static appbase::abstract_plugin &_relic_api_plugin = app().register_plugin<relic_api_plugin>();

    using namespace eosio;

    class relic_api_plugin_impl {
    public:
        relic_api_plugin_impl(controller &db)
                : db(db) {}

        controller &db;
    };


    relic_api_plugin::relic_api_plugin() {}

    relic_api_plugin::~relic_api_plugin() {}

    void relic_api_plugin::set_program_options(options_description &, options_description &) {}

    void relic_api_plugin::plugin_initialize(const variables_map &) {}

    struct async_result_visitor : public fc::visitor<std::string> {
        template<typename T>
        std::string operator()(const T &v) const {
            return fc::json::to_string(v);
        }
    };

#define CALL(api_name, api_handle, api_namespace, call_name, http_response_code) \
{std::string("/v1/" #api_name "/" #call_name), \
   [api_handle](string, string body, url_response_callback cb) mutable { \
          try { \
             if (body.empty()) body = "{}"; \
             fc::variant result( api_handle.call_name(fc::json::from_string(body).as<api_namespace::call_name ## _params>()) ); \
             cb(http_response_code, std::move(result)); \
          } catch (...) { \
             http_plugin::handle_exception(#api_name, #call_name, body, cb); \
          } \
       }}

#define RELIC_RO_CALL(call_name, http_response_code) CALL(relic, ro_api, relic_apis::read_only, call_name, http_response_code)

    void relic_api_plugin::plugin_startup() {
         ilog("starting relic_api_plugin");
        my.reset(new relic_api_plugin_impl(app().get_plugin<chain_plugin>().chain()));
        auto ro_api = app().get_plugin<relic_plugin>().get_read_only_api();
        auto &_http_plugin = app().get_plugin<http_plugin>();
    
         ilog("starting adding ro apis");

        _http_plugin.add_api({
                                                        RELIC_RO_CALL(hello_world, 202)
                                                });
         ilog("relic api plugin done adding apis");
    }

    void relic_api_plugin::plugin_shutdown() {}

}
