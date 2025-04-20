/**
 *  @file
 *  @copyright defined in fio/LICENSE
 */
#include <eosio/relic_plugin/relic_plugin.hpp>
#include <fc/optional.hpp>
#include <atomic>

namespace fc { class variant; }

namespace eosio {

    static appbase::abstract_plugin &_relic_plugin = app().register_plugin<relic_plugin>();

    class relic_plugin_impl {
    public:
        relic_plugin_impl(chain::controller &c) : _chain(c) {}

        void connect();

        void disconnect();

       
    private:
        chain::controller &_chain;
    };

    void relic_plugin_impl::connect() {
    }

    void relic_plugin_impl::disconnect() {
    }

    relic_plugin::relic_plugin() {
        app().register_config_type<eosio::chain::db_read_mode>();
        app().register_config_type<eosio::chain::validation_mode>();
        app().register_config_type<chainbase::pinnable_mapped_file::map_mode>();
    }

    void relic_plugin::set_program_options(options_description &cli, options_description &cfg) {
    }

    void relic_plugin::plugin_initialize(const variables_map &options) {
    }

    void relic_plugin::plugin_startup() {
        ilog("relic_plugin starting up");
        my.reset(new relic_plugin_impl(app().get_plugin<chain_plugin>().chain()));
        my->connect();
    }

    void relic_plugin::plugin_shutdown() {
        my->disconnect();
        ilog("relic_plugin shutting down");
    }

    namespace relic_apis {

        read_only::hello_world_result
        read_only::hello_world(const read_only::hello_world_params &p) const {
            read_only::hello_world_result res;
            return res;
        }

    } // namespace relic_apis

} // namespace eosio
