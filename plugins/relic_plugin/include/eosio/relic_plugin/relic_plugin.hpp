/**
 *  @file
 *  @copyright defined in fio/LICENSE
 */
#pragma once

#include <appbase/application.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <fc/variant.hpp>
#include <memory>

namespace fc { class variant; }

namespace eosio {
    using namespace appbase;
    using namespace chain_apis;
    typedef std::shared_ptr<class relic_plugin_impl> relic_ptr;

    namespace relic_apis {
       
        class read_only {

        public:
            read_only(const relic_ptr &relic)
                    : my(relic) {}

             struct hello_world_params {
                int32_t blocknumber;
            };

            struct hello_world_result {
                string resultval = "hello world!!";
                   };

            hello_world_result
            hello_world(const hello_world_params &params) const;

        private:
            relic_ptr my;
        };


    } // namespace relic_apis


    class relic_plugin : public plugin<relic_plugin> {
    public:
        APPBASE_PLUGIN_REQUIRES((chain_plugin))

        relic_plugin();

        relic_plugin(const relic_plugin &) = delete;

        relic_plugin(relic_plugin &&) = delete;

        relic_plugin &operator=(const relic_plugin &) = delete;

        relic_plugin &operator=(relic_plugin &&) = delete;

        virtual ~relic_plugin() override = default;

        virtual void set_program_options(options_description &cli, options_description &cfg) override;

        void plugin_initialize(const variables_map &options);

        void plugin_startup();

        void plugin_shutdown();

        relic_apis::read_only get_read_only_api() const { return relic_apis::read_only(my); }

    private:
        relic_ptr my;
    };

}

FC_REFLECT(eosio::relic_apis::read_only::hello_world_params,
           (blocknumber))
FC_REFLECT(eosio::relic_apis::read_only::hello_world_result,
           (resultval))
