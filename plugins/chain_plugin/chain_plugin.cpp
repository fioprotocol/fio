/**
 *  @file
 *  @copyright defined in fio/LICENSE
 */
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/chain/fioio/actionmapping.hpp>
#include <eosio/chain/fork_database.hpp>
#include <eosio/chain/block_log.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/fioaction_object.hpp>
#include <eosio/chain/authorization_manager.hpp>
#include <eosio/chain/code_object.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/wasm_interface.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/reversible_block_object.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/generated_transaction_object.hpp>
#include <eosio/chain/snapshot.hpp>

#include <eosio/chain/fioio/fioerror.hpp>
#include <eosio/chain/fioio/keyops.hpp>
#include <eosio/chain/fioio/fio_common_validator.hpp>

#include <eosio/chain/eosio_contract.hpp>

#include <eosio/chain/fioio/fioserialize.h>
#include <eosio/chain/fioio/pubkey_validation.hpp>

#include <boost/signals2/connection.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <fc/io/json.hpp>
#include <fc/variant.hpp>
#include <signal.h>
#include <cstdlib>

namespace eosio {

//declare operator<< and validate funciton for read_mode in the same namespace as read_mode itself
    namespace chain {

        std::ostream &operator<<(std::ostream &osm, eosio::chain::db_read_mode m) {
            if (m == eosio::chain::db_read_mode::SPECULATIVE) {
                osm << "speculative";
            } else if (m == eosio::chain::db_read_mode::HEAD) {
                osm << "head";
            } else if (m == eosio::chain::db_read_mode::READ_ONLY) {
                osm << "read-only";
            } else if (m == eosio::chain::db_read_mode::IRREVERSIBLE) {
                osm << "irreversible";
            }

            return osm;
        }

        void validate(boost::any &v,
                      const std::vector<std::string> &values,
                      eosio::chain::db_read_mode * /* target_type */,
                      int) {
            using namespace boost::program_options;

            // Make sure no previous assignment to 'v' was made.
            validators::check_first_occurrence(v);

            // Extract the first string from 'values'. If there is more than
            // one string, it's an error, and exception will be thrown.
            std::string const &s = validators::get_single_string(values);

            if (s == "speculative") {
                v = boost::any(eosio::chain::db_read_mode::SPECULATIVE);
            } else if (s == "head") {
                v = boost::any(eosio::chain::db_read_mode::HEAD);
            } else if (s == "read-only") {
                v = boost::any(eosio::chain::db_read_mode::READ_ONLY);
            } else if (s == "irreversible") {
                v = boost::any(eosio::chain::db_read_mode::IRREVERSIBLE);
            } else {
                throw validation_error(validation_error::invalid_option_value);
            }
        }

        std::ostream &operator<<(std::ostream &osm, eosio::chain::validation_mode m) {
            if (m == eosio::chain::validation_mode::FULL) {
                osm << "full";
            } else if (m == eosio::chain::validation_mode::LIGHT) {
                osm << "light";
            }

            return osm;
        }

        void validate(boost::any &v,
                      const std::vector<std::string> &values,
                      eosio::chain::validation_mode * /* target_type */,
                      int) {
            using namespace boost::program_options;

            // Make sure no previous assignment to 'v' was made.
            validators::check_first_occurrence(v);

            // Extract the first string from 'values'. If there is more than
            // one string, it's an error, and exception will be thrown.
            std::string const &s = validators::get_single_string(values);

            if (s == "full") {
                v = boost::any(eosio::chain::validation_mode::FULL);
            } else if (s == "light") {
                v = boost::any(eosio::chain::validation_mode::LIGHT);
            } else {
                throw validation_error(validation_error::invalid_option_value);
            }
        }

    }

    using namespace eosio;
    using namespace eosio::chain;
    using namespace eosio::chain::config;
    using namespace eosio::chain::plugin_interface;
    using vm_type = wasm_interface::vm_type;
    using fc::flat_map;

    using boost::signals2::scoped_connection;

//using txn_msg_rate_limits = controller::txn_msg_rate_limits;

#define CATCH_AND_CALL(NEXT)\
   catch ( const fc::exception& err ) {\
      NEXT(err.dynamic_copy_exception());\
   } catch ( const std::exception& e ) {\
      fc::exception fce( \
         FC_LOG_MESSAGE( warn, "rethrow ${what}: ", ("what",e.what())),\
         fc::std_exception_code,\
         BOOST_CORE_TYPEID(e).name(),\
         e.what() ) ;\
      NEXT(fce.dynamic_copy_exception());\
   } catch( ... ) {\
      fc::unhandled_exception e(\
         FC_LOG_MESSAGE(warn, "rethrow"),\
         std::current_exception());\
      NEXT(e.dynamic_copy_exception());\
   }


    class chain_plugin_impl {
    public:
        chain_plugin_impl()
                : pre_accepted_block_channel(app().get_channel<channels::pre_accepted_block>()),
                  accepted_block_header_channel(app().get_channel<channels::accepted_block_header>()),
                  accepted_block_channel(app().get_channel<channels::accepted_block>()),
                  irreversible_block_channel(app().get_channel<channels::irreversible_block>()),
                  accepted_transaction_channel(app().get_channel<channels::accepted_transaction>()),
                  applied_transaction_channel(app().get_channel<channels::applied_transaction>()),
                  incoming_block_channel(app().get_channel<incoming::channels::block>()),
                  incoming_block_sync_method(app().get_method<incoming::methods::block_sync>()),
                  incoming_transaction_async_method(app().get_method<incoming::methods::transaction_async>()) {}

        bfs::path blocks_dir;
        bool readonly = false;
        flat_map<uint32_t, block_id_type> loaded_checkpoints;

        fc::optional<fork_database> fork_db;
        fc::optional<block_log> block_logger;
        fc::optional<controller::config> chain_config;
        fc::optional<controller> chain;
        fc::optional<chain_id_type> chain_id;
        //txn_msg_rate_limits              rate_limits;
        fc::optional<vm_type> wasm_runtime;
        fc::microseconds abi_serializer_max_time_ms;
        fc::optional<bfs::path> snapshot_path;


        // retained references to channels for easy publication
        channels::pre_accepted_block::channel_type &pre_accepted_block_channel;
        channels::accepted_block_header::channel_type &accepted_block_header_channel;
        channels::accepted_block::channel_type &accepted_block_channel;
        channels::irreversible_block::channel_type &irreversible_block_channel;
        channels::accepted_transaction::channel_type &accepted_transaction_channel;
        channels::applied_transaction::channel_type &applied_transaction_channel;
        incoming::channels::block::channel_type &incoming_block_channel;

        // retained references to methods for easy calling
        incoming::methods::block_sync::method_type &incoming_block_sync_method;
        incoming::methods::transaction_async::method_type &incoming_transaction_async_method;

        // method provider handles
        methods::get_block_by_number::method_type::handle get_block_by_number_provider;
        methods::get_block_by_id::method_type::handle get_block_by_id_provider;
        methods::get_head_block_id::method_type::handle get_head_block_id_provider;
        methods::get_last_irreversible_block_number::method_type::handle get_last_irreversible_block_number_provider;

        // scoped connections for chain controller
        fc::optional<scoped_connection> pre_accepted_block_connection;
        fc::optional<scoped_connection> accepted_block_header_connection;
        fc::optional<scoped_connection> accepted_block_connection;
        fc::optional<scoped_connection> irreversible_block_connection;
        fc::optional<scoped_connection> accepted_transaction_connection;
        fc::optional<scoped_connection> applied_transaction_connection;

    };

    chain_plugin::chain_plugin()
            : my(new chain_plugin_impl()) {
        app().register_config_type<eosio::chain::db_read_mode>();
        app().register_config_type<eosio::chain::validation_mode>();
        app().register_config_type<chainbase::pinnable_mapped_file::map_mode>();
    }

    chain_plugin::~chain_plugin() {}

    void chain_plugin::set_program_options(options_description &cli, options_description &cfg) {
        cfg.add_options()
                ("blocks-dir", bpo::value<bfs::path>()->default_value("blocks"),
                 "the location of the blocks directory (absolute path or relative to application data dir)")
                ("protocol-features-dir", bpo::value<bfs::path>()->default_value("protocol_features"),
                 "the location of the protocol_features directory (absolute path or relative to application config dir)")
                ("checkpoint", bpo::value<vector<string>>()->composing(),
                 "Pairs of [BLOCK_NUM,BLOCK_ID] that should be enforced as checkpoints.")
                ("wasm-runtime", bpo::value<eosio::chain::wasm_interface::vm_type>()->value_name("wavm/wabt"),
                 "Override default WASM runtime")
                ("abi-serializer-max-time-ms",
                 bpo::value<uint32_t>()->default_value(config::default_abi_serializer_max_time_ms),
                 "Override default maximum ABI serialization time allowed in ms")
                ("chain-state-db-size-mb",
                 bpo::value<uint64_t>()->default_value(config::default_state_size / (1024 * 1024)),
                 "Maximum size (in MiB) of the chain state database")
                ("chain-state-db-guard-size-mb",
                 bpo::value<uint64_t>()->default_value(config::default_state_guard_size / (1024 * 1024)),
                 "Safely shut down node when free space remaining in the chain state database drops below this size (in MiB).")
                ("history-dir",
                 bpo::value<std::string>(),
                 "Directory containing history data")
                ("history-state-db-size-mb",
                 bpo::value<uint64_t>()->default_value(config::default_history_size / (1024  * 1024)),
                 "Maximum size (in MiB) of the history state database")
                ("history-state-db-guard-size-mb",
                 bpo::value<uint64_t>()->default_value(config::default_history_guard_size / (1024  * 1024)),
                 "Safely shut down node when free space remaining in the history state database drops below this size (in MiB).")
                ("history-index-dir",
                 bpo::value<std::string>(),
                 "Directory containing history index data")
                ("history-index-state-db-size-mb",
                 bpo::value<uint64_t>()->default_value(config::default_history_size / (1024  * 1024)),
                 "Maximum size (in MiB) of the history index state database")
                ("history-index-state-db-guard-size-mb",
                 bpo::value<uint64_t>()->default_value(config::default_history_guard_size / (1024  * 1024)),
                 "Safely shut down node when free space remaining in the history index state database drops below this size (in MiB).")
                ("reversible-blocks-db-size-mb",
                 bpo::value<uint64_t>()->default_value(config::default_reversible_cache_size / (1024 * 1024)),
                 "Maximum size (in MiB) of the reversible blocks database")
                ("reversible-blocks-db-guard-size-mb",
                 bpo::value<uint64_t>()->default_value(config::default_reversible_guard_size / (1024 * 1024)),
                 "Safely shut down node when free space remaining in the reverseible blocks database drops below this size (in MiB).")
                ("signature-cpu-billable-pct",
                 bpo::value<uint32_t>()->default_value(config::default_sig_cpu_bill_pct / config::percent_1),
                 "Percentage of actual signature recovery cpu to bill. Whole number percentages, e.g. 50 for 50%")
                ("chain-threads", bpo::value<uint16_t>()->default_value(config::default_controller_thread_pool_size),
                 "Number of worker threads in controller thread pool")
                ("contracts-console", bpo::bool_switch()->default_value(false),
                 "print contract's output to console")
                ("actor-whitelist", boost::program_options::value<vector<string>>()->composing()->multitoken(),
                 "Account added to actor whitelist (may specify multiple times)")
                ("actor-blacklist", boost::program_options::value<vector<string>>()->composing()->multitoken(),
                 "Account added to actor blacklist (may specify multiple times)")
                ("contract-whitelist", boost::program_options::value<vector<string>>()->composing()->multitoken(),
                 "Contract account added to contract whitelist (may specify multiple times)")
                ("contract-blacklist", boost::program_options::value<vector<string>>()->composing()->multitoken(),
                 "Contract account added to contract blacklist (may specify multiple times)")
                ("action-blacklist", boost::program_options::value<vector<string>>()->composing()->multitoken(),
                 "Action (in the form code::action) added to action blacklist (may specify multiple times)")
                ("key-blacklist", boost::program_options::value<vector<string>>()->composing()->multitoken(),
                 "Public key added to blacklist of keys that should not be included in authorities (may specify multiple times)")
                ("sender-bypass-whiteblacklist",
                 boost::program_options::value<vector<string>>()->composing()->multitoken(),
                 "Deferred transactions sent by accounts in this list do not have any of the subjective whitelist/blacklist checks applied to them (may specify multiple times)")
                ("read-mode", boost::program_options::value<eosio::chain::db_read_mode>()->default_value(
                        eosio::chain::db_read_mode::SPECULATIVE),
                 "Database read mode (\"speculative\", \"head\", \"read-only\", \"irreversible\").\n"
                 "In \"speculative\" mode database contains changes done up to the head block plus changes made by transactions not yet included to the blockchain.\n"
                 "In \"head\" mode database contains changes done up to the current head block.\n"
                 "In \"read-only\" mode database contains changes done up to the current head block and transactions cannot be pushed to the chain API.\n"
                 "In \"irreversible\" mode database contains changes done up to the last irreversible block and transactions cannot be pushed to the chain API.\n"
                )
                ("validation-mode", boost::program_options::value<eosio::chain::validation_mode>()->default_value(
                        eosio::chain::validation_mode::FULL),
                 "Chain validation mode (\"full\" or \"light\").\n"
                 "In \"full\" mode all incoming blocks will be fully validated.\n"
                 "In \"light\" mode all incoming blocks headers will be fully validated; transactions in those validated blocks will be trusted \n")
                ("disable-ram-billing-notify-checks", bpo::bool_switch()->default_value(false),
                 "Disable the check which subjectively fails a transaction if a contract bills more RAM to another account within the context of a notification handler (i.e. when the receiver is not the code of the action).")
                ("trusted-producer", bpo::value<vector<string>>()->composing(),
                 "Indicate a producer whose blocks headers signed by it will be fully validated, but transactions in those validated blocks will be trusted.")
                ("database-map-mode", bpo::value<chainbase::pinnable_mapped_file::map_mode>()->default_value(
                chainbase::pinnable_mapped_file::map_mode::mapped),
                 "Database map mode (\"mapped\", \"heap\", or \"locked\").\n"
                 "In \"mapped\" mode database is memory mapped as a file.\n"
                 "In \"heap\" mode database is preloaded in to swappable memory.\n"
                 #ifdef __linux__
                 "In \"locked\" mode database is preloaded, locked in to memory, and optionally can use huge pages.\n"
                 #else
                 "In \"locked\" mode database is preloaded and locked in to memory.\n"
#endif
        )
#ifdef __linux__
            ("database-hugepage-path", bpo::value<vector<string>>()->composing(), "Optional path for database hugepages when in \"locked\" mode (may specify multiple times)")
#endif
                ;

// TODO: rate limiting
        /*("per-authorized-account-transaction-msg-rate-limit-time-frame-sec", bpo::value<uint32_t>()->default_value(default_per_auth_account_time_frame_seconds),
         "The time frame, in seconds, that the per-authorized-account-transaction-msg-rate-limit is imposed over.")
        ("per-authorized-account-transaction-msg-rate-limit", bpo::value<uint32_t>()->default_value(default_per_auth_account),
         "Limits the maximum rate of transaction messages that an account is allowed each per-authorized-account-transaction-msg-rate-limit-time-frame-sec.")
         ("per-code-account-transaction-msg-rate-limit-time-frame-sec", bpo::value<uint32_t>()->default_value(default_per_code_account_time_frame_seconds),
          "The time frame, in seconds, that the per-code-account-transaction-msg-rate-limit is imposed over.")
         ("per-code-account-transaction-msg-rate-limit", bpo::value<uint32_t>()->default_value(default_per_code_account),
          "Limits the maximum rate of transaction messages that an account's code is allowed each per-code-account-transaction-msg-rate-limit-time-frame-sec.")*/

        cli.add_options()
                ("genesis-json", bpo::value<bfs::path>(), "File to read Genesis State from")
                ("genesis-timestamp", bpo::value<string>(), "override the initial timestamp in the Genesis State file")
                ("print-genesis-json", bpo::bool_switch()->default_value(false),
                 "extract genesis_state from blocks.log as JSON, print to console, and exit")
                ("extract-genesis-json", bpo::value<bfs::path>(),
                 "extract genesis_state from blocks.log as JSON, write into specified file, and exit")
                ("fix-reversible-blocks", bpo::bool_switch()->default_value(false),
                 "recovers reversible block database if that database is in a bad state")
                ("force-all-checks", bpo::bool_switch()->default_value(false),
                 "do not skip any checks that can be skipped while replaying irreversible blocks")
                ("disable-replay-opts", bpo::bool_switch()->default_value(false),
                 "disable optimizations that specifically target replay")
                ("replay-blockchain", bpo::bool_switch()->default_value(false),
                 "clear chain state database and replay all blocks")
                ("hard-replay-blockchain", bpo::bool_switch()->default_value(false),
                 "clear chain state database, recover as many blocks as possible from the block log, and then replay those blocks")
                ("delete-all-blocks", bpo::bool_switch()->default_value(false),
                 "clear chain state database and block log")
                ("truncate-at-block", bpo::value<uint32_t>()->default_value(0),
                 "stop hard replay / block log recovery at this block number (if set to non-zero number)")
                ("import-reversible-blocks", bpo::value<bfs::path>(),
                 "replace reversible block database with blocks imported from specified file and then exit")
                ("export-reversible-blocks", bpo::value<bfs::path>(),
                 "export reversible block database in portable format into specified file and then exit")
                ("snapshot", bpo::value<bfs::path>(), "File to read Snapshot State from");

    }

#define LOAD_VALUE_SET(options, name, container) \
if( options.count(name) ) { \
   const std::vector<std::string>& ops = options[name].as<std::vector<std::string>>(); \
   std::copy(ops.begin(), ops.end(), std::inserter(container, container.end())); \
}

    fc::time_point calculate_genesis_timestamp(string tstr) {
        fc::time_point genesis_timestamp;
        if (strcasecmp(tstr.c_str(), "now") == 0) {
            genesis_timestamp = fc::time_point::now();
        } else {
            genesis_timestamp = time_point::from_iso_string(tstr);
        }

        auto epoch_us = genesis_timestamp.time_since_epoch().count();
        auto diff_us = epoch_us % config::block_interval_us;
        if (diff_us > 0) {
            auto delay_us = (config::block_interval_us - diff_us);
            genesis_timestamp += fc::microseconds(delay_us);
            dlog("pausing ${us} microseconds to the next interval", ("us", delay_us));
        }

        ilog("Adjusting genesis timestamp to ${timestamp}", ("timestamp", genesis_timestamp));
        return genesis_timestamp;
    }

    void clear_directory_contents(const fc::path &p) {
        using boost::filesystem::directory_iterator;

        if (!fc::is_directory(p))
            return;

        for (directory_iterator enditr, itr{p}; itr != enditr; ++itr) {
            fc::remove_all(itr->path());
        }
    }

    void clear_chainbase_files(const fc::path &p) {
        if (!fc::is_directory(p))
            return;

        fc::remove(p / "shared_memory.bin");
        fc::remove(p / "shared_memory.meta");
    }

    optional<builtin_protocol_feature> read_builtin_protocol_feature(const fc::path &p) {
        try {
            return fc::json::from_file<builtin_protocol_feature>(p);
        } catch (const fc::exception &e) {
            wlog("problem encountered while reading '${path}':\n${details}",
                 ("path", p.generic_string())("details", e.to_detail_string()));
        } catch (...) {
            dlog("unknown problem encountered while reading '${path}'",
                 ("path", p.generic_string()));
        }
        return {};
    }

    protocol_feature_set initialize_protocol_features(const fc::path &p, bool populate_missing_builtins = true) {
        using boost::filesystem::directory_iterator;

        protocol_feature_set pfs;

        bool directory_exists = true;

        if (fc::exists(p)) {
            EOS_ASSERT(fc::is_directory(p), plugin_exception,
                       "Path to protocol-features is not a directory: ${path}",
                       ("path", p.generic_string())
            );
        } else {
            if (populate_missing_builtins)
                bfs::create_directories(p);
            else
                directory_exists = false;
        }

        auto log_recognized_protocol_feature = [](const builtin_protocol_feature &f,
                                                  const digest_type &feature_digest) {
            if (f.subjective_restrictions.enabled) {
                if (f.subjective_restrictions.preactivation_required) {
                    if (f.subjective_restrictions.earliest_allowed_activation_time == time_point{}) {
                        ilog("Support for builtin protocol feature '${codename}' (with digest of '${digest}') is enabled with preactivation required",
                             ("codename", builtin_protocol_feature_codename(f.get_codename()))
                                     ("digest", feature_digest)
                        );
                    } else {
                        ilog("Support for builtin protocol feature '${codename}' (with digest of '${digest}') is enabled with preactivation required and with an earliest allowed activation time of ${earliest_time}",
                             ("codename", builtin_protocol_feature_codename(f.get_codename()))
                                     ("digest", feature_digest)
                                     ("earliest_time", f.subjective_restrictions.earliest_allowed_activation_time)
                        );
                    }
                } else {
                    if (f.subjective_restrictions.earliest_allowed_activation_time == time_point{}) {
                        ilog("Support for builtin protocol feature '${codename}' (with digest of '${digest}') is enabled without activation restrictions",
                             ("codename", builtin_protocol_feature_codename(f.get_codename()))
                                     ("digest", feature_digest)
                        );
                    } else {
                        ilog("Support for builtin protocol feature '${codename}' (with digest of '${digest}') is enabled without preactivation required but with an earliest allowed activation time of ${earliest_time}",
                             ("codename", builtin_protocol_feature_codename(f.get_codename()))
                                     ("digest", feature_digest)
                                     ("earliest_time", f.subjective_restrictions.earliest_allowed_activation_time)
                        );
                    }
                }
            } else {
                ilog("Recognized builtin protocol feature '${codename}' (with digest of '${digest}') but support for it is not enabled",
                     ("codename", builtin_protocol_feature_codename(f.get_codename()))
                             ("digest", feature_digest)
                );
            }
        };

        map<builtin_protocol_feature_t, fc::path> found_builtin_protocol_features;
        map<digest_type, std::pair<builtin_protocol_feature, bool> > builtin_protocol_features_to_add;
        // The bool in the pair is set to true if the builtin protocol feature has already been visited to add
        map<builtin_protocol_feature_t, optional<digest_type> > visited_builtins;

        // Read all builtin protocol features
        if (directory_exists) {
            for (directory_iterator enditr, itr{p}; itr != enditr; ++itr) {
                auto file_path = itr->path();
                if (!fc::is_regular_file(file_path) || file_path.extension().generic_string().compare(".json") != 0)
                    continue;

                auto f = read_builtin_protocol_feature(file_path);

                if (!f) continue;

                auto res = found_builtin_protocol_features.emplace(f->get_codename(), file_path);

                EOS_ASSERT(res.second, plugin_exception,
                           "Builtin protocol feature '${codename}' was already included from a previous_file",
                           ("codename", builtin_protocol_feature_codename(f->get_codename()))
                                   ("current_file", file_path.generic_string())
                                   ("previous_file", res.first->second.generic_string())
                );

                const auto feature_digest = f->digest();

                builtin_protocol_features_to_add.emplace(std::piecewise_construct,
                                                         std::forward_as_tuple(feature_digest),
                                                         std::forward_as_tuple(*f, false));
            }
        }

        // Add builtin protocol features to the protocol feature manager in the right order (to satisfy dependencies)
        using itr_type = map<digest_type, std::pair<builtin_protocol_feature, bool>>::iterator;
        std::function<void(const itr_type &)> add_protocol_feature =
                [&pfs, &builtin_protocol_features_to_add, &visited_builtins, &log_recognized_protocol_feature, &add_protocol_feature](
                        const itr_type &itr) -> void {
                    if (itr->second.second) {
                        return;
                    } else {
                        itr->second.second = true;
                        visited_builtins.emplace(itr->second.first.get_codename(), itr->first);
                    }

                    for (const auto &d : itr->second.first.dependencies) {
                        auto itr2 = builtin_protocol_features_to_add.find(d);
                        if (itr2 != builtin_protocol_features_to_add.end()) {
                            add_protocol_feature(itr2);
                        }
                    }

                    pfs.add_feature(itr->second.first);

                    log_recognized_protocol_feature(itr->second.first, itr->first);
                };

        for (auto itr = builtin_protocol_features_to_add.begin();
             itr != builtin_protocol_features_to_add.end(); ++itr) {
            add_protocol_feature(itr);
        }

        auto output_protocol_feature = [&p](const builtin_protocol_feature &f, const digest_type &feature_digest) {
            static constexpr int max_tries = 10;

            string filename("BUILTIN-");
            filename += builtin_protocol_feature_codename(f.get_codename());
            filename += ".json";

            auto file_path = p / filename;

            EOS_ASSERT(!fc::exists(file_path), plugin_exception,
                       "Could not save builtin protocol feature with codename '${codename}' because a file at the following path already exists: ${path}",
                       ("codename", builtin_protocol_feature_codename(f.get_codename()))
                               ("path", file_path.generic_string())
            );

            if (fc::json::save_to_file(f, file_path)) {
                ilog("Saved default specification for builtin protocol feature '${codename}' (with digest of '${digest}') to: ${path}",
                     ("codename", builtin_protocol_feature_codename(f.get_codename()))
                             ("digest", feature_digest)
                             ("path", file_path.generic_string())
                );
            } else {
                elog("Error occurred while writing default specification for builtin protocol feature '${codename}' (with digest of '${digest}') to: ${path}",
                     ("codename", builtin_protocol_feature_codename(f.get_codename()))
                             ("digest", feature_digest)
                             ("path", file_path.generic_string())
                );
            }
        };

        std::function<digest_type(builtin_protocol_feature_t)> add_missing_builtins =
                [&pfs, &visited_builtins, &output_protocol_feature, &log_recognized_protocol_feature, &add_missing_builtins, populate_missing_builtins]
                        (builtin_protocol_feature_t codename) -> digest_type {
                    auto res = visited_builtins.emplace(codename, optional<digest_type>());
                    if (!res.second) {
                        EOS_ASSERT(res.first->second, protocol_feature_exception,
                                   "invariant failure: cycle found in builtin protocol feature dependencies"
                        );
                        return *res.first->second;
                    }

                    auto f = protocol_feature_set::make_default_builtin_protocol_feature(codename,
                                                                                         [&add_missing_builtins](
                                                                                                 builtin_protocol_feature_t d) {
                                                                                             return add_missing_builtins(
                                                                                                     d);
                                                                                         });

                    if (!populate_missing_builtins)
                        f.subjective_restrictions.enabled = false;

                    const auto &pf = pfs.add_feature(f);
                    res.first->second = pf.feature_digest;

                    log_recognized_protocol_feature(f, pf.feature_digest);

                    if (populate_missing_builtins)
                        output_protocol_feature(f, pf.feature_digest);

                    return pf.feature_digest;
                };

        for (const auto &p : builtin_protocol_feature_codenames) {
            auto itr = found_builtin_protocol_features.find(p.first);
            if (itr != found_builtin_protocol_features.end()) continue;

            add_missing_builtins(p.first);
        }

        return pfs;
    }

    void chain_plugin::plugin_initialize(const variables_map &options) {

        std::cout << "      ___                       ___                 " << std::endl;
        std::cout << "     /\\__\\                     /\\  \\            " << std::endl;
        std::cout << "    /:/ _/_       ___         /::\\  \\             " << std::endl;
        std::cout << "   /:/ /\\__\\     /\\__\\       /:/\\:\\  \\       " << std::endl;
        std::cout << "  /:/ /:/  /    /:/__/      /:/  \\:\\  \\          " << std::endl;
        std::cout << " /:/_/:/  /    /::\\  \\     /:/__/ \\:\\__\\       " << std::endl;
        std::cout << " \\:\\/:/  /     \\/\\:\\  \\__  \\:\\  \\ /:/  /   " << std::endl;
        std::cout << "  \\::/__/         \\:\\/\\__\\  \\:\\  /:/  /      " << std::endl;
        std::cout << "   \\:\\  \\          \\::/  /   \\:\\/:/  /        " << std::endl;
        std::cout << "    \\:\\__\\         /:/  /     \\::/  /           " << std::endl;
        std::cout << "     \\/__/         \\/__/       \\/__/             " << std::endl;
        std::cout << "  FOUNDATION FOR INTERWALLET OPERABILITY            " << std::endl;


        ilog("initializing chain plugin");

        try {
            try {
                genesis_state gs; // Check if EOSIO_ROOT_KEY is bad
            } catch (const fc::exception &) {
                elog("EOSIO_ROOT_KEY ('${root_key}') is invalid. Recompile with a valid public key.",
                     ("root_key", genesis_state::eosio_root_key));
                throw;
            }

            my->chain_config = controller::config();

            LOAD_VALUE_SET(options, "sender-bypass-whiteblacklist", my->chain_config->sender_bypass_whiteblacklist);
            LOAD_VALUE_SET(options, "actor-whitelist", my->chain_config->actor_whitelist);
            LOAD_VALUE_SET(options, "actor-blacklist", my->chain_config->actor_blacklist);
            LOAD_VALUE_SET(options, "contract-whitelist", my->chain_config->contract_whitelist);
            LOAD_VALUE_SET(options, "contract-blacklist", my->chain_config->contract_blacklist);

            LOAD_VALUE_SET(options, "trusted-producer", my->chain_config->trusted_producers);

            if (options.count("action-blacklist")) {
                const std::vector<std::string> &acts = options["action-blacklist"].as<std::vector<std::string>>();
                auto &list = my->chain_config->action_blacklist;
                for (const auto &a : acts) {
                    auto pos = a.find("::");
                    EOS_ASSERT(pos != std::string::npos, plugin_config_exception,
                               "Invalid entry in action-blacklist: '${a}'", ("a", a));
                    account_name code(a.substr(0, pos));
                    action_name act(a.substr(pos + 2));
                    list.emplace(code.value, act.value);
                }
            }

            if (options.count("key-blacklist")) {
                const std::vector<std::string> &keys = options["key-blacklist"].as<std::vector<std::string>>();
                auto &list = my->chain_config->key_blacklist;
                for (const auto &key_str : keys) {
                    list.emplace(key_str);
                }
            }

            if (options.count("blocks-dir")) {
                auto bld = options.at("blocks-dir").as<bfs::path>();
                if (bld.is_relative())
                    my->blocks_dir = app().data_dir() / bld;
                else
                    my->blocks_dir = bld;
            }

            protocol_feature_set pfs;
            {
                fc::path protocol_features_dir;
                auto pfd = options.at("protocol-features-dir").as<bfs::path>();
                if (pfd.is_relative())
                    protocol_features_dir = app().config_dir() / pfd;
                else
                    protocol_features_dir = pfd;

                pfs = initialize_protocol_features(protocol_features_dir);
            }

            if (options.count("checkpoint")) {
                auto cps = options.at("checkpoint").as<vector<string>>();
                my->loaded_checkpoints.reserve(cps.size());
                for (const auto &cp : cps) {
                    auto item = fc::json::from_string(cp).as<std::pair<uint32_t, block_id_type>>();
                    auto itr = my->loaded_checkpoints.find(item.first);
                    if (itr != my->loaded_checkpoints.end()) {
                        EOS_ASSERT(itr->second == item.second,
                                   plugin_config_exception,
                                   "redefining existing checkpoint at block number ${num}: original: ${orig} new: ${new}",
                                   ("num", item.first)("orig", itr->second)("new", item.second)
                        );
                    } else {
                        my->loaded_checkpoints[item.first] = item.second;
                    }
                }
            }

            if (options.count("wasm-runtime"))
                my->wasm_runtime = options.at("wasm-runtime").as<vm_type>();

            if (options.count("abi-serializer-max-time-ms"))
                my->abi_serializer_max_time_ms = fc::microseconds(
                        options.at("abi-serializer-max-time-ms").as<uint32_t>() * 1000);

            my->chain_config->blocks_dir = my->blocks_dir;
            my->chain_config->state_dir = app().data_dir() / config::default_state_dir_name;
            my->chain_config->read_only = my->readonly;

            if (options.count("chain-state-db-size-mb"))
                my->chain_config->state_size = options.at("chain-state-db-size-mb").as<uint64_t>() * 1024 * 1024;

            if (options.count("chain-state-db-guard-size-mb"))
                my->chain_config->state_guard_size =
                        options.at("chain-state-db-guard-size-mb").as<uint64_t>() * 1024 * 1024;

            if( options.count( "history-dir" ) ) {
                // Workaround for 10+ year old Boost defect
                // See https://svn.boost.org/trac10/ticket/8535
                // Should be .as<bfs::path>() but paths with escaped spaces break bpo e.g.
                // std::exception::what: the argument ('/path/with/white\ space') for option '--history-dir' is invalid
                auto workaround = options["history-dir"].as<std::string>();
                bfs::path h_dir = workaround;
                if( h_dir.is_relative() )
                   h_dir = bfs::current_path() / h_dir;
                my->chain_config->history_dir = h_dir / config::default_history_dir_name;
            }

            if( options.count( "history-state-db-size-mb" ))
                my->chain_config->history_size = options.at( "history-state-db-size-mb" ).as<uint64_t>() * 1024 * 1024;

            if( options.count( "history-state-db-guard-size-mb" ))
                my->chain_config->history_guard_size = options.at( "history-state-db-guard-size-mb" ).as<uint64_t>() * 1024 * 1024;

            if( options.count( "history-index-dir" ) ) {
                auto workaround = options["history-index-dir"].as<std::string>();
                bfs::path hi_dir = workaround;
                if( hi_dir.is_relative() )
                  hi_dir = bfs::current_path() / hi_dir;
                my->chain_config->history_index_dir = hi_dir / config::default_history_index_dir_name;
            }

            if( options.count( "history-index-state-db-size-mb" ))
                my->chain_config->history_index_size = options.at( "history-index-state-db-size-mb" ).as<uint64_t>() * 1024 * 1024;

            if( options.count( "history-index-state-db-guard-size-mb" ))
                my->chain_config->history_index_guard_size = options.at( "history-index-state-db-guard-size-mb" ).as<uint64_t>() * 1024 * 1024;

            if (options.count("reversible-blocks-db-size-mb"))
                my->chain_config->reversible_cache_size =
                        options.at("reversible-blocks-db-size-mb").as<uint64_t>() * 1024 * 1024;

            if (options.count("reversible-blocks-db-guard-size-mb"))
                my->chain_config->reversible_guard_size =
                        options.at("reversible-blocks-db-guard-size-mb").as<uint64_t>() * 1024 * 1024;

            if (options.count("chain-threads")) {
              my->chain_config->thread_pool_size = options.at("chain-threads").as<uint16_t>();
                EOS_ASSERT(my->chain_config->thread_pool_size > 0, plugin_config_exception,
                           "chain-threads ${num} must be greater than 0", ("num", my->chain_config->thread_pool_size));
            }

            my->chain_config->sig_cpu_bill_pct = options.at("signature-cpu-billable-pct").as<uint32_t>();
            EOS_ASSERT(my->chain_config->sig_cpu_bill_pct >= 0 && my->chain_config->sig_cpu_bill_pct <= 100,
                       plugin_config_exception,
                       "signature-cpu-billable-pct must be 0 - 100, ${pct}",
                       ("pct", my->chain_config->sig_cpu_bill_pct));
            my->chain_config->sig_cpu_bill_pct *= config::percent_1;

            if (my->wasm_runtime)
                my->chain_config->wasm_runtime = *my->wasm_runtime;

            my->chain_config->force_all_checks = options.at("force-all-checks").as<bool>();
            my->chain_config->disable_replay_opts = options.at("disable-replay-opts").as<bool>();
            my->chain_config->contracts_console = options.at("contracts-console").as<bool>();
            my->chain_config->allow_ram_billing_in_notify = options.at("disable-ram-billing-notify-checks").as<bool>();

            if (options.count("extract-genesis-json") || options.at("print-genesis-json").as<bool>()) {
                genesis_state gs;

                if (fc::exists(my->blocks_dir / "blocks.log")) {
                    gs = block_log::extract_genesis_state(my->blocks_dir);
                } else {
                    wlog("No blocks.log found at '${p}'. Using default genesis state.",
                         ("p", (my->blocks_dir / "blocks.log").generic_string()));
                }

                if (options.at("print-genesis-json").as<bool>()) {
                    ilog("Genesis JSON:\n${genesis}", ("genesis", json::to_pretty_string(gs)));
                }

                if (options.count("extract-genesis-json")) {
                    auto p = options.at("extract-genesis-json").as<bfs::path>();

                    if (p.is_relative()) {
                        p = bfs::current_path() / p;
                    }

                    EOS_ASSERT(fc::json::save_to_file(gs, p, true),
                               misc_exception,
                               "Error occurred while writing genesis JSON to '${path}'",
                               ("path", p.generic_string())
                    );

                    ilog("Saved genesis JSON to '${path}'", ("path", p.generic_string()));
                }

                EOS_THROW(extract_genesis_state_exception, "extracted genesis state from blocks.log");
            }

            if (options.count("export-reversible-blocks")) {
                auto p = options.at("export-reversible-blocks").as<bfs::path>();

                if (p.is_relative()) {
                    p = bfs::current_path() / p;
                }

                if (export_reversible_blocks(my->chain_config->blocks_dir / config::reversible_blocks_dir_name, p))
                    ilog("Saved all blocks from reversible block database into '${path}'",
                         ("path", p.generic_string()));
                else
                    ilog("Saved recovered blocks from reversible block database into '${path}'",
                         ("path", p.generic_string()));

                EOS_THROW(node_management_success, "exported reversible blocks");
            }

            if (options.at("delete-all-blocks").as<bool>()) {
                ilog("Deleting state database and blocks");
                if (options.at("truncate-at-block").as<uint32_t>() > 0)
                    wlog("The --truncate-at-block option does not make sense when deleting all blocks.");
                clear_directory_contents(my->chain_config->state_dir);
                fc::remove_all(my->blocks_dir);
            } else if (options.at("hard-replay-blockchain").as<bool>()) {
                ilog("Hard replay requested: deleting state database");
                clear_directory_contents(my->chain_config->state_dir);
                auto backup_dir = block_log::repair_log(my->blocks_dir, options.at("truncate-at-block").as<uint32_t>());
                if (fc::exists(backup_dir / config::reversible_blocks_dir_name) ||
                    options.at("fix-reversible-blocks").as<bool>()) {
                    // Do not try to recover reversible blocks if the directory does not exist, unless the option was explicitly provided.
                    if (!recover_reversible_blocks(backup_dir / config::reversible_blocks_dir_name,
                                                   my->chain_config->reversible_cache_size,
                                                   my->chain_config->blocks_dir / config::reversible_blocks_dir_name,
                                                   options.at("truncate-at-block").as<uint32_t>())) {
                        ilog("Reversible blocks database was not corrupted. Copying from backup to blocks directory.");
                        fc::copy(backup_dir / config::reversible_blocks_dir_name,
                                 my->chain_config->blocks_dir / config::reversible_blocks_dir_name);
                        fc::copy(backup_dir / config::reversible_blocks_dir_name / "shared_memory.bin",
                                 my->chain_config->blocks_dir / config::reversible_blocks_dir_name /
                                 "shared_memory.bin");
                    }
                }
            } else if (options.at("replay-blockchain").as<bool>()) {
                ilog("Replay requested: deleting state database");
                if (options.at("truncate-at-block").as<uint32_t>() > 0)
                    wlog("The --truncate-at-block option does not work for a regular replay of the blockchain.");
                clear_chainbase_files(my->chain_config->state_dir);
                if (options.at("fix-reversible-blocks").as<bool>()) {
                    if (!recover_reversible_blocks(my->chain_config->blocks_dir / config::reversible_blocks_dir_name,
                                                   my->chain_config->reversible_cache_size)) {
                        ilog("Reversible blocks database was not corrupted.");
                    }
                }
            } else if (options.at("fix-reversible-blocks").as<bool>()) {
                if (!recover_reversible_blocks(my->chain_config->blocks_dir / config::reversible_blocks_dir_name,
                                               my->chain_config->reversible_cache_size,
                                               optional<fc::path>(),
                                               options.at("truncate-at-block").as<uint32_t>())) {
                    ilog("Reversible blocks database verified to not be corrupted. Now exiting...");
                } else {
                    ilog("Exiting after fixing reversible blocks database...");
                }
                EOS_THROW(fixed_reversible_db_exception, "fixed corrupted reversible blocks database");
            } else if (options.at("truncate-at-block").as<uint32_t>() > 0) {
                wlog("The --truncate-at-block option can only be used with --fix-reversible-blocks without a replay or with --hard-replay-blockchain.");
            } else if (options.count("import-reversible-blocks")) {
                auto reversible_blocks_file = options.at("import-reversible-blocks").as<bfs::path>();
                ilog("Importing reversible blocks from '${file}'", ("file", reversible_blocks_file.generic_string()));
                fc::remove_all(my->chain_config->blocks_dir / config::reversible_blocks_dir_name);

                import_reversible_blocks(my->chain_config->blocks_dir / config::reversible_blocks_dir_name,
                                         my->chain_config->reversible_cache_size, reversible_blocks_file);

                EOS_THROW(node_management_success, "imported reversible blocks");
            }

            if (options.count("import-reversible-blocks")) {
                wlog("The --import-reversible-blocks option should be used by itself.");
            }

            if (options.count("snapshot")) {
                my->snapshot_path = options.at("snapshot").as<bfs::path>();
                EOS_ASSERT(fc::exists(*my->snapshot_path), plugin_config_exception,
                           "Cannot load snapshot, ${name} does not exist",
                           ("name", my->snapshot_path->generic_string()));

                // recover genesis information from the snapshot
                auto infile = std::ifstream(my->snapshot_path->generic_string(), (std::ios::in | std::ios::binary));
                auto reader = std::make_shared<istream_snapshot_reader>(infile);
                reader->validate();
                reader->read_section<genesis_state>([this](auto &section) {
                    section.read_row(my->chain_config->genesis);
                });
                infile.close();

                EOS_ASSERT(options.count("genesis-timestamp") == 0,
                           plugin_config_exception,
                           "--snapshot is incompatible with --genesis-timestamp as the snapshot contains genesis information");

                if (options.count("genesis-json")) {
                    auto genesis_path = options.at("genesis-json").as<bfs::path>();
                    if (genesis_path.is_relative()) {
                        genesis_path = bfs::current_path() / genesis_path;
                    }
                    EOS_ASSERT(fc::is_regular_file(genesis_path),
                               plugin_config_exception,
                               "Specified genesis file '${genesis}' does not exist.",
                               ("genesis", genesis_path.generic_string()));
                    auto genesis_file = fc::json::from_file(genesis_path).as<genesis_state>();
                    EOS_ASSERT(my->chain_config->genesis == genesis_file, plugin_config_exception,
                               "Genesis state provided via command line arguments does not match the existing genesis state in the snapshot. "
                               "It is not necessary to provide a genesis state argument when loading a snapshot."
                    );
                }
                auto shared_mem_path = my->chain_config->state_dir / "shared_memory.bin";
                EOS_ASSERT(!fc::exists(shared_mem_path),
                           plugin_config_exception,
                           "Snapshot can only be used to initialize an empty database.");

                if (fc::is_regular_file(my->blocks_dir / "blocks.log")) {
                    auto log_genesis = block_log::extract_genesis_state(my->blocks_dir);
                    EOS_ASSERT(log_genesis.compute_chain_id() == my->chain_config->genesis.compute_chain_id(),
                               plugin_config_exception,
                               "Genesis information in blocks.log does not match genesis information in the snapshot");
                }

            } else {
                bfs::path genesis_file;
                bool genesis_timestamp_specified = false;
                fc::optional<genesis_state> existing_genesis;

                if (fc::exists(my->blocks_dir / "blocks.log")) {
                    my->chain_config->genesis = block_log::extract_genesis_state(my->blocks_dir);
                    existing_genesis = my->chain_config->genesis;
                }

                if (options.count("genesis-json")) {
                    genesis_file = options.at("genesis-json").as<bfs::path>();
                    if (genesis_file.is_relative()) {
                        genesis_file = bfs::current_path() / genesis_file;
                    }

                    EOS_ASSERT(fc::is_regular_file(genesis_file),
                               plugin_config_exception,
                               "Specified genesis file '${genesis}' does not exist.",
                               ("genesis", genesis_file.generic_string()));

                    my->chain_config->genesis = fc::json::from_file(genesis_file).as<genesis_state>();
                }

                if (options.count("genesis-timestamp")) {
                    my->chain_config->genesis.initial_timestamp = calculate_genesis_timestamp(
                            options.at("genesis-timestamp").as<string>());
                    genesis_timestamp_specified = true;
                }

                if (!existing_genesis) {
                    if (!genesis_file.empty()) {
                        if (genesis_timestamp_specified) {
                            ilog("Using genesis state provided in '${genesis}' but with adjusted genesis timestamp",
                                 ("genesis", genesis_file.generic_string()));
                        } else {
                            ilog("Using genesis state provided in '${genesis}'",
                                 ("genesis", genesis_file.generic_string()));
                        }
                        wlog("Starting up fresh blockchain with provided genesis state.");
                    } else if (genesis_timestamp_specified) {
                        wlog("Starting up fresh blockchain with default genesis state but with adjusted genesis timestamp.");
                    } else {
                        wlog("Starting up fresh blockchain with default genesis state.");
                    }
                } else {
                    EOS_ASSERT(my->chain_config->genesis == *existing_genesis, plugin_config_exception,
                               "Genesis state provided via command line arguments does not match the existing genesis state in blocks.log. "
                               "It is not necessary to provide genesis state arguments when a blocks.log file already exists."
                    );
                }
            }

            if (options.count("read-mode")) {
                my->chain_config->read_mode = options.at("read-mode").as<db_read_mode>();
            }

            if (options.count("validation-mode")) {
                my->chain_config->block_validation_mode = options.at("validation-mode").as<validation_mode>();
            }

            my->chain_config->db_map_mode = options.at("database-map-mode").as<pinnable_mapped_file::map_mode>();
#ifdef __linux__
            if( options.count("database-hugepage-path") )
         my->chain_config->db_hugepage_paths = options.at("database-hugepage-path").as<std::vector<std::string>>();
#endif

            my->chain.emplace(*my->chain_config, std::move(pfs));
            my->chain_id.emplace(my->chain->get_chain_id());

            // set up method providers
            my->get_block_by_number_provider = app().get_method<methods::get_block_by_number>().register_provider(
                    [this](uint32_t block_num) -> signed_block_ptr {
                        return my->chain->fetch_block_by_number(block_num);
                    });

            my->get_block_by_id_provider = app().get_method<methods::get_block_by_id>().register_provider(
                    [this](block_id_type id) -> signed_block_ptr {
                        return my->chain->fetch_block_by_id(id);
                    });

            my->get_head_block_id_provider = app().get_method<methods::get_head_block_id>().register_provider([this]() {
                return my->chain->head_block_id();
            });

            my->get_last_irreversible_block_number_provider = app().get_method<methods::get_last_irreversible_block_number>().register_provider(
                    [this]() {
                        return my->chain->last_irreversible_block_num();
                    });

            // relay signals to channels
            my->pre_accepted_block_connection = my->chain->pre_accepted_block.connect(
                    [this](const signed_block_ptr &blk) {
                        auto itr = my->loaded_checkpoints.find(blk->block_num());
                        if (itr != my->loaded_checkpoints.end()) {
                            auto id = blk->id();
                            EOS_ASSERT(itr->second == id, checkpoint_exception,
                                       "Checkpoint does not match for block number ${num}: expected: ${expected} actual: ${actual}",
                                       ("num", blk->block_num())("expected", itr->second)("actual", id)
                            );
                        }

                        my->pre_accepted_block_channel.publish(priority::medium, blk);
                    });

            my->accepted_block_header_connection = my->chain->accepted_block_header.connect(
                    [this](const block_state_ptr &blk) {
                        my->accepted_block_header_channel.publish(priority::medium, blk);
                    });

            my->accepted_block_connection = my->chain->accepted_block.connect([this](const block_state_ptr &blk) {
                my->accepted_block_channel.publish(priority::high, blk);
            });

            my->irreversible_block_connection = my->chain->irreversible_block.connect(
                    [this](const block_state_ptr &blk) {
                        my->irreversible_block_channel.publish(priority::low, blk);
                    });

            my->accepted_transaction_connection = my->chain->accepted_transaction.connect(
                    [this](const transaction_metadata_ptr &meta) {
                        my->accepted_transaction_channel.publish(priority::low, meta);
                    });

            my->applied_transaction_connection = my->chain->applied_transaction.connect(
                    [this](std::tuple<const transaction_trace_ptr &, const signed_transaction &> t) {
                        my->applied_transaction_channel.publish(priority::low, std::get<0>(t));
                    });

            my->chain->add_indices();
        } FC_LOG_AND_RETHROW()

    }

    void chain_plugin::plugin_startup() {
        try {
            try {
                auto shutdown = []() { return app().is_quiting(); };
                if (my->snapshot_path) {
                    auto infile = std::ifstream(my->snapshot_path->generic_string(), (std::ios::in | std::ios::binary));
                    auto reader = std::make_shared<istream_snapshot_reader>(infile);
                    my->chain->startup(shutdown, reader);
                    infile.close();
                } else {
                    my->chain->startup(shutdown);
                }
            } catch (const database_guard_exception &e) {
                log_guard_exception(e);
                // make sure to properly close the db
                my->chain.reset();
                throw;
            }

            if (!my->readonly) {
                ilog("starting chain in read/write mode");
            }

            ilog("Blockchain started; head block is #${num}, genesis timestamp is ${ts}",
                 ("num", my->chain->head_block_num())("ts", (std::string) my->chain_config->genesis.initial_timestamp));

            my->chain_config.reset();
        } FC_CAPTURE_AND_RETHROW()
    }

    void chain_plugin::plugin_shutdown() {
        my->pre_accepted_block_connection.reset();
        my->accepted_block_header_connection.reset();
        my->accepted_block_connection.reset();
        my->irreversible_block_connection.reset();
        my->accepted_transaction_connection.reset();
        my->applied_transaction_connection.reset();
        if (app().is_quiting())
            my->chain->get_wasm_interface().indicate_shutting_down();
        my->chain.reset();
    }

    chain_apis::read_write::read_write(controller &db, const fc::microseconds &abi_serializer_max_time)
            : db(db), abi_serializer_max_time(abi_serializer_max_time) {
    }

    void chain_apis::read_write::validate() const {
      EOS_ASSERT( !db.in_immutable_mode(), missing_chain_api_plugin_exception,
            "Not allowed, node in read-only mode" );
    }

    void chain_plugin::accept_block(const signed_block_ptr &block) {
        my->incoming_block_sync_method(block);
    }

    void chain_plugin::accept_transaction(const chain::packed_transaction &trx,
                                          next_function<chain::transaction_trace_ptr> next) {
        my->incoming_transaction_async_method(
                std::make_shared<transaction_metadata>(std::make_shared<packed_transaction>(trx)), false,
                std::forward<decltype(next)>(next));
    }

    void chain_plugin::accept_transaction(const chain::transaction_metadata_ptr &trx,
                                          next_function<chain::transaction_trace_ptr> next) {
        my->incoming_transaction_async_method(trx, false, std::forward<decltype(next)>(next));
    }

    bool chain_plugin::block_is_on_preferred_chain(const block_id_type &block_id) {
        auto b = chain().fetch_block_by_number(block_header::num_from_id(block_id));
        return b && b->id() == block_id;
    }

    bool chain_plugin::recover_reversible_blocks(const fc::path &db_dir, uint32_t cache_size,
                                                 optional<fc::path> new_db_dir, uint32_t truncate_at_block) {
        try {
            chainbase::database reversible(db_dir, database::read_only); // Test if dirty
            // If it reaches here, then the reversible database is not dirty

            if (truncate_at_block == 0)
                return false;

            reversible.add_index<reversible_block_index>();
            const auto &ubi = reversible.get_index<reversible_block_index, by_num>();

            auto itr = ubi.rbegin();
            if (itr != ubi.rend() && itr->blocknum <= truncate_at_block)
                return false; // Because we are not going to be truncating the reversible database at all.
        } catch (const std::runtime_error &) {
        } catch (...) {
            throw;
        }
        // Reversible block database is dirty. So back it up (unless already moved) and then create a new one.

        auto reversible_dir = fc::canonical(db_dir);
        if (reversible_dir.filename().generic_string() == ".") {
            reversible_dir = reversible_dir.parent_path();
        }
        fc::path backup_dir;

        auto now = fc::time_point::now();

        if (new_db_dir) {
            backup_dir = reversible_dir;
            reversible_dir = *new_db_dir;
        } else {
            auto reversible_dir_name = reversible_dir.filename().generic_string();
            EOS_ASSERT(reversible_dir_name != ".", invalid_reversible_blocks_dir,
                       "Invalid path to reversible directory");
            backup_dir = reversible_dir.parent_path() / reversible_dir_name.append("-").append(now);

            EOS_ASSERT(!fc::exists(backup_dir),
                       reversible_blocks_backup_dir_exist,
                       "Cannot move existing reversible directory to already existing directory '${backup_dir}'",
                       ("backup_dir", backup_dir));

            fc::rename(reversible_dir, backup_dir);
            ilog("Moved existing reversible directory to backup location: '${new_db_dir}'", ("new_db_dir", backup_dir));
        }

        fc::create_directories(reversible_dir);

        ilog("Reconstructing '${reversible_dir}' from backed up reversible directory",
             ("reversible_dir", reversible_dir));

        chainbase::database old_reversible(backup_dir, database::read_only, 0, true);
        chainbase::database new_reversible(reversible_dir, database::read_write, cache_size);
        std::fstream reversible_blocks;
        reversible_blocks.open((reversible_dir.parent_path() /
                                std::string("portable-reversible-blocks-").append(now)).generic_string().c_str(),
                               std::ios::out | std::ios::binary);

        uint32_t num = 0;
        uint32_t start = 0;
        uint32_t end = 0;
        old_reversible.add_index<reversible_block_index>();
        new_reversible.add_index<reversible_block_index>();
        const auto &ubi = old_reversible.get_index<reversible_block_index, by_num>();
        auto itr = ubi.begin();
        if (itr != ubi.end()) {
            start = itr->blocknum;
            end = start - 1;
        }
        if (truncate_at_block > 0 && start > truncate_at_block) {
            ilog("Did not recover any reversible blocks since the specified block number to stop at (${stop}) is less than first block in the reversible database (${start}).",
                 ("stop", truncate_at_block)("start", start));
            return true;
        }
        try {
            for (; itr != ubi.end(); ++itr) {
                EOS_ASSERT(itr->blocknum == end + 1, gap_in_reversible_blocks_db,
                           "gap in reversible block database between ${end} and ${blocknum}",
                           ("end", end)("blocknum", itr->blocknum)
                );
                reversible_blocks.write(itr->packedblock.data(), itr->packedblock.size());
                new_reversible.create<reversible_block_object>([&](auto &ubo) {
                    ubo.blocknum = itr->blocknum;
                    ubo.set_block(
                            itr->get_block()); // get_block and set_block rather than copying the packed data acts as additional validation
                });
                end = itr->blocknum;
                ++num;
                if (end == truncate_at_block)
                    break;
            }
        } catch (const gap_in_reversible_blocks_db &e) {
            wlog("${details}", ("details", e.to_detail_string()));
        } catch (...) {}

        if (end == truncate_at_block)
            ilog("Stopped recovery of reversible blocks early at specified block number: ${stop}",
                 ("stop", truncate_at_block));

        if (num == 0)
            ilog("There were no recoverable blocks in the reversible block database");
        else if (num == 1)
            ilog("Recovered 1 block from reversible block database: block ${start}", ("start", start));
        else
            ilog("Recovered ${num} blocks from reversible block database: blocks ${start} to ${end}",
                 ("num", num)("start", start)("end", end));

        return true;
    }

    bool chain_plugin::import_reversible_blocks(const fc::path &reversible_dir,
                                                uint32_t cache_size,
                                                const fc::path &reversible_blocks_file) {
        std::fstream reversible_blocks;
        chainbase::database new_reversible(reversible_dir, database::read_write, cache_size);
        reversible_blocks.open(reversible_blocks_file.generic_string().c_str(), std::ios::in | std::ios::binary);

        reversible_blocks.seekg(0, std::ios::end);
        auto end_pos = reversible_blocks.tellg();
        reversible_blocks.seekg(0);

        uint32_t num = 0;
        uint32_t start = 0;
        uint32_t end = 0;
        new_reversible.add_index<reversible_block_index>();
        try {
            while (reversible_blocks.tellg() < end_pos) {
                signed_block tmp;
                fc::raw::unpack(reversible_blocks, tmp);
                num = tmp.block_num();

                if (start == 0) {
                    start = num;
                } else {
                    EOS_ASSERT(num == end + 1, gap_in_reversible_blocks_db,
                               "gap in reversible block database between ${end} and ${num}",
                               ("end", end)("num", num)
                    );
                }

                new_reversible.create<reversible_block_object>([&](auto &ubo) {
                    ubo.blocknum = num;
                    ubo.set_block(std::make_shared<signed_block>(std::move(tmp)));
                });
                end = num;
            }
        } catch (gap_in_reversible_blocks_db &e) {
            wlog("${details}", ("details", e.to_detail_string()));
            FC_RETHROW_EXCEPTION(e, warn, "rethrow");
        } catch (...) {}

        ilog("Imported blocks ${start} to ${end}", ("start", start)("end", end));

        if (num == 0 || end != num)
            return false;

        return true;
    }

    bool chain_plugin::export_reversible_blocks(const fc::path &reversible_dir,
                                                const fc::path &reversible_blocks_file) {
        chainbase::database reversible(reversible_dir, database::read_only, 0, true);
        std::fstream reversible_blocks;
        reversible_blocks.open(reversible_blocks_file.generic_string().c_str(), std::ios::out | std::ios::binary);

        uint32_t num = 0;
        uint32_t start = 0;
        uint32_t end = 0;
        reversible.add_index<reversible_block_index>();
        const auto &ubi = reversible.get_index<reversible_block_index, by_num>();
        auto itr = ubi.begin();
        if (itr != ubi.end()) {
            start = itr->blocknum;
            end = start - 1;
        }
        try {
            for (; itr != ubi.end(); ++itr) {
                EOS_ASSERT(itr->blocknum == end + 1, gap_in_reversible_blocks_db,
                           "gap in reversible block database between ${end} and ${blocknum}",
                           ("end", end)("blocknum", itr->blocknum)
                );
                signed_block tmp;
                fc::datastream<const char *> ds(itr->packedblock.data(), itr->packedblock.size());
                fc::raw::unpack(ds, tmp); // Verify that packed block has not been corrupted.
                reversible_blocks.write(itr->packedblock.data(), itr->packedblock.size());
                end = itr->blocknum;
                ++num;
            }
        } catch (const gap_in_reversible_blocks_db &e) {
            wlog("${details}", ("details", e.to_detail_string()));
        } catch (...) {}

        if (num == 0) {
            ilog("There were no recoverable blocks in the reversible block database");
            return false;
        } else if (num == 1)
            ilog("Exported 1 block from reversible block database: block ${start}", ("start", start));
        else
            ilog("Exported ${num} blocks from reversible block database: blocks ${start} to ${end}",
                 ("num", num)("start", start)("end", end));

        return (end >= start) && ((end - start + 1) == num);
    }

    controller &chain_plugin::chain() { return *my->chain; }

    const controller &chain_plugin::chain() const { return *my->chain; }

    chain::chain_id_type chain_plugin::get_chain_id() const {
        EOS_ASSERT(my->chain_id.valid(), chain_id_type_exception, "chain ID has not been initialized yet");
        return *my->chain_id;
    }

    fc::microseconds chain_plugin::get_abi_serializer_max_time() const {
        return my->abi_serializer_max_time_ms;
    }

    void chain_plugin::log_guard_exception(const chain::guard_exception &e) {
        if (e.code() == chain::database_guard_exception::code_value) {
            elog("Database has reached an unsafe level of usage, shutting down to avoid corrupting the database.  "
                 "Please increase the value set for \"chain-state-db-size-mb\" and restart the process!");
        } else if (e.code() == chain::reversible_guard_exception::code_value) {
            elog("Reversible block database has reached an unsafe level of usage, shutting down to avoid corrupting the database.  "
                 "Please increase the value set for \"reversible-blocks-db-size-mb\" and restart the process!");
        }

        dlog("Details: ${details}", ("details", e.to_detail_string()));
    }

    void chain_plugin::handle_guard_exception(const chain::guard_exception &e) {
        log_guard_exception(e);

        // quit the app
        app().quit();
    }

    void chain_plugin::handle_db_exhaustion() {
        elog("database memory exhausted: increase chain-state-db-size-mb and/or reversible-blocks-db-size-mb");
        //return 1 -- it's what programs/nodeos/main.cpp considers "BAD_ALLOC"
        std::_Exit(1);
    }

    void chain_plugin::handle_bad_alloc() {
        elog("std::bad_alloc - memory exhausted");
        //return -2 -- it's what programs/nodeos/main.cpp reports for std::exception
        std::_Exit(-2);
    }

    namespace chain_apis {

        const string read_only::KEYi64 = "i64";

        template<typename I>
        std::string itoh(I n, size_t hlen = sizeof(I) << 1) {
            static const char *digits = "0123456789abcdef";
            std::string r(hlen, '0');
            for (size_t i = 0, j = (hlen - 1) * 4; i < hlen; ++i, j -= 4)
                r[i] = digits[(n >> j) & 0x0f];
            return r;
        }

        read_only::get_info_results read_only::get_info(const read_only::get_info_params &) const {
            const auto &rm = db.get_resource_limits_manager();
            return {
                    itoh(static_cast<uint32_t>(app().version())),
                    db.get_chain_id(),
                    db.head_block_num(),
                    db.last_irreversible_block_num(),
                    db.last_irreversible_block_id(),
                    db.head_block_id(),
                    db.head_block_time(),
                    db.head_block_producer(),
                    rm.get_virtual_block_cpu_limit(),
                    rm.get_virtual_block_net_limit(),
                    rm.get_block_cpu_limit(),
                    rm.get_block_net_limit(),
                    //std::bitset<64>(db.get_dynamic_global_properties().recent_slots_filled).to_string(),
                    //__builtin_popcountll(db.get_dynamic_global_properties().recent_slots_filled) / 64.0,
                    app().version_string(),
                    db.fork_db_pending_head_block_num(),
                    db.fork_db_pending_head_block_id()
            };
        }

        read_only::get_activated_protocol_features_results
        read_only::get_activated_protocol_features(
                const read_only::get_activated_protocol_features_params &params) const {
            read_only::get_activated_protocol_features_results result;
            const auto &pfm = db.get_protocol_feature_manager();

            uint32_t lower_bound_value = std::numeric_limits<uint32_t>::lowest();
            uint32_t upper_bound_value = std::numeric_limits<uint32_t>::max();

            if (params.lower_bound) {
                lower_bound_value = *params.lower_bound;
            }

            if (params.upper_bound) {
                upper_bound_value = *params.upper_bound;
            }

            if (upper_bound_value < lower_bound_value)
                return result;

            auto walk_range = [&](auto itr, auto end_itr, auto &&convert_iterator) {
                fc::mutable_variant_object mvo;
                mvo("activation_ordinal", 0);
                mvo("activation_block_num", 0);

                auto &activation_ordinal_value = mvo["activation_ordinal"];
                auto &activation_block_num_value = mvo["activation_block_num"];

                auto cur_time = fc::time_point::now();
                auto end_time = cur_time + fc::microseconds(1000 * 10); /// 10ms max time
                for (unsigned int count = 0;
                     cur_time <= end_time && count < params.limit && itr != end_itr;
                     ++itr, cur_time = fc::time_point::now()) {
                    const auto &conv_itr = convert_iterator(itr);
                    activation_ordinal_value = conv_itr.activation_ordinal();
                    activation_block_num_value = conv_itr.activation_block_num();

                    result.activated_protocol_features.emplace_back(conv_itr->to_variant(false, &mvo));
                    ++count;
                }
                if (itr != end_itr) {
                    result.more = convert_iterator(itr).activation_ordinal();
                }
            };

            auto get_next_if_not_end = [&pfm](auto &&itr) {
                if (itr == pfm.cend()) return itr;

                ++itr;
                return itr;
            };

            auto lower = (params.search_by_block_num ? pfm.lower_bound(lower_bound_value)
                                                     : pfm.at_activation_ordinal(lower_bound_value));

            auto upper = (params.search_by_block_num ? pfm.upper_bound(lower_bound_value)
                                                     : get_next_if_not_end(
                            pfm.at_activation_ordinal(upper_bound_value)));

            if (params.reverse) {
                walk_range(std::make_reverse_iterator(upper), std::make_reverse_iterator(lower),
                           [](auto &&ritr) { return --(ritr.base()); });
            } else {
                walk_range(lower, upper, [](auto &&itr) { return itr; });
            }

            return result;
        }

        uint64_t read_only::get_table_index_name(const read_only::get_table_rows_params &p, bool &primary) {
            using boost::algorithm::starts_with;
            // see multi_index packing of index name
            const uint64_t table = p.table;
            uint64_t index = table & 0xFFFFFFFFFFFFFFF0ULL;
            EOS_ASSERT(index == table, chain::contract_table_query_exception, "Unsupported table name: ${n}",
                       ("n", p.table));

            primary = false;
            uint64_t pos = 0;
            if (p.index_position.empty() || p.index_position == "first" || p.index_position == "primary" ||
                p.index_position == "one") {
                primary = true;
            } else if (starts_with(p.index_position, "sec") || p.index_position == "two") { // second, secondary
            } else if (starts_with(p.index_position, "ter") ||
                       starts_with(p.index_position, "th")) { // tertiary, ternary, third, three
                pos = 1;
            } else if (starts_with(p.index_position, "fou")) { // four, fourth
                pos = 2;
            } else if (starts_with(p.index_position, "fi")) { // five, fifth
                pos = 3;
            } else if (starts_with(p.index_position, "six")) { // six, sixth
                pos = 4;
            } else if (starts_with(p.index_position, "sev")) { // seven, seventh
                pos = 5;
            } else if (starts_with(p.index_position, "eig")) { // eight, eighth
                pos = 6;
            } else if (starts_with(p.index_position, "nin")) { // nine, ninth
                pos = 7;
            } else if (starts_with(p.index_position, "ten")) { // ten, tenth
                pos = 8;
            } else {
                try {
                    pos = fc::to_uint64(p.index_position);
                } catch (...) {
                    EOS_ASSERT(false, chain::contract_table_query_exception, "Invalid index_position: ${p}",
                               ("p", p.index_position));
                }
                if (pos < 2) {
                    primary = true;
                    pos = 0;
                } else {
                    pos -= 2;
                }
            }
            index |= (pos & 0x000000000000000FULL);
            return index;
        }

        template<>
        uint64_t convert_to_type(const string &str, const string &desc) {

            try {
                return boost::lexical_cast<uint64_t>(str.c_str(), str.size());
            } catch (...) {}

            try {
                auto trimmed_str = str;
                boost::trim(trimmed_str);
                name s(trimmed_str);
                return s.value;
            } catch (...) {}

            if (str.find(',') != string::npos) { // fix #6274 only match formats like 4,EOS
                try {
                    auto symb = eosio::chain::symbol::from_string(str);
                    return symb.value();
                } catch (...) {}
            }

            try {
                return (eosio::chain::string_to_symbol(0, str.c_str()) >> 8);
            } catch (...) {
                EOS_ASSERT(false, chain_type_exception,
                           "Could not convert ${desc} string '${str}' to any of the following: "
                           "uint64_t, valid name, or valid symbol (with or without the precision)",
                           ("desc", desc)("str", str));
            }
        }

        template<>
        double convert_to_type(const string &str, const string &desc) {
            double val{};
            try {
                val = fc::variant(str).as<double>();
            } FC_RETHROW_EXCEPTIONS(warn, "Could not convert ${desc} string '${str}' to key type.",
                                    ("desc", desc)("str", str))

            EOS_ASSERT(!std::isnan(val), chain::contract_table_query_exception,
                       "Converted ${desc} string '${str}' to NaN which is not a permitted value for the key type",
                       ("desc", desc)("str", str));

            return val;
        }

        abi_def get_abi(const controller &db, const name &account) {
            const auto &d = db.db();
            const account_object *code_accnt = d.find<account_object, by_name>(account);
            EOS_ASSERT(code_accnt != nullptr, chain::account_query_exception, "Fail to retrieve account for ${account}",
                       ("account", account));
            abi_def abi;
            abi_serializer::to_abi(code_accnt->abi, abi);
            return abi;
        }

        string get_table_type(const abi_def &abi, const name &table_name) {
            for (const auto &t : abi.tables) {
                if (t.name == table_name) {
                    return t.index_type;
                }
            }
            EOS_ASSERT(false, chain::contract_table_query_exception, "Table ${table} is not specified in the ABI",
                       ("table", table_name));
        }
//*******************BEGIN FIO API

        const name fio_system_code = N(fio.address);    // FIO name contract account, init in the top of this class
        const string fio_system_scope = "fio.address";   // FIO name contract scope
        const name fio_reqobt_code = N(fio.reqobt);    // FIO request obt contract account, init in the top of this class
        const string fio_reqobt_scope = "fio.reqobt";   // FIO request obt contract scope
        const name fio_fee_code = N(fio.fee);    // FIO fee account, init in the top of this class
        const string fio_fee_scope = "fio.fee";   // FIO fee contract scope
        const name fio_whitelst_code = N(fio.whitelst);    // FIO whitelst account, init in the top of this class
        const string fio_whitelst_scope = "fio.whitelst";   // FIO whitelst contract scope
        const name fio_code = N(eosio);    // FIO system account
        const string fio_scope = "eosio";   // FIO system scope
        const name fio_staking_code = N(fio.staking);    // FIO system account
        const string fio_staking_scope = "fio.staking";   // FIO system scope
        const name fio_escrow_code = N(fio.escrow); // FIO Escrow account
        const string fio_escrow_scope = "fio.escrow";
        const name fio_oracle_code = N(fio.oracle); // Oracle code
        const string fio_oracle_scope = "fio.oracle";   // Oracle scope
        const name fio_perms_code = N(fio.perms);
        const string fio_perms_scope = "fio.perms";

        const name fio_whitelist_table = N(whitelist); // FIO Address Table
        const name fio_address_table = N(fionames); // FIO Address Table
        const name fio_fees_table = N(fiofees); // FIO fees Table
        const name fio_domains_table = N(domains); // FIO Domains Table
        const name fio_accounts_table = N(accountmap); // FIO Chains Table
        const name fio_locks_table = N(locktokensv2); // FIO locktokens Table
        const name fio_mainnet_locks_table = N(lockedtokens); // FIO lockedtokens Table
        const name fio_accountstake_table = N(accountstake); // FIO locktokens Table
        const name fio_oracles_table = N(oracless); // FIO Registered Oracles
        const name fio_permissions_table = N(permissions);
        const name fio_accesses_table = N(accesses);

        const uint16_t FEEMAXLENGTH = 32;
        const uint16_t FIOPUBLICKEYLENGTH = 53;

        /***
      * get locks.
      * @param p Input fio_public_key, the public key of the user..
      * @return lock info on match.
      */
        read_only::get_locks_result
        read_only::get_locks(const read_only::get_locks_params &p) const {
            string fioKey = p.fio_public_key;
            FIO_400_ASSERT(fioio::isPubKeyValid(fioKey), "fio_public_key", p.fio_public_key.c_str(),
                           "Invalid FIO Public Key",
                           fioio::ErrorPubKeyValid);

            string account_name;
            fioio::key_to_account(fioKey, account_name);

            name account = name{account_name};
            get_locks_result result;

            const abi_def system_abi = eosio::chain_apis::get_abi(db, fio_code);

            get_table_rows_params table_row_params = get_table_rows_params{
                    .json        = true,
                    .code        = fio_code,
                    .scope       = fio_scope,
                    .table       = fio_locks_table,
                    .lower_bound = boost::lexical_cast<string>(account.value),
                    .upper_bound = boost::lexical_cast<string>(account.value),
                    .key_type       = "i64",
                    .index_position = "2"};

            get_table_rows_result rows_result = get_table_rows_by_seckey<index64_index, uint64_t>(
                    table_row_params, system_abi, [](uint64_t v) -> uint64_t {
                        return v;
                    });

            FIO_404_ASSERT(!rows_result.rows.empty(), "No lock tokens in account",
                           fioio::ErrorNoGeneralLocksFound);
            FIO_404_ASSERT(rows_result.rows.size() == 1, "Unexpected number of results found",
                           fioio::ErrorUnexpectedNumberResults);

            uint64_t nowepoch = db.head_block_time().sec_since_epoch();
            uint64_t newlockamount = rows_result.rows[0]["lock_amount"].as_uint64();
            uint64_t tlockamount = 0;
            uint64_t newremaininglockamount = 0;

            uint64_t additional_available_fio_locks = 0;
            if (!rows_result.rows.empty()) {

                FIO_404_ASSERT(rows_result.rows.size() == 1, "Unexpected number of results found for main net locks",
                               fioio::ErrorUnexpectedNumberResults);

                uint64_t timestamp = rows_result.rows[0]["timestamp"].as_uint64();

                uint32_t payouts_performed = rows_result.rows[0]["payouts_performed"].as_uint64();
                uint64_t timesincelockcreated = 0;

                if (nowepoch > timestamp) {
                    timesincelockcreated = nowepoch - timestamp;
                }

                //traverse the locks and compute the amount available but not yet accounted by the system.
                //this makes the available accurate when the user has not called transfer, or vote yet
                //but has locked funds that are eligible for spending in their general lock.
                for (int i = 0; i < rows_result.rows[0]["periods"].size(); i++) {
                    uint64_t duration = rows_result.rows[0]["periods"][i]["duration"].as_uint64();
                    uint64_t amount = rows_result.rows[0]["periods"][i]["amount"].as_uint64();

                    if (duration <= timesincelockcreated) {
                        newlockamount -= amount;
                        if (i > ((int) payouts_performed - 1)) {
                            tlockamount += amount;
                        }
                    } else { //lock periods after now get added to the results.
                        lockperiodv2 lp{duration, amount};
                        result.unlock_periods.push_back(lp);
                    }

                }

                //correct the remaining lock amount to account for tokens that are unlocked before system
                //accounting is updated by calling transfer or vote.
                newremaininglockamount = rows_result.rows[0]["remaining_lock_amount"].as_uint64() - tlockamount;

            }
            result.lock_amount = newlockamount;
            result.remaining_lock_amount = newremaininglockamount;
            result.time_stamp = rows_result.rows[0]["timestamp"].as_uint64();
            result.payouts_performed = 0;
            result.can_vote = rows_result.rows[0]["can_vote"].as_uint64();

            return result;
        }


        read_only::get_actions_result
        read_only::get_actions(const read_only::get_actions_params &p) const {

            FIO_400_ASSERT(p.limit >= 0, "limit", to_string(p.limit), "Invalid limit",
                           fioio::ErrorPagingInvalid);

            FIO_400_ASSERT(p.offset >= 0, "offset", to_string(p.offset), "Invalid offset",
                           fioio::ErrorPagingInvalid);

            get_actions_result results;

            const auto &idx = db.db().get_index<fioaction_index,by_id>();
            auto itr = idx.rbegin();

            int count = 0;
            if (p.offset > 0){
                while ((itr != idx.rend()) && (count < p.offset)){
                    itr++;
                    count++;
                }
            }

            count = 0;
            while ((itr != idx.rend())){
                if (count == p.limit && p.limit != 0){
                    break;
                }
                string action = itr->actionname.to_string();
                string contract = itr->contractname;
                string timestamp = to_string(itr->blocktimestamp);

                action_record rr{action, contract, timestamp};
                results.actions.push_back(rr);
                itr++;
                count++;
            }

            count = 0;
            while ((itr != idx.rend())){
                itr++;
                count++;
            }


            FIO_404_ASSERT(!(results.actions.size() == 0), "No actions", fioio::ErrorNoFioActionsFound);
            results.more = count;
            return results;
        } // get_actions

        /***
      * get escrow listings
      * @param p status is the value of the status. status = 1: on sale, status = 2: Sold, status = 3; Cancelled
      * @return listings that are of the status in the request object
      */
        read_only::get_escrow_listings_result
        read_only::get_escrow_listings(const read_only::get_escrow_listings_params &p) const {

            bool actorRequired = false;

            if(p.actor.size()) {
                actorRequired = true;
            }

            FIO_400_ASSERT(p.status > 0 && p.status <= 3, "status", to_string(p.status),
                           "Invalid status value",
                           fioio::ErrorInvalidValue);

            FIO_400_ASSERT(p.limit >= 0, "limit", to_string(p.limit), "Invalid limit",
                           fioio::ErrorPagingInvalid);

            FIO_400_ASSERT(p.offset >= 0, "offset", to_string(p.offset), "Invalid offset",
                           fioio::ErrorPagingInvalid);

            get_escrow_listings_result result;
            string fio_escrow_lookup_table = "domainsales";   // table name
            const abi_def escrow_abi = eosio::chain_apis::get_abi(db, fio_escrow_code);
            uint32_t records_returned = 0;
            uint32_t records_size = 0;
            uint32_t search_offset = p.offset;

            get_table_rows_params fio_table_row_params2 = get_table_rows_params{
                    .json           = true,
                    .code           = fio_escrow_code,
                    .scope          = fio_escrow_scope,
                    .table          = fio_escrow_lookup_table,
                    .lower_bound    = to_string(p.status),
                    .upper_bound    = to_string(p.status),
                    .key_type       = "i64",
                    .index_position = "4"};

            get_table_rows_result requests_rows_result = get_table_rows_by_seckey<index64_index, uint64_t>(
                    fio_table_row_params2, escrow_abi,
                    [](uint64_t v) -> uint64_t {
                        return v;
                    });

            if (!requests_rows_result.rows.empty()) {
                uint32_t search_limit;
                auto start_time = fc::time_point::now();
                auto end_time = start_time;
                records_size = requests_rows_result.rows.size();
                search_limit = p.limit > 1000 || p.limit == 0 ? 1000 : p.limit;
                if (search_limit > records_size) { search_limit = records_size; }
                if (search_offset >= records_size) { records_size = 0; }
                FIO_404_ASSERT(!(records_size == 0), "No Escrow Listings", fioio::ErrorNoEscrowListingsFound);

                // get proper count for `more` value
                if(actorRequired) {
                    records_size = 0;
                    // iterate through everything
                    for (auto & row : requests_rows_result.rows) {
                        // only count record if the owner matches the actor parameter
                        if(p.actor == row["owner"].as_string()) {
                            records_size++;
                        }
                    }
                    // if no matches
                    FIO_404_ASSERT(!(records_size == 0), "No Escrow Listings", fioio::ErrorNoEscrowListingsFound);
                }

                for (size_t i = 0; i < search_limit; i++) {
                    if((i + search_offset) == requests_rows_result.rows.size()){ break; }
                    //get all the attributes of the listing
                    uint64_t id = requests_rows_result.rows[i + search_offset]["id"].as_uint64();
                    string commission_fee = requests_rows_result.rows[i + search_offset]["commission_fee"].as_string();
                    uint64_t date_listed = requests_rows_result.rows[i + search_offset]["date_listed"].as_uint64();
                    uint64_t date_updated = requests_rows_result.rows[i + search_offset]["date_updated"].as_uint64();
                    string domain = requests_rows_result.rows[i + search_offset]["domain"].as_string();
                    string owner = requests_rows_result.rows[i + search_offset]["owner"].as_string();
                    uint64_t sale_price = requests_rows_result.rows[i + search_offset]["sale_price"].as_uint64();
                    uint64_t status = requests_rows_result.rows[i + search_offset]["status"].as_uint64();

                    if ((!actorRequired) || (actorRequired && p.actor == owner)) {
                        time_t created_temptime;
                        time_t updated_temptime;
                        struct tm *created_timeinfo;
                        struct tm *updated_timeinfo;
                        char created_buffer[80];
                        char updated_buffer[80];

                        created_temptime = date_listed;
                        created_timeinfo = gmtime(&created_temptime);
                        strftime(created_buffer, 80, "%Y-%m-%dT%T", created_timeinfo);

                        updated_temptime = date_updated;
                        updated_timeinfo = gmtime(&updated_temptime);
                        strftime(updated_buffer, 80, "%Y-%m-%dT%T", updated_timeinfo);

                        listing_record rr{id, commission_fee, created_buffer, updated_buffer, domain,
                                 owner, sale_price, status};

                        result.listings.push_back(rr);
                        records_returned++;
                        end_time = fc::time_point::now();
                        if (end_time - start_time > fc::microseconds(100000)) {
                            result.time_limit_exceeded_error = true;
                            break;
                        }
                    }
                }
            }
            FIO_404_ASSERT(!(result.listings.size() == 0), "No Escrow Listings", fioio::ErrorNoEscrowListingsFound);
            result.more = records_size - records_returned - search_offset;
            // fix uint overflow
            if(result.more >= requests_rows_result.rows.size()) {
                result.more = 0;
            }
            return result;
        } // get_escrow_listings

        /***
        * get pending fio requests.
        * @param p Input is FIO name(.fio_name) and chain name(.chain). .chain is allowed to be null/empty, in which case this will bea domain only lookup.
        * @return .is_registered will be true if a match is found, else false. .is_domain is true upon domain match. .address is set if found. .expiration is set upon match.
        */
        read_only::get_pending_fio_requests_result
        read_only::get_pending_fio_requests(const read_only::get_pending_fio_requests_params &p) const {
            string fioKey = p.fio_public_key;
            FIO_400_ASSERT(fioio::isPubKeyValid(fioKey), "fio_public_key", p.fio_public_key.c_str(),
                           "Invalid FIO Public Key",
                           fioio::ErrorPubKeyValid);

            FIO_400_ASSERT(p.limit >= 0, "limit", to_string(p.limit), "Invalid limit",
                           fioio::ErrorPagingInvalid);

            FIO_400_ASSERT(p.offset >= 0, "offset", to_string(p.offset), "Invalid offset",
                           fioio::ErrorPagingInvalid);

            get_pending_fio_requests_result result;
            string fio_trx_lookup_table = "fiotrxtss";   // table name
            const abi_def reqobt_abi = eosio::chain_apis::get_abi(db, fio_reqobt_code);
            uint32_t records_returned = 0;
            uint32_t records_size = 0;
            uint32_t search_offset = p.offset;

            string account_name;
            fioio::key_to_account(fioKey, account_name);
            name account = name{account_name};
            uint64_t hexstat = account.value;

            get_table_rows_params fio_table_row_params2 = get_table_rows_params{
                    .json           = true,
                    .code           = fio_reqobt_code,
                    .scope          = fio_reqobt_scope,
                    .table          = fio_trx_lookup_table,
                    .lower_bound    = boost::lexical_cast<string>(hexstat),
                    .upper_bound    = boost::lexical_cast<string>(hexstat),
                    .key_type       = "i64",
                    .index_position = "9"};

            get_table_rows_result requests_rows_result = get_table_rows_by_seckey<index64_index, uint64_t>(
                    fio_table_row_params2, reqobt_abi,
                    [](uint64_t v) -> uint64_t {
                        return v;
                    });


            if (!requests_rows_result.rows.empty()) {
                uint32_t search_limit;
                auto start_time = fc::time_point::now();
                auto end_time = start_time;
                records_size = requests_rows_result.rows.size();
                search_limit = p.limit > 1000 || p.limit == 0 ? 1000 : p.limit;
                if (search_limit > records_size) { search_limit = records_size; }
                if (search_offset >= records_size) { records_size = 0; }
                FIO_404_ASSERT(!(records_size == 0), "No pending FIO Requests", fioio::ErrorNoFioRequestsFound);

                for (size_t i = 0; i < search_limit; i++) {
                    if ((i + search_offset) == requests_rows_result.rows.size()) { break; }
                    //get all the attributes of the fio request
                    uint64_t fio_request_id = requests_rows_result.rows[i +
                                                                        search_offset]["fio_request_id"].as_uint64();
                    string payee_fio_addr = requests_rows_result.rows[i + search_offset]["payee_fio_addr"].as_string();
                    string payer_fio_addr = requests_rows_result.rows[i + search_offset]["payer_fio_addr"].as_string();
                    string content = requests_rows_result.rows[i + search_offset]["req_content"].as_string();
                    uint64_t time_stamp = requests_rows_result.rows[i + search_offset]["req_time"].as_uint64();
                    string payer_fio_public_key = requests_rows_result.rows[i + search_offset]["payer_key"].as_string();
                    string payee_fio_public_key = requests_rows_result.rows[i + search_offset]["payee_key"].as_string();

                    //get the owning account
                    string the_accountstr = requests_rows_result.rows[i + search_offset]["payer_account"].as_string();
                    name the_account = name{the_accountstr};


                    //present results where payer address owning account == the account owning the specified pub key
                    if (account == the_account) {
                        time_t temptime;
                        struct tm *timeinfo;
                        char buffer[80];

                        temptime = time_stamp;
                        timeinfo = gmtime(&temptime);
                        strftime(buffer, 80, "%Y-%m-%dT%T", timeinfo);

                        request_record rr{fio_request_id, payer_fio_addr,
                                          payee_fio_addr, payer_fio_public_key, payee_fio_public_key, content, buffer};

                        result.requests.push_back(rr);
                        records_returned++;
                        end_time = fc::time_point::now();
                        if (end_time - start_time > fc::microseconds(100000)) {
                            result.time_limit_exceeded_error = true;
                            break;
                        }
                    }
                }
            }
            FIO_404_ASSERT(!(result.requests.size() == 0), "No FIO Requests", fioio::ErrorNoFioRequestsFound);
            result.more = records_size - records_returned - search_offset;
            return result;
        } // get_pending_fio_requests

        /***
      * get cancelled fio requests.
      * @param p Input is FIO name(.fio_name) and chain name(.chain). .chain is allowed to be null/empty, in which case this will bea domain only lookup.
      * @return .is_registered will be true if a match is found, else false. .is_domain is true upon domain match. .address is set if found. .expiration is set upon match.
      */
        read_only::get_cancelled_fio_requests_result
        read_only::get_cancelled_fio_requests(const read_only::get_cancelled_fio_requests_params &p) const {
            string fioKey = p.fio_public_key;
            FIO_400_ASSERT(fioio::isPubKeyValid(fioKey), "fio_public_key", p.fio_public_key.c_str(),
                           "Invalid FIO Public Key",
                           fioio::ErrorPubKeyValid);

            FIO_400_ASSERT(p.limit >= 0, "limit", to_string(p.limit), "Invalid limit",
                           fioio::ErrorPagingInvalid);

            FIO_400_ASSERT(p.offset >= 0, "offset", to_string(p.offset), "Invalid offset",
                           fioio::ErrorPagingInvalid);

            get_cancelled_fio_requests_result result;
            string fio_trx_lookup_table = "fiotrxtss";   // table name
            const abi_def reqobt_abi = eosio::chain_apis::get_abi(db, fio_reqobt_code);
            uint32_t records_returned = 0;
            uint32_t records_size = 0;
            uint32_t search_offset = p.offset;

            string account_name;
            fioio::key_to_account(fioKey, account_name);
            name account = name{account_name};
            uint64_t hexstat = account.value + 3;

            get_table_rows_params fio_table_row_params2 = get_table_rows_params{
                    .json           = true,
                    .code           = fio_reqobt_code,
                    .scope          = fio_reqobt_scope,
                    .table          = fio_trx_lookup_table,
                    .lower_bound    = boost::lexical_cast<string>(hexstat),
                    .upper_bound    = boost::lexical_cast<string>(hexstat),
                    .key_type       = "i64",
                    .index_position = "10"};

            get_table_rows_result requests_rows_result = get_table_rows_by_seckey<index64_index, uint64_t>(
                    fio_table_row_params2, reqobt_abi,
                    [](uint64_t v) -> uint64_t {
                        return v;
                    });

            if (!requests_rows_result.rows.empty()) {
                uint32_t search_limit;
                auto start_time = fc::time_point::now();
                auto end_time = start_time;
                string status = "cancelled";
                records_size = requests_rows_result.rows.size();
                search_limit = p.limit > 1000 || p.limit == 0 ? 1000 : p.limit;
                if (search_limit > records_size) { search_limit = records_size; }
                if (search_offset >= records_size) { records_size = 0; }
                FIO_404_ASSERT(!(records_size == 0), "No FIO Requests", fioio::ErrorNoFioRequestsFound);

                for (size_t i = 0; i < search_limit; i++) {
                    if((i + search_offset) == requests_rows_result.rows.size()){ break; }
                    //get all the attributes of the fio request
                    uint64_t fio_request_id = requests_rows_result.rows[i + search_offset]["fio_request_id"].as_uint64();
                    string payee_fio_addr = requests_rows_result.rows[i + search_offset]["payee_fio_addr"].as_string();
                    string payer_fio_addr = requests_rows_result.rows[i + search_offset]["payer_fio_addr"].as_string();
                    string content = requests_rows_result.rows[i + search_offset]["req_content"].as_string();
                    string payer_fio_public_key = requests_rows_result.rows[i + search_offset]["payer_key"].as_string();
                    string payee_fio_public_key = requests_rows_result.rows[i + search_offset]["payee_key"].as_string();
                    uint64_t time_stamp = requests_rows_result.rows[i + search_offset]["req_time"].as_uint64();

                    //get the owning account
                    string the_accountstr = requests_rows_result.rows[i + search_offset]["payee_account"].as_string();
                    name the_account = name{the_accountstr};

                    //present results where payee address owning account == the account owning the specified pub key
                    if (account == the_account) {
                        time_t temptime;
                        struct tm *timeinfo;
                        char buffer[80];

                        temptime = time_stamp;
                        timeinfo = gmtime(&temptime);
                        strftime(buffer, 80, "%Y-%m-%dT%T", timeinfo);

                        request_status_record rr{fio_request_id, payer_fio_addr, payee_fio_addr, payer_fio_public_key,
                                                 payee_fio_public_key, content, buffer, status};

                        result.requests.push_back(rr);
                        records_returned++;
                        end_time = fc::time_point::now();
                        if (end_time - start_time > fc::microseconds(100000)) {
                            result.time_limit_exceeded_error = true;
                            break;
                        }
                    }
                }
            }
            FIO_404_ASSERT(!(result.requests.size() == 0), "No FIO Requests", fioio::ErrorNoFioRequestsFound);
            result.more = records_size - records_returned - search_offset;
            return result;
        } //get_cancelled_fio_requests

        /***
         * get received fio requests.
         * @param p Input is FIO name(.fio_name) and chain name(.chain). .chain is allowed to be null/empty, in which case this will bea domain only lookup.
         * @return .is_registered will be true if a match is found, else false. .is_domain is true upon domain match. .address is set if found. .expiration is set upon match.
         */
        read_only::get_received_fio_requests_result
        read_only::get_received_fio_requests(const read_only::get_received_fio_requests_params &p) const {
            string fioKey = p.fio_public_key;
            FIO_400_ASSERT(fioio::isPubKeyValid(fioKey), "fio_public_key", p.fio_public_key.c_str(),
                           "Invalid FIO Public Key",
                           fioio::ErrorPubKeyValid);

            FIO_400_ASSERT(p.limit >= 0, "limit", to_string(p.limit), "Invalid limit",
                           fioio::ErrorPagingInvalid);

            FIO_400_ASSERT(p.offset >= 0, "offset", to_string(p.offset), "Invalid offset",
                           fioio::ErrorPagingInvalid);

            get_received_fio_requests_result result;
            string fio_trx_lookup_table = "fiotrxtss";   // table name
            const abi_def reqobt_abi = eosio::chain_apis::get_abi(db, fio_reqobt_code);
            uint32_t records_returned = 0;
            uint32_t records_size = 0;
            uint32_t search_offset = p.offset;

            string account_name;
            fioio::key_to_account(fioKey, account_name);
            name account = name{account_name};
            uint64_t hexstat = account.value + true;

            get_table_rows_params fio_table_row_params2 = get_table_rows_params{
                    .json           = true,
                    .code           = fio_reqobt_code,
                    .scope          = fio_reqobt_scope,
                    .table          = fio_trx_lookup_table,
                    .lower_bound    = boost::lexical_cast<string>(hexstat),
                    .upper_bound    = boost::lexical_cast<string>(hexstat),
                    .key_type       = "i64",
                    .index_position = "13"};

            get_table_rows_result requests_rows_result = get_table_rows_by_seckey<index64_index, uint64_t>(
                    fio_table_row_params2, reqobt_abi,
                    [](uint64_t v) -> uint64_t {
                        return v;
                    });

            if (!requests_rows_result.rows.empty()) {
                uint32_t search_limit;
                auto start_time = fc::time_point::now();
                auto end_time = start_time;
                records_size = requests_rows_result.rows.size();
                search_limit = p.limit > 1000 || p.limit == 0 ? 1000 : p.limit;
                if (search_limit > records_size) { search_limit = records_size; }
                if (search_offset >= records_size) { records_size = 0; }
                FIO_404_ASSERT(!(records_size == 0), "No FIO Requests", fioio::ErrorNoFioRequestsFound);
                for (size_t i = 0; i < search_limit; i++) {
                    if((i + search_offset) == requests_rows_result.rows.size()){ break; }
                    //get all the attributes of the fio request
                    uint64_t fio_request_id = requests_rows_result.rows[i + search_offset]["fio_request_id"].as_uint64();
                    string payee_fio_addr = requests_rows_result.rows[i + search_offset]["payee_fio_addr"].as_string();
                    string payer_fio_addr = requests_rows_result.rows[i + search_offset]["payer_fio_addr"].as_string();
                    string content = requests_rows_result.rows[i + search_offset]["req_content"].as_string();
                    string payer_fio_public_key = requests_rows_result.rows[i + search_offset]["payer_key"].as_string();
                    string payee_fio_public_key = requests_rows_result.rows[i + search_offset]["payee_key"].as_string();
                    uint8_t statusint = requests_rows_result.rows[i + search_offset]["fio_data_type"].as_uint64();
                    uint64_t time_stamp = requests_rows_result.rows[i + search_offset]["req_time"].as_uint64();
                    //get the owning account
                    string the_accountstr = requests_rows_result.rows[i + search_offset]["payer_account"].as_string();
                    name the_account = name{the_accountstr};



                    //present results where payer address owning account == the account owning the specified pub key
                    if (account == the_account) {
                        string status = "requested";
                        if (statusint == 1) {
                            status = "rejected";
                        } else if (statusint == 2) {
                            status = "sent_to_blockchain";
                        } else if (statusint == 3) {
                            status = "cancelled";
                        }

                        time_t temptime;
                        struct tm *timeinfo;
                        char buffer[80];

                        temptime = time_stamp;
                        timeinfo = gmtime(&temptime);
                        strftime(buffer, 80, "%Y-%m-%dT%T", timeinfo);

                        request_status_record rr{fio_request_id, payer_fio_addr, payee_fio_addr, payer_fio_public_key,
                                                 payee_fio_public_key, content, buffer, status};

                        result.requests.push_back(rr);
                        records_returned++;
                        end_time = fc::time_point::now();
                        if (end_time - start_time > fc::microseconds(100000)) {
                            result.time_limit_exceeded_error = true;
                            break;
                        }
                    }
                }
            }
            FIO_404_ASSERT(!(result.requests.size() == 0), "No FIO Requests", fioio::ErrorNoFioRequestsFound);
            result.more = records_size - records_returned - search_offset;
            return result;
        } // get_received_fio_requests

        /***
        * get sent fio requests.
        * @param p Input is FIO name(.fio_name) and chain name(.chain). .chain is allowed to be null/empty, in which case this will bea domain only lookup.
        * @return .is_registered will be true if a match is found, else false. .is_domain is true upon domain match. .address is set if found. .expiration is set upon match.
        */
        read_only::get_sent_fio_requests_result
        read_only::get_sent_fio_requests(const read_only::get_sent_fio_requests_params &p) const {
            string fioKey = p.fio_public_key;
            FIO_400_ASSERT(fioio::isPubKeyValid(fioKey), "fio_public_key", p.fio_public_key.c_str(),
                           "Invalid FIO Public Key",
                           fioio::ErrorPubKeyValid);

            FIO_400_ASSERT(p.limit >= 0, "limit", to_string(p.limit), "Invalid limit",
                           fioio::ErrorPagingInvalid);

            FIO_400_ASSERT(p.offset >= 0, "offset", to_string(p.offset), "Invalid offset",
                           fioio::ErrorPagingInvalid);

            get_sent_fio_requests_result result;
            string fio_trx_lookup_table = "fiotrxtss";   // table name
            const abi_def reqobt_abi = eosio::chain_apis::get_abi(db, fio_reqobt_code);
            uint32_t records_returned = 0;
            uint32_t records_size = 0;
            uint32_t search_offset = p.offset;

            string account_name;
            fioio::key_to_account(fioKey, account_name);
            name account = name{account_name};
            uint64_t hexstat = account.value + true;

            get_table_rows_params fio_table_row_params2 = get_table_rows_params{
                    .json           = true,
                    .code           = fio_reqobt_code,
                    .scope          = fio_reqobt_scope,
                    .table          = fio_trx_lookup_table,
                    .lower_bound    = boost::lexical_cast<string>(hexstat),
                    .upper_bound    = boost::lexical_cast<string>(hexstat),
                    .key_type       = "i64",
                    .index_position = "14"};

            get_table_rows_result requests_rows_result = get_table_rows_by_seckey<index64_index, uint64_t>(
                    fio_table_row_params2, reqobt_abi,
                    [](uint64_t v) -> uint64_t {
                        return v;
                    });

            if (!requests_rows_result.rows.empty()) {
                uint32_t search_limit;
                auto start_time = fc::time_point::now();
                auto end_time = start_time;
                records_size = requests_rows_result.rows.size();
                search_limit = p.limit > 1000 || p.limit == 0 ? 1000 : p.limit;
                if (search_limit > records_size) { search_limit = records_size; }
                if (search_offset >= records_size) { records_size = 0; }
                FIO_404_ASSERT(!(records_size == 0), "No FIO Requests", fioio::ErrorNoFioRequestsFound);

                for (size_t i = 0; i < search_limit; i++) {
                    if((i + search_offset) == requests_rows_result.rows.size()){ break; }
                    //get all the attributes of the fio request
                    uint64_t fio_request_id = requests_rows_result.rows[i + search_offset]["fio_request_id"].as_uint64();
                    string payee_fio_addr = requests_rows_result.rows[i + search_offset]["payee_fio_addr"].as_string();
                    string payer_fio_addr = requests_rows_result.rows[i + search_offset]["payer_fio_addr"].as_string();
                    string content = requests_rows_result.rows[i + search_offset]["req_content"].as_string();
                    string payer_fio_public_key = requests_rows_result.rows[i + search_offset]["payer_key"].as_string();
                    string payee_fio_public_key = requests_rows_result.rows[i + search_offset]["payee_key"].as_string();
                    uint8_t statusint = requests_rows_result.rows[i + search_offset]["fio_data_type"].as_uint64();
                    uint64_t time_stamp = requests_rows_result.rows[i + search_offset]["req_time"].as_uint64();
                    //get the owning account
                    string the_accountstr = requests_rows_result.rows[i + search_offset]["payee_account"].as_string();
                    name the_account = name{the_accountstr};

                    //present results where payee address owning account == the account owning the specified pub key
                    if (account == the_account) {
                        string status = "requested";
                        if (statusint == 1) {
                            status = "rejected";
                        } else if (statusint == 2) {
                            status = "sent_to_blockchain";
                        } else if (statusint == 3) {
                            status = "cancelled";
                        }

                        time_t temptime;
                        struct tm *timeinfo;
                        char buffer[80];

                        temptime = time_stamp;
                        timeinfo = gmtime(&temptime);
                        strftime(buffer, 80, "%Y-%m-%dT%T", timeinfo);

                        request_status_record rr{fio_request_id, payer_fio_addr, payee_fio_addr, payer_fio_public_key,
                                                 payee_fio_public_key, content, buffer, status};

                        result.requests.push_back(rr);
                        records_returned++;
                        end_time = fc::time_point::now();
                        if (end_time - start_time > fc::microseconds(100000)) {
                            result.time_limit_exceeded_error = true;
                            break;
                        }
                    }
                }
            }
            FIO_404_ASSERT(!(result.requests.size() == 0), "No FIO Requests", fioio::ErrorNoFioRequestsFound);
            result.more = records_size - records_returned - search_offset;
            return result;
        }

        read_only::get_obt_data_result
        read_only::get_obt_data(const read_only::get_obt_data_params &p) const {
            string fioKey = p.fio_public_key;
            FIO_400_ASSERT(fioio::isPubKeyValid(fioKey), "fio_public_key", p.fio_public_key.c_str(),
                           "Invalid FIO Public Key",
                           fioio::ErrorPubKeyValid);

            FIO_400_ASSERT(p.limit >= 0, "limit", to_string(p.limit), "Invalid limit",
                           fioio::ErrorPagingInvalid);

            FIO_400_ASSERT(p.offset >= 0, "offset", to_string(p.offset), "Invalid offset",
                           fioio::ErrorPagingInvalid);

            get_obt_data_result result;
            string fio_trx_lookup_table = "fiotrxtss";   // table name
            const abi_def reqobt_abi = eosio::chain_apis::get_abi(db, fio_reqobt_code);
            uint32_t records_returned = 0;
            uint32_t records_size = 0;
            int32_t orig_offset = p.offset;

            string account_name;
            fioio::key_to_account(fioKey, account_name);
            name account = name{account_name};
            uint64_t hexstat = account.value + true;

            get_table_rows_params fio_table_row_params1 = get_table_rows_params{
                    .json           = true,
                    .code           = fio_reqobt_code,
                    .scope          = fio_reqobt_scope,
                    .table          = fio_trx_lookup_table,
                    .lower_bound    = boost::lexical_cast<string>(hexstat),
                    .upper_bound    = boost::lexical_cast<string>(hexstat),
                    .key_type       = "i64",
                    .index_position = "11"};

            get_table_rows_result requests_rows_result = get_table_rows_by_seckey<index64_index, uint64_t>(
                    fio_table_row_params1, reqobt_abi,
                    [](uint64_t v) -> uint64_t {
                        return v;
                    });

            get_table_rows_params fio_table_row_params2 = get_table_rows_params{
                    .json           = true,
                    .code           = fio_reqobt_code,
                    .scope          = fio_reqobt_scope,
                    .table          = fio_trx_lookup_table,
                    .lower_bound    = boost::lexical_cast<string>(hexstat),
                    .upper_bound    = boost::lexical_cast<string>(hexstat),
                    .key_type       = "i64",
                    .index_position = "12"};

            get_table_rows_result requests_rows_result2 = get_table_rows_by_seckey<index64_index, uint64_t>(
                    fio_table_row_params2, reqobt_abi,
                    [](uint64_t v) -> uint64_t {
                        return v;
                    });

            if (!requests_rows_result.rows.empty() || !requests_rows_result2.rows.empty()) {
                uint32_t search_limit;
                int32_t search_offset = orig_offset;
                auto start_time = fc::time_point::now();
                auto end_time = start_time;
                string status = "sent_to_blockchain";
                records_size = requests_rows_result.rows.size() + requests_rows_result2.rows.size();
                search_limit = p.limit > 1000 || p.limit == 0 ? 1000 : p.limit;
                if (search_limit > records_size) { search_limit = records_size; }
                if (search_offset >= records_size) { records_size = 0; }
                FIO_404_ASSERT(!(records_size == 0), "No FIO Requests", fioio::ErrorNoFioRequestsFound);

                uint64_t fio_request_id;
                string payee_fio_addr;
                string payer_fio_addr;
                string content;
                string payer_fio_public_key;
                string payee_fio_public_key;
                int32_t search_offset2 = 0;
                uint64_t time_stamp;
                bool wContinue = false;
                size_t j = 0;

                for (size_t i = 0; (i + j) < search_limit; i++) {
                    if ((i + search_offset) < requests_rows_result.rows.size()) {
                        fio_request_id = requests_rows_result.rows[i + search_offset]["fio_request_id"].as_uint64();
                        payee_fio_addr = requests_rows_result.rows[i + search_offset]["payee_fio_addr"].as_string();
                        payer_fio_addr = requests_rows_result.rows[i + search_offset]["payer_fio_addr"].as_string();
                        content = requests_rows_result.rows[i + search_offset]["obt_content"].as_string();
                        payer_fio_public_key = requests_rows_result.rows[i + search_offset]["payer_key"].as_string();
                        payee_fio_public_key = requests_rows_result.rows[i + search_offset]["payee_key"].as_string();
                        time_stamp = requests_rows_result.rows[i + search_offset]["obt_time"].as_uint64();
                    } else if (!wContinue) {
                        wContinue = true;
                        if( search_offset - static_cast<int>(records_returned) > 0 ) { search_offset2 = search_offset - records_returned; }
                    }
                    if(wContinue) {
                        if(requests_rows_result2.rows.size() == (j + search_offset2) ) { break; } // safety check
                        fio_request_id = requests_rows_result2.rows[j + search_offset2]["fio_request_id"].as_uint64();
                        payee_fio_addr = requests_rows_result2.rows[j + search_offset2]["payee_fio_addr"].as_string();
                        payer_fio_addr = requests_rows_result2.rows[j + search_offset2]["payer_fio_addr"].as_string();
                        content = requests_rows_result2.rows[j + search_offset2]["obt_content"].as_string();
                        payer_fio_public_key = requests_rows_result2.rows[j + search_offset2]["payer_key"].as_string();
                        payee_fio_public_key = requests_rows_result2.rows[j + search_offset2]["payee_key"].as_string();
                        time_stamp = requests_rows_result2.rows[j + search_offset2]["obt_time"].as_uint64();
                        j++;
                        i--;
                    }

                    time_t temptime;
                    struct tm *timeinfo;
                    char buffer[80];
                    temptime = time_stamp;
                    timeinfo = gmtime(&temptime);
                    strftime(buffer, 80, "%Y-%m-%dT%T", timeinfo);

                    obt_records rr{payer_fio_addr, payee_fio_addr, payer_fio_public_key,
                                   payee_fio_public_key, content, fio_request_id, buffer, status};

                    result.obt_data_records.push_back(rr);
                    records_returned++;
                    end_time = fc::time_point::now();
                    if (end_time - start_time > fc::microseconds(100000)) {
                        result.time_limit_exceeded_error = true;
                        break;
                    }
                }
            }
            FIO_404_ASSERT(!(result.obt_data_records.size() == 0), "No FIO Requests",
                           fioio::ErrorNoFioRequestsFound);
            result.more = records_size - (records_returned - orig_offset);
            return result;
        }

        read_only::get_nfts_fio_address_result read_only::get_nfts_fio_address(const read_only::get_nfts_fio_address_params &params) const {

           fioio::FioAddress fa;
           fioio::getFioAddressStruct(params.fio_address, fa);
           FIO_400_ASSERT(validateFioNameFormat(fa), "fio_address", fa.fioaddress.c_str(), "Invalid FIO Address",
                          fioio::ErrorFioNameNotReg);
           uint128_t name_hash = fioio::string_to_uint128_t(fa.fioaddress.c_str());
           FIO_400_ASSERT(params.limit >= 0, "limit", to_string(params.limit), "Invalid limit",
                          fioio::ErrorPagingInvalid);
           FIO_400_ASSERT(params.offset >= 0, "offset", to_string(params.offset), "Invalid offset",
                          fioio::ErrorPagingInvalid);



           std::string fioaddresshash = "0x";
           fioaddresshash.append(
                   fioio::to_hex_little_endian(reinterpret_cast<const char *>(&name_hash), sizeof(name_hash)));

           const abi_def abi = eosio::chain_apis::get_abi(db, fio_system_code);

           get_table_rows_params nft_table_row_params = get_table_rows_params{.json=true,
                   .code = N(fio.address),
                   .scope = "fio.address",
                   .table = N(nfts),
                   .lower_bound = fioaddresshash,
                   .upper_bound = fioaddresshash,
                   .encode_type = "hex",
                   .index_position = "2"};

           get_table_rows_result address_result = get_table_rows_by_seckey<index128_index, uint128_t>(
                   nft_table_row_params, abi, [](uint128_t v) -> uint128_t {
                       return v;
                   });

           FIO_404_ASSERT(!address_result.rows.empty(), "No NFTS are mapped", fioio::ErrorPubAddressNotFound);

           uint32_t search_limit = params.limit;
           uint32_t search_offset = params.offset;

           get_nfts_fio_address_result result;

           if (search_offset < address_result.rows.size() ) {
               int64_t remaining = address_result.rows.size() - (search_offset+search_limit);
               if (remaining < 0){
                   remaining = 0;
               }
               result.more = remaining;
               for (size_t pos = 0 + search_offset; pos < address_result.rows.size();pos++) {
                   if((search_limit > 0)&&(pos-search_offset >= search_limit)){
                       break;
                   }

                   nft_info nft = nft_info {
                    // Per FIP-27 specification, do not set fio_address member of nft for get_nfts_fio_address. Set all other members.
                    .chain_code = address_result.rows[pos]["chain_code"].as_string(),
                    .contract_address =  address_result.rows[pos]["contract_address"].as_string(),
                    .token_id = address_result.rows[pos]["token_id"].as_string(),
                    .url = address_result.rows[pos]["url"].as_string(),
                    .hash = address_result.rows[pos]["hash"].as_string(),
                    .metadata = address_result.rows[pos]["metadata"].as_string()
                   };
                   result.nfts.push_back(nft);    //pushback results in nftinfo record
                   result.more = (address_result.rows.size()-pos)-1;
               }
           }

           return result;
        }

        read_only::get_nfts_hash_result read_only::get_nfts_hash(const read_only::get_nfts_hash_params &params) const {

           FIO_400_ASSERT(!params.hash.empty() && params.hash.length() <= 64, "hash", params.hash, "Invalid NFT Hash",
                          fioio::ErrorFioNameNotReg);
           uint128_t hashedstring = fioio::string_to_uint128_t(params.hash.c_str());
           FIO_400_ASSERT(params.limit >= 0, "limit", to_string(params.limit), "Invalid limit",
                          fioio::ErrorPagingInvalid);
           FIO_400_ASSERT(params.offset >= 0, "offset", to_string(params.offset), "Invalid offset",
                          fioio::ErrorPagingInvalid);


           std::string hash = "0x";
           hash.append(
                   fioio::to_hex_little_endian(reinterpret_cast<const char *>(&hashedstring), sizeof(hashedstring)));

           const abi_def abi = eosio::chain_apis::get_abi(db, fio_system_code);

           get_table_rows_params nft_table_row_params = get_table_rows_params{.json=true,
                   .code = N(fio.address),
                   .scope = "fio.address",
                   .table = N(nfts),
                   .lower_bound = hash,
                   .upper_bound = hash,
                   .encode_type = "hex",
                   .index_position = "4"};

           get_table_rows_result hash_result = get_table_rows_by_seckey<index128_index, uint128_t>(
                   nft_table_row_params, abi, [](uint128_t v) -> uint128_t {
                       return v;
                   });

           FIO_404_ASSERT(!hash_result.rows.empty(), "No NFTS are mapped", fioio::ErrorPubAddressNotFound);

           uint32_t search_limit = params.limit;
           uint32_t search_offset = params.offset;

           get_nfts_hash_result result;

           if (search_offset < hash_result.rows.size() ) {
               int64_t remaining = hash_result.rows.size() - (search_offset+search_limit);
               if (remaining < 0){
                   remaining = 0;
               }
               result.more = remaining;
               for (size_t pos = 0 + search_offset; pos < hash_result.rows.size();pos++) {
                   if((search_limit > 0)&&(pos-search_offset >= search_limit)){
                       break;
                   }

                   nft_info nft = nft_info {
                     //optional fio_address member is initialized for this endpoint
                    .fio_address = hash_result.rows[pos]["fio_address"].as_string(),
                    .chain_code = hash_result.rows[pos]["chain_code"].as_string(),
                    .contract_address = hash_result.rows[pos]["contract_address"].as_string(),
                    .token_id = hash_result.rows[pos]["token_id"].as_string(),
                    .url = hash_result.rows[pos]["url"].as_string(),
                    .hash = hash_result.rows[pos]["hash"].as_string(),
                    .metadata = hash_result.rows[pos]["metadata"].as_string()
                   };
                   result.nfts.push_back(nft);    //pushback results in nftinfo record
                   result.more = (hash_result.rows.size()-pos)-1;
               }
           }

           return result;
        }

        read_only::get_nfts_contract_result read_only::get_nfts_contract(const read_only::get_nfts_contract_params &params) const {

           FIO_400_ASSERT(!params.chain_code.empty() && params.chain_code.length() <= 10, "chain_code", params.chain_code, "Invalid chain code",
                          fioio::ErrorFioNameNotReg);
           FIO_400_ASSERT(!params.contract_address.empty(), "contract_address", params.contract_address, "Invalid contract address",
                          fioio::ErrorFioNameNotReg);
           FIO_400_ASSERT(params.limit >= 0, "limit", to_string(params.limit), "Invalid limit",
                          fioio::ErrorPagingInvalid);
           FIO_400_ASSERT(params.offset >= 0, "offset", to_string(params.offset), "Invalid offset",
                          fioio::ErrorPagingInvalid);

           uint128_t contractaddress = fioio::string_to_uint128_t(params.contract_address.c_str());

           std::string contracthash = "0x";
           contracthash.append(
                   fioio::to_hex_little_endian(reinterpret_cast<const char *>(&contractaddress), sizeof(contractaddress)));

           const abi_def abi = eosio::chain_apis::get_abi(db, fio_system_code);

           get_table_rows_params nft_table_row_params = get_table_rows_params{.json=true,
                   .code = N(fio.address),
                   .scope = "fio.address",
                   .table = N(nfts),
                   .lower_bound = contracthash,
                   .upper_bound = contracthash,
                   .encode_type = "hex",
                   .index_position = "3"};

           get_table_rows_result contract_result = get_table_rows_by_seckey<index128_index, uint128_t>(
                   nft_table_row_params, abi, [](uint128_t v) -> uint128_t {
                       return v;
                   });

           FIO_404_ASSERT(!contract_result.rows.empty(), "No NFTS are mapped", fioio::ErrorPubAddressNotFound);

           uint32_t search_limit = params.limit;
           uint32_t search_offset = params.offset;

           get_nfts_contract_result result;

           if (search_offset < contract_result.rows.size() ) {
               int64_t remaining = contract_result.rows.size() - (search_offset+search_limit);
               if (remaining < 0){
                   remaining = 0;
               }
               result.more = remaining;
               for (size_t pos = 0 + search_offset; pos < contract_result.rows.size();pos++) {
                   if((search_limit > 0)&&(pos-search_offset >= search_limit)){
                       break;
                   }

                  if (contract_result.rows[pos]["chain_code"].as_string() == params.chain_code ) {
                    if (contract_result.rows[pos]["token_id"].as_string().empty() || contract_result.rows[pos]["token_id"].as_string() == params.token_id || params.token_id.empty()) {

                    nft_info nft = nft_info {
                      //optional fio_address member is initialized for this endpoint
                     .fio_address = contract_result.rows[pos]["fio_address"].as_string(),
                     .chain_code = contract_result.rows[pos]["chain_code"].as_string(),
                     .contract_address = contract_result.rows[pos]["contract_address"].as_string(),
                     .token_id = contract_result.rows[pos]["token_id"].as_string(),
                     .url = contract_result.rows[pos]["url"].as_string(),
                     .hash = contract_result.rows[pos]["hash"].as_string(),
                     .metadata = contract_result.rows[pos]["metadata"].as_string()
                    };
                    result.nfts.push_back(nft);    //pushback results in nftinfo record
                    result.more = (contract_result.rows.size()-pos)-1;
                    }
                  }
                }
           }

           return result;
        }


        void read_only::GetFIOAccount(name account, read_only::get_table_rows_result &account_result) const {

            const abi_def system_abi = eosio::chain_apis::get_abi(db, fio_system_code);
            get_table_rows_params fio_table_row_params = get_table_rows_params{
                    .json           = true,
                    .code           = fio_system_code,
                    .scope          = fio_system_scope,
                    .table          = fio_accounts_table,
                    .lower_bound    = boost::lexical_cast<string>(account.value),
                    .upper_bound    = boost::lexical_cast<string>(account.value),
                    .key_type       = "i64",
                    .index_position = "1"};

            account_result =
                    get_table_rows_ex<key_value_index>(fio_table_row_params, system_abi);
        }
        //FIP-36 begin
        read_only::get_account_fio_public_key_result read_only::get_account_fio_public_key(const read_only::get_account_fio_public_key_params &p) const {

            get_account_fio_public_key_result result;
            //get the pub key from the accountmap table.
            string fioKey;
            get_table_rows_result account_result;

            FIO_400_ASSERT(fioio::isAccountValid(p.account), "account", p.account, "Invalid FIO Account format",
                           fioio::ErrorInvalidAccount);

            name accountnm = name{p.account};
            GetFIOAccount(accountnm,account_result);


            FIO_404_ASSERT(!account_result.rows.empty(), "Account not found",
                           fioio::ErrorNotFound);

            FIO_404_ASSERT(account_result.rows.size() == 1, "Unexpected number of results found account in account map",
                           fioio::ErrorUnexpectedNumberResults);

            fioKey = account_result.rows[0]["clientkey"].as_string();

            //hash it and re-verify
            string account_name;
            fioio::key_to_account(fioKey, account_name);
            name pubkeyaccount = name{account_name};

            FIO_404_ASSERT(accountnm.value == pubkeyaccount.value, "account map does not match specified account",
                           fioio::ErrorNotFound);

            result.fio_public_key = fioKey;



            return result;


        }
        //FIP-36 end

        //FIP-40
        /*** v1/chain/get_grantee_permissions
        * Retrieves the permissions for the specified grantee account name
        */
        read_only::get_grantee_permissions_result read_only::get_grantee_permissions(const read_only::get_grantee_permissions_params &p) const {

            get_grantee_permissions_result result;

            FIO_400_ASSERT(fioio::isAccountValid(p.grantee_account), "grantee_account", p.grantee_account, "Invalid FIO Account format",
                           fioio::ErrorInvalidAccount);

            name accountnm = name{p.grantee_account};

            FIO_400_ASSERT(p.limit >= 0, "limit", to_string(p.limit), "Invalid limit",
                           fioio::ErrorPagingInvalid);

            FIO_400_ASSERT(p.offset >= 0, "offset", to_string(p.offset), "Invalid offset",
                           fioio::ErrorPagingInvalid);

            
            uint32_t search_limit = p.limit;
            uint32_t search_offset = p.offset;

            const abi_def abi = eosio::chain_apis::get_abi(db, fio_perms_code);


            get_table_rows_params accesses_row_params = get_table_rows_params{.json=true,
                    .code=fio_perms_code,
                    .scope=fio_perms_scope,
                    .table=fio_accesses_table,
                    .lower_bound=boost::lexical_cast<string>(accountnm.value),
                    .upper_bound=boost::lexical_cast<string>(accountnm.value),
                    .key_type       = "i64",
                    .index_position = "3"};

            get_table_rows_result accesses_result = get_table_rows_by_seckey<index64_index, uint64_t>(accesses_row_params,
                                                                                                    abi,
                                                                                                    [](uint64_t v) -> uint64_t {
                                                                                                        return v;
                                                                                                    });

            FIO_404_ASSERT(!accesses_result.rows.empty(), "No Permissions or Domain does not exist", fioio::ErrorInvalidAccount);

            std::string grantee_account;
            std::string permission_name;
            std::string object_name;
            std::string perm_info;
            std::string grantor_account;
            uint64_t id = accesses_result.rows[0]["permission_id"].as_uint64();

            get_table_rows_params permissions_row_params = get_table_rows_params{.json=true,
                    .code=fio_perms_code,
                    .scope=fio_perms_scope,
                    .table=fio_permissions_table,
                    .lower_bound=boost::lexical_cast<string>(id),
                    .upper_bound=boost::lexical_cast<string>(id),
                    .key_type       = "i64",
                    .index_position = "1"};

            get_table_rows_result permissions_result =
                    get_table_rows_ex<key_value_index>(permissions_row_params, abi);


            FIO_404_ASSERT(!permissions_result.rows.empty(), "Permission not found", fioio::ErrorInvalidAccount);
            //get the permission record.

            permission_name = ((string) permissions_result.rows[0]["permission_name"].as_string());
            object_name = ((string) permissions_result.rows[0]["object_name"].as_string());
            perm_info = ((string) permissions_result.rows[0]["auxiliary_info"].as_string());
            grantor_account = ((string) permissions_result.rows[0]["grantor_account"].as_string());


            if (search_offset < accesses_result.rows.size() ) {
                int64_t leftover = accesses_result.rows.size() - (search_offset+search_limit);
                if (leftover < 0){
                    leftover = 0;
                }
                result.more = leftover;
                for (size_t pos = 0 + search_offset; pos < accesses_result.rows.size();pos++) {
                    if((search_limit > 0)&&(pos-search_offset >= search_limit)){
                        break;
                    }

                    uint64_t tid = accesses_result.rows[pos]["permission_id"].as_uint64();
                    if(tid != id)
                    {
                        permissions_row_params = get_table_rows_params{.json=true,
                                .code=fio_perms_code,
                                .scope=fio_perms_scope,
                                .table=fio_permissions_table,
                                .lower_bound=boost::lexical_cast<string>(tid),
                                .upper_bound=boost::lexical_cast<string>(tid),
                                .key_type       = "i64",
                                .index_position = "1"};

                        permissions_result =
                                get_table_rows_ex<key_value_index>(permissions_row_params, abi);

                        FIO_404_ASSERT(!permissions_result.rows.empty(), "Permission not found", fioio::ErrorInvalidAccount);

                        permission_name = ((string) permissions_result.rows[0]["permission_name"].as_string());
                        object_name = ((string) permissions_result.rows[0]["object_name"].as_string());
                        perm_info = ((string) permissions_result.rows[0]["auxiliary_info"].as_string());
                        grantor_account = ((string) permissions_result.rows[0]["grantor_account"].as_string());

                    }
                    grantee_account = ((string) accesses_result.rows[pos]["grantee_account"].as_string());

                    permission_info inf = permission_info{
                        .grantee_account = grantee_account,
                        .permission_name = permission_name,
                        .permission_info = perm_info,
                        .object_name = object_name,
                        .grantor_account = grantor_account
                    };


                    result.permissions.push_back(inf);
                    result.more = (accesses_result.rows.size()-pos)-1;
                }
            }


            return result;
        } // get_grantee_permissions

        /*** v1/chain/get_grantor_permissions
       * Retrieves the permissions for the specified grantor account name
       */
        read_only::get_grantor_permissions_result read_only::get_grantor_permissions(const read_only::get_grantor_permissions_params &p) const {
            // assert if empty chain key
            get_grantor_permissions_result result;

            FIO_400_ASSERT(fioio::isAccountValid(p.grantor_account), "grantor_account", p.grantor_account, "Invalid FIO Account format",
                           fioio::ErrorInvalidAccount);

            name accountnm = name{p.grantor_account};

            FIO_400_ASSERT(p.limit >= 0, "limit", to_string(p.limit), "Invalid limit",
                           fioio::ErrorPagingInvalid);

            FIO_400_ASSERT(p.offset >= 0, "offset", to_string(p.offset), "Invalid offset",
                           fioio::ErrorPagingInvalid);


            uint32_t search_limit = p.limit;
            uint32_t search_offset = p.offset;

            const abi_def abi = eosio::chain_apis::get_abi(db, fio_perms_code);


            get_table_rows_params accesses_row_params = get_table_rows_params{.json=true,
                    .code=fio_perms_code,
                    .scope=fio_perms_scope,
                    .table=fio_accesses_table,
                    .lower_bound=boost::lexical_cast<string>(accountnm.value),
                    .upper_bound=boost::lexical_cast<string>(accountnm.value),
                    .key_type       = "i64",
                    .index_position = "5"};

            get_table_rows_result accesses_result = get_table_rows_by_seckey<index64_index, uint64_t>(accesses_row_params,
                                                                                                      abi,
                                                                                                      [](uint64_t v) -> uint64_t {
                                                                                                          return v;
                                                                                                      });

            FIO_404_ASSERT(!accesses_result.rows.empty(), "No Permissions or Domain does not exist", fioio::ErrorInvalidAccount);

            std::string grantee_account;
            std::string permission_name;
            std::string object_name;
            std::string perm_info;
            std::string grantor_account;
            uint64_t id = accesses_result.rows[0]["permission_id"].as_uint64();

            get_table_rows_params permissions_row_params = get_table_rows_params{.json=true,
                    .code=fio_perms_code,
                    .scope=fio_perms_scope,
                    .table=fio_permissions_table,
                    .lower_bound=boost::lexical_cast<string>(id),
                    .upper_bound=boost::lexical_cast<string>(id),
                    .key_type       = "i64",
                    .index_position = "1"};

            get_table_rows_result permissions_result =
                    get_table_rows_ex<key_value_index>(permissions_row_params, abi);


            FIO_404_ASSERT(!permissions_result.rows.empty(), "Permission not found", fioio::ErrorInvalidAccount);
            //get the permission record.

            permission_name = ((string) permissions_result.rows[0]["permission_name"].as_string());
            object_name = ((string) permissions_result.rows[0]["object_name"].as_string());
            perm_info = ((string) permissions_result.rows[0]["auxiliary_info"].as_string());
            grantor_account = ((string) permissions_result.rows[0]["grantor_account"].as_string());


            if (search_offset < accesses_result.rows.size() ) {
                int64_t leftover = accesses_result.rows.size() - (search_offset+search_limit);
                if (leftover < 0){
                    leftover = 0;
                }
                result.more = leftover;
                for (size_t pos = 0 + search_offset; pos < accesses_result.rows.size();pos++) {
                    if((search_limit > 0)&&(pos-search_offset >= search_limit)){
                        break;
                    }

                    uint64_t tid = accesses_result.rows[pos]["permission_id"].as_uint64();
                    if(tid != id)
                    {
                        permissions_row_params = get_table_rows_params{.json=true,
                                .code=fio_perms_code,
                                .scope=fio_perms_scope,
                                .table=fio_permissions_table,
                                .lower_bound=boost::lexical_cast<string>(tid),
                                .upper_bound=boost::lexical_cast<string>(tid),
                                .key_type       = "i64",
                                .index_position = "1"};

                        permissions_result =
                                get_table_rows_ex<key_value_index>(permissions_row_params, abi);

                        FIO_404_ASSERT(!permissions_result.rows.empty(), "Permission not found", fioio::ErrorInvalidAccount);

                        permission_name = ((string) permissions_result.rows[0]["permission_name"].as_string());
                        object_name = ((string) permissions_result.rows[0]["object_name"].as_string());
                        perm_info = ((string) permissions_result.rows[0]["auxiliary_info"].as_string());
                        grantor_account = ((string) permissions_result.rows[0]["grantor_account"].as_string());

                    }
                    grantee_account = ((string) accesses_result.rows[pos]["grantee_account"].as_string());

                    permission_info inf = permission_info{
                            .grantee_account = grantee_account,
                            .permission_name = permission_name,
                            .permission_info = perm_info,
                            .object_name = object_name,
                            .grantor_account = grantor_account
                    };


                    result.permissions.push_back(inf);
                    result.more = (accesses_result.rows.size()-pos)-1;
                }
            }


            return result;
        } // get_actor_permissions

        /*** v1/chain/get_object_permissions
       * Retrieves the permissions for the specified permission and object name
       */
        read_only::get_object_permissions_result read_only::get_object_permissions(const read_only::get_object_permissions_params &p) const {
            // assert if empty chain key
            get_object_permissions_result result;


            FIO_400_ASSERT(p.permission_name.compare("register_address_on_domain") == 0, "permission_name", p.permission_name, "Invalid permission name",
                           fioio::ErrorInvalidPermission);

            FIO_400_ASSERT(p.object_name.length() > 0, "object_name", p.object_name, "Invalid object name",
                           fioio::ErrorInvalidPermission);


            FIO_400_ASSERT(p.limit >= 0, "limit", to_string(p.limit), "Invalid limit",
                           fioio::ErrorPagingInvalid);

            FIO_400_ASSERT(p.offset >= 0, "offset", to_string(p.offset), "Invalid offset",
                           fioio::ErrorPagingInvalid);


            uint32_t search_limit = p.limit;
            uint32_t search_offset = p.offset;

            const abi_def abi = eosio::chain_apis::get_abi(db, fio_perms_code);


            string hashstr = p.object_name + p.permission_name;
            uint128_t names_hash = fioio::string_to_uint128_t(hashstr.c_str());
            std::string hexvalnameshash = "0x";
            hexvalnameshash.append(
                    fioio::to_hex_little_endian(reinterpret_cast<const char *>(&names_hash), sizeof(names_hash)));



            get_table_rows_params accesses_row_params = get_table_rows_params{.json=true,
                    .code=fio_perms_code,
                    .scope=fio_perms_scope,
                    .table=fio_accesses_table,
                    .lower_bound=hexvalnameshash,
                    .upper_bound=hexvalnameshash,
                    .key_type       = "hex",
                    .index_position = "6"};

            get_table_rows_result accesses_result = get_table_rows_by_seckey<index128_index, uint128_t>(accesses_row_params,
                                                                                                      abi,
                                                                                                      [](uint128_t v) -> uint128_t {
                                                                                                          return v;
                                                                                                      });

            FIO_404_ASSERT(!accesses_result.rows.empty(), "No Permissions or Domain does not exist", fioio::ErrorInvalidAccount);

            std::string grantee_account;
            std::string permission_name;
            std::string object_name;
            std::string perm_info;
            std::string grantor_account;
            uint64_t id = accesses_result.rows[0]["permission_id"].as_uint64();
            get_table_rows_params permissions_row_params = get_table_rows_params{.json=true,
                    .code=fio_perms_code,
                    .scope=fio_perms_scope,
                    .table=fio_permissions_table,
                    .lower_bound=boost::lexical_cast<string>(id),
                    .upper_bound=boost::lexical_cast<string>(id),
                    .key_type       = "i64",
                    .index_position = "1"};

            get_table_rows_result permissions_result =
                    get_table_rows_ex<key_value_index>(permissions_row_params, abi);


            FIO_404_ASSERT(!permissions_result.rows.empty(), "Permission not found", fioio::ErrorInvalidAccount);
            //get the permission record.

            permission_name = ((string) permissions_result.rows[0]["permission_name"].as_string());
            object_name = ((string) permissions_result.rows[0]["object_name"].as_string());
            perm_info = ((string) permissions_result.rows[0]["auxiliary_info"].as_string());
            grantor_account = ((string) permissions_result.rows[0]["grantor_account"].as_string());


            if (search_offset < accesses_result.rows.size() ) {
                int64_t leftover = accesses_result.rows.size() - (search_offset+search_limit);
                if (leftover < 0){
                    leftover = 0;
                }
                result.more = leftover;
                for (size_t pos = 0 + search_offset; pos < accesses_result.rows.size();pos++) {
                    if((search_limit > 0)&&(pos-search_offset >= search_limit)){
                        break;
                    }

                    uint64_t tid = accesses_result.rows[pos]["permission_id"].as_uint64();
                    if(tid != id)
                    {
                        permissions_row_params = get_table_rows_params{.json=true,
                                .code=fio_perms_code,
                                .scope=fio_perms_scope,
                                .table=fio_permissions_table,
                                .lower_bound=boost::lexical_cast<string>(tid),
                                .upper_bound=boost::lexical_cast<string>(tid),
                                .key_type       = "i64",
                                .index_position = "1"};

                        permissions_result =
                                get_table_rows_ex<key_value_index>(permissions_row_params, abi);

                        FIO_404_ASSERT(!permissions_result.rows.empty(), "Permission not found", fioio::ErrorInvalidAccount);

                        permission_name = ((string) permissions_result.rows[0]["permission_name"].as_string());
                        object_name = ((string) permissions_result.rows[0]["object_name"].as_string());
                        perm_info = ((string) permissions_result.rows[0]["auxiliary_info"].as_string());
                        grantor_account = ((string) permissions_result.rows[0]["grantor_account"].as_string());

                    }
                    grantee_account = ((string) accesses_result.rows[pos]["grantee_account"].as_string());

                    permission_info inf = permission_info{
                            .grantee_account = grantee_account,
                            .permission_name = permission_name,
                            .permission_info = perm_info,
                            .object_name = object_name,
                            .grantor_account = grantor_account
                    };


                    result.permissions.push_back(inf);
                    result.more = (accesses_result.rows.size()-pos)-1;
                }
            }

            return result;
        } // get_object_permissions



        /*** v1/chain/get_fio_names
        * Retrieves the fionames associated with the provided public address
        * @param p
        * @return result
        */
        read_only::get_fio_names_result read_only::get_fio_names(const read_only::get_fio_names_params &p) const {
            // assert if empty chain key
            get_fio_names_result result;
            string fioKey = p.fio_public_key;

            //first check the pub key for validity.
            FIO_400_ASSERT(fioio::isPubKeyValid(fioKey), "fio_public_key", p.fio_public_key.c_str(),
                           "Invalid FIO Public Key",
                           fioio::ErrorPubKeyValid);

            string account_name;
            fioio::key_to_account(fioKey, account_name);
            name account = name{account_name};

            const abi_def abi = eosio::chain_apis::get_abi(db, fio_system_code);
            const uint64_t key_hash = ::eosio::string_to_uint64_t(fioKey.c_str()); // hash of public address

            get_table_rows_params table_row_params = get_table_rows_params{
                    .json        = true,
                    .code        = fio_system_code,
                    .scope       = fio_system_scope,
                    .table       = fio_address_table,
                    .lower_bound = boost::lexical_cast<string>(account.value),
                    .upper_bound = boost::lexical_cast<string>(account.value),
                    .key_type       = "i64",
                    .index_position ="4"};

            get_table_rows_result table_rows_result = get_table_rows_by_seckey<index64_index, uint64_t>(
                    table_row_params, abi,
                    [](uint64_t v) -> uint64_t {
                        return v;
                    });

            std::string nam;
            uint64_t namexpiration = 4294967295; //Sunday, February 7, 2106 6:28:15 AM GMT+0000 (Max 32 bit expiration)
            uint64_t rem_bundle;
            time_t temptime;
            struct tm *timeinfo;
            char buffer[80];

            if (!table_rows_result.rows.empty()) {

                // Look through the keynames lookup results and push the fio_addresses into results
                for (size_t pos = 0; pos < table_rows_result.rows.size(); pos++) {

                    nam = (string) table_rows_result.rows[pos]["name"].as_string();
                    if (nam.find('@') !=
                        std::string::npos) { //if it's not a domain record in the keynames table (no '.'),
                        rem_bundle = (uint64_t) table_rows_result.rows[pos]["bundleeligiblecountdown"].as_uint64();

                        temptime = namexpiration;
                        timeinfo = gmtime(&temptime);
                        strftime(buffer, 80, "%Y-%m-%dT%T", timeinfo);

                        fioaddress_record fa{nam, buffer,rem_bundle};
                        result.fio_addresses.push_back(fa);
                    }
                } // Get FIO domains and push
            }
            //Get the domain record
            get_table_rows_params domain_row_params = get_table_rows_params{.json=true,
                    .code=fio_system_code,
                    .scope=fio_system_scope,
                    .table=fio_domains_table,
                    .lower_bound=boost::lexical_cast<string>(::eosio::string_to_name(account_name.c_str())),
                    .upper_bound=boost::lexical_cast<string>(::eosio::string_to_name(account_name.c_str())),
                    .key_type       = "i64",
                    .index_position = "2"};

            get_table_rows_result domain_result = get_table_rows_by_seckey<index64_index, uint64_t>(domain_row_params,
                                                                                                    abi,
                                                                                                    [](uint64_t v) -> uint64_t {
                                                                                                        return v;
                                                                                                    });
            FIO_404_ASSERT(!(domain_result.rows.empty() && table_rows_result.rows.empty()), "No FIO names",
                           fioio::ErrorNoFIONames);

            if (domain_result.rows.empty()) {

                return result;
            }

            std::string dom;
            uint64_t domexpiration;
            bool public_domain;

            for (size_t pos = 0; pos < domain_result.rows.size(); pos++) {
                dom = ((string) domain_result.rows[pos]["name"].as_string());
                domexpiration = domain_result.rows[pos]["expiration"].as_uint64();
                public_domain = domain_result.rows[pos]["is_public"].as_bool();

                temptime = domexpiration;
                timeinfo = gmtime(&temptime);
                strftime(buffer, 80, "%Y-%m-%dT%T", timeinfo);

                fiodomain_record d{dom, buffer, public_domain};
                result.fio_domains.push_back(d);    //pushback results in domain
            }

            return result;
        } // get_fio_names


        /*** v1/chain/get_fio_domains
       * Retrieves the domains associated with the provided public address
       * @param p
       * @return result
       */
        read_only::get_fio_domains_result read_only::get_fio_domains(const read_only::get_fio_domains_params &p) const {
            // assert if empty chain key
            get_fio_domains_result result;
            result.more = 0;
            //first check the pub key for validity.
            FIO_400_ASSERT(fioio::isPubKeyValid(p.fio_public_key), "fio_public_key", p.fio_public_key.c_str(),
                           "Invalid FIO Public Key",
                           fioio::ErrorPubKeyValid);

            FIO_400_ASSERT(p.limit >= 0, "limit", to_string(p.limit), "Invalid limit",
                           fioio::ErrorPagingInvalid);

            FIO_400_ASSERT(p.offset >= 0, "offset", to_string(p.offset), "Invalid offset",
                           fioio::ErrorPagingInvalid);

            string account_name;
            fioio::key_to_account(p.fio_public_key, account_name);
            name account = name{account_name};
            time_t temptime;
            struct tm *timeinfo;
            char buffer[80];

            uint32_t search_limit = p.limit;
            uint32_t search_offset = p.offset;

            const abi_def abi = eosio::chain_apis::get_abi(db, fio_system_code);

            //Get the domain record
            get_table_rows_params domain_row_params = get_table_rows_params{.json=true,
                    .code=fio_system_code,
                    .scope=fio_system_scope,
                    .table=fio_domains_table,
                    .lower_bound=boost::lexical_cast<string>(::eosio::string_to_name(account_name.c_str())),
                    .upper_bound=boost::lexical_cast<string>(::eosio::string_to_name(account_name.c_str())),
                    .key_type       = "i64",
                    .index_position = "2"};

            get_table_rows_result domain_result = get_table_rows_by_seckey<index64_index, uint64_t>(domain_row_params,
                                                                                                    abi,
                                                                                                    [](uint64_t v) -> uint64_t {
                                                                                                        return v;
                                                                                                    });

            FIO_404_ASSERT(!domain_result.rows.empty(), "No FIO Domains", fioio::ErrorPubAddressNotFound);

            std::string dom;
            uint64_t domexpiration;
            bool public_domain;

            if (search_offset < domain_result.rows.size() ) {
                int64_t leftover = domain_result.rows.size() - (search_offset+search_limit);
                if (leftover < 0){
                    leftover = 0;
                }
                result.more = leftover;
                for (size_t pos = 0 + search_offset; pos < domain_result.rows.size();pos++) {
                    if((search_limit > 0)&&(pos-search_offset >= search_limit)){
                        break;
                    }

                    dom = ((string) domain_result.rows[pos]["name"].as_string());
                    domexpiration = domain_result.rows[pos]["expiration"].as_uint64();
                    public_domain = domain_result.rows[pos]["is_public"].as_bool();

                    temptime = domexpiration;
                    timeinfo = gmtime(&temptime);
                    strftime(buffer, 80, "%Y-%m-%dT%T", timeinfo);

                    fiodomain_record d{dom, buffer, public_domain};
                    result.fio_domains.push_back(d);    //pushback results in domain
                    result.more = (domain_result.rows.size()-pos)-1;
                }
            }

            return result;
        } // get_fio_domains

        /*** v1/chain/get_fio_addresses
       * Retrieves the fio addresses associated with the provided public address
       * @param p
       * @return result
       */
        read_only::get_fio_addresses_result read_only::get_fio_addresses(const read_only::get_fio_addresses_params &p) const {
            // assert if empty chain key
            get_fio_addresses_result result;
            result.more = 0;
            //first check the pub key for validity.
            FIO_400_ASSERT(fioio::isPubKeyValid(p.fio_public_key), "fio_public_key", p.fio_public_key.c_str(),
                           "Invalid FIO Public Key",
                           fioio::ErrorPubKeyValid);

            FIO_400_ASSERT(p.limit >= 0, "limit", to_string(p.limit), "Invalid limit",
                           fioio::ErrorPagingInvalid);

            FIO_400_ASSERT(p.offset >= 0, "offset", to_string(p.offset), "Invalid offset",
                           fioio::ErrorPagingInvalid);


            string account_name;
            fioio::key_to_account(p.fio_public_key, account_name);
            name account = name{account_name};

            uint32_t search_limit = p.limit;
            uint32_t search_offset = p.offset;

            const abi_def abi = eosio::chain_apis::get_abi(db, fio_system_code);
            const uint64_t key_hash = ::eosio::string_to_uint64_t(p.fio_public_key.c_str()); // hash of public address

            get_table_rows_params table_row_params = get_table_rows_params{
                    .json        = true,
                    .code        = fio_system_code,
                    .scope       = fio_system_scope,
                    .table       = fio_address_table,
                    .lower_bound = boost::lexical_cast<string>(account.value),
                    .upper_bound = boost::lexical_cast<string>(account.value),
                    .key_type       = "i64",
                    .index_position ="4"};

            get_table_rows_result table_rows_result = get_table_rows_by_seckey<index64_index, uint64_t>(
                    table_row_params, abi,
                    [](uint64_t v) -> uint64_t {
                        return v;
                    });

            std::string nam;
            uint64_t namexpiration = 4294967295; //Sunday, February 7, 2106 6:28:15 AM GMT+0000 (Max 32 bit expiration)
            uint64_t rem_bundle;
            time_t temptime;
            struct tm *timeinfo;
            char buffer[80];

            FIO_404_ASSERT(!table_rows_result.rows.empty(), "No FIO Addresses", fioio::ErrorPubAddressNotFound);

            if (search_offset < table_rows_result.rows.size()) {
                int64_t leftover = table_rows_result.rows.size() - (search_offset+search_limit);
                if (leftover < 0){
                    leftover = 0;
                }
                result.more = leftover;

                for (size_t pos = 0 + search_offset;pos < table_rows_result.rows.size();pos++) {
                    if((search_limit > 0)&&(pos-search_offset >= search_limit)){
                        break;
                    }
                    nam = (string) table_rows_result.rows[pos]["name"].as_string();
                    if (nam.find('@') != std::string::npos) {
                        rem_bundle = (uint64_t) table_rows_result.rows[pos]["bundleeligiblecountdown"].as_uint64();

                        temptime = namexpiration;
                        timeinfo = gmtime(&temptime);
                        strftime(buffer, 80, "%Y-%m-%dT%T", timeinfo);
                        fioaddress_record fa{nam, buffer,rem_bundle};
                        result.fio_addresses.push_back(fa);
                    }
                    result.more = (table_rows_result.rows.size()-pos)-1;
                }
            }
            return result;
        } // get_fio_addresses

        fc::variant get_staking_row(const database &db, const abi_def &abi, const abi_serializer &abis,
                                    const fc::microseconds &abi_serializer_max_time_ms, bool shorten_abi_errors) {
            const auto table_type = get_table_type(abi, N(staking));
            EOS_ASSERT(table_type == read_only::KEYi64, chain::contract_table_query_exception,
                       "Invalid table type ${type} for table global", ("type", table_type));

            const auto *const table_id = db.find<chain::table_id_object, chain::by_code_scope_table>(
                    boost::make_tuple(N(fio.staking), N(fio.staking), N(staking)));
            EOS_ASSERT(table_id, chain::contract_table_query_exception, "Missing table staking");

            const auto &kv_index = db.get_index<key_value_index, by_scope_primary>();
            const auto it = kv_index.find(boost::make_tuple(table_id->id, N(staking)));
            EOS_ASSERT(it != kv_index.end(), chain::contract_table_query_exception, "Missing row in table staking");

            vector<char> data;
            read_only::copy_inline_row(*it, data);
            return abis.binary_to_variant(abis.get_table_type(N(staking)), data, abi_serializer_max_time_ms,
                                          shorten_abi_errors);
        }

        //FIP-39 begin

        read_only::get_encrypt_key_result read_only::get_encrypt_key(const read_only::get_encrypt_key_params &p) const {

            get_encrypt_key_result result1;

            fioio::FioAddress fa;
            fioio::getFioAddressStruct(p.fio_address, fa);
            uint128_t name_hash = fioio::string_to_uint128_t(fa.fioaddress.c_str());
            FIO_400_ASSERT(validateFioNameFormat(fa), "fio_address", fa.fioaddress.c_str(), "Invalid FIO Address",
                           fioio::ErrorFioNameNotReg);

            std::string hexvalnamehash = "0x";
            hexvalnamehash.append(
                    fioio::to_hex_little_endian(reinterpret_cast<const char *>(&name_hash), sizeof(name_hash)));

            const abi_def abi = eosio::chain_apis::get_abi(db, fio_system_code);

            get_table_rows_params name_table_row_params = get_table_rows_params{.json=true,
                    .code=fio_system_code,
                    .scope=fio_system_scope,
                    .table=fio_address_table,
                    .lower_bound=hexvalnamehash,
                    .upper_bound=hexvalnamehash,
                    .encode_type="hex",
                    .index_position ="5"};

            get_table_rows_result names_table_rows_result = get_table_rows_by_seckey<index128_index, uint128_t>(
                    name_table_row_params, abi, [](uint128_t v) -> uint128_t {
                        return v;
                    });

            FIO_400_ASSERT(!names_table_rows_result.rows.empty(), "fio_address", fa.fioaddress.c_str(),
                           "No such FIO address",
                           fioio::ErrorFioNameNotReg);

            //128 bit indexes returned multiple rows unexpectedly during integration testing, this code ensures
            // that if the index search returns multiple rows unexpectedly the results are not processed.
            FIO_404_ASSERT(names_table_rows_result.rows.size() == 1, "Multiple names found for fio address",
                           fioio::ErrorNoFeesFoundForEndpoint);

            uint64_t handleid = names_table_rows_result.rows[0]["id"].as_uint64();
            //now get that handle id from fionameinfo table.
            get_table_rows_params fionameinfo_table_row_params = get_table_rows_params{.json=true,
                    .code=fio_system_code,
                    .scope=fio_system_scope,
                    .table="fionameinfo",
                    .lower_bound=boost::lexical_cast<string>(handleid),
                    .upper_bound=boost::lexical_cast<string>(handleid),
                    .key_type       = "i64",
                    .index_position ="2"};

            get_table_rows_result fionameinfo_table_rows_result = get_table_rows_by_seckey<index64_index, uint64_t>(
                    fionameinfo_table_row_params, abi, [](uint64_t v) -> uint64_t {
                        return v;
                    });

            bool found = true;
            if(fionameinfo_table_rows_result.rows.empty()){
               found = false;
            }else{
                bool descmatch = false;
                for (size_t pos = 0; pos < fionameinfo_table_rows_result.rows.size(); pos++) {
                    string datadesc = fionameinfo_table_rows_result.rows[pos]["datadesc"].as_string();
                    string datavalue = fionameinfo_table_rows_result.rows[pos]["datavalue"].as_string();
                    if (datadesc.compare("FIO_REQUEST_CONTENT_ENCRYPTION_PUB_KEY") == 0)
                    {
                        result1.encrypt_public_key = datavalue;
                        descmatch = true;
                    }

                }
                if (!descmatch){
                    found = false;
                }

            }

            if (!found)
            {
                //go get the pub key for FIO FIO for this address.
                const name code = ::eosio::string_to_name("fio.address");
                const abi_def abi = eosio::chain_apis::get_abi(db, code);
                const uint128_t name_hash = fioio::string_to_uint128_t(fa.fioaddress.c_str());
                const uint128_t domain_hash = fioio::string_to_uint128_t(fa.fiodomain.c_str());
                const string chainCode = fioio::makeLowerCase("FIO");
                const string tokenCode = fioio::makeLowerCase("FIO");
                const string defaultCode = "*";

                //these are the results for the table searches for domain ansd fio name
                get_table_rows_result domain_result;
                get_table_rows_result fioname_result;
                get_table_rows_result name_result;

                std::string hexvaldomainhash = "0x";
                hexvaldomainhash.append(
                        fioio::to_hex_little_endian(reinterpret_cast<const char *>(&domain_hash), sizeof(domain_hash)));

                get_table_rows_params name_table_row_params = get_table_rows_params{.json=true,
                        .code=code,
                        .scope=fio_system_scope,
                        .table=fio_domains_table,
                        .lower_bound=hexvaldomainhash,
                        .upper_bound=hexvaldomainhash,
                        .encode_type="hex",
                        .index_position ="4"};

                domain_result = get_table_rows_by_seckey<index128_index, uint128_t>(
                        name_table_row_params, abi, [](uint128_t v) -> uint128_t {
                            return v;
                        });

                FIO_404_ASSERT(!domain_result.rows.empty(), "Public address not found", fioio::ErrorPubAddressNotFound);

                uint32_t domain_expiration = (uint32_t) (domain_result.rows[0]["expiration"].as_uint64());
                uint32_t present_time = (uint32_t) time(0);


                FIO_400_ASSERT(!(present_time > domain_expiration), "fio_address", p.fio_address, "Invalid FIO Address",
                               fioio::ErrorFioNameEmpty);

                
                string defaultPubAddress = "";

                for (int i = 0; i < names_table_rows_result.rows[0]["addresses"].size(); i++) {
                    string tToken = fioio::makeLowerCase(names_table_rows_result.rows[0]["addresses"][i]["token_code"].as_string());
                    string tChain = fioio::makeLowerCase(names_table_rows_result.rows[0]["addresses"][i]["chain_code"].as_string());

                    if ((tToken == tokenCode) && (tChain == chainCode)) {
                        result1.encrypt_public_key = names_table_rows_result.rows[0]["addresses"][i]["public_address"].as_string();
                        break;
                    }
                    if ((tToken == defaultCode) && (tChain == chainCode)) {
                        defaultPubAddress = names_table_rows_result.rows[0]["addresses"][i]["public_address"].as_string();
                    }
                    if (i == names_table_rows_result.rows[0]["addresses"].size() - 1 && defaultPubAddress != "" ) {
                        result1.encrypt_public_key = defaultPubAddress;
                    }
                }

                FIO_404_ASSERT(!(result1.encrypt_public_key == ""), "Public address not found", fioio::ErrorPubAddressNotFound);

            }

            return result1;
        } //get_encrypt_key

        //FIP-39 end


        read_only::get_fio_balance_result read_only::get_fio_balance(const read_only::get_fio_balance_params &p) const {
            string fioKey = p.fio_public_key;
            FIO_400_ASSERT(fioio::isPubKeyValid(fioKey), "fio_public_key", p.fio_public_key.c_str(),
                           "Invalid FIO Public Key",
                           fioio::ErrorPubKeyValid);

            get_account_results actor_lookup_results;
            get_account_params actor_lookup_params;
            get_fio_balance_result result;
            vector<asset> cursor;
            result.balance = 0;
            //set available in result to defaults, if nothing present, 0
            result.available = 0;


            uint128_t keyhash = fioio::string_to_uint128_t(fioKey.c_str());
            const abi_def system_abi = eosio::chain_apis::get_abi(db, fio_system_code);

            std::string hexvalkeyhash = "0x";
            hexvalkeyhash.append(
                    fioio::to_hex_little_endian(reinterpret_cast<const char *>(&keyhash), sizeof(keyhash)));


            get_table_rows_params eosio_table_row_params = get_table_rows_params{
                    .json           = true,
                    .code           = fio_system_code,
                    .scope          = fio_system_scope,
                    .table          = fio_accounts_table,
                    .lower_bound    = hexvalkeyhash,
                    .upper_bound    = hexvalkeyhash,
                    .key_type       = "hex",
                    .index_position = "2"};

            get_table_rows_result account_result =
                    get_table_rows_by_seckey<index128_index, uint128_t>(
                            eosio_table_row_params, system_abi, [](uint128_t v) -> uint128_t {
                                return v;
                            });

            FIO_404_ASSERT(!account_result.rows.empty(), "Public key not found", fioio::ErrorPubAddressNotFound);

            string fio_account = account_result.rows[0]["account"].as_string();
            actor_lookup_params.account_name = fio_account;

            try {
                actor_lookup_results = get_account(actor_lookup_params);
            }
            catch (...) {
                FIO_404_ASSERT(false, "Public key not found", fioio::ErrorPubAddressNotFound);
            }

            get_currency_balance_params balance_params;
            balance_params.code = ::eosio::string_to_name("fio.token");
            balance_params.account = ::eosio::string_to_name(fio_account.c_str());
            cursor = get_currency_balance(balance_params);


            string account_name;
            fioio::key_to_account(fioKey, account_name);
            name account = name{account_name};
            const abi_def sys_abi = eosio::chain_apis::get_abi(db, fio_code);

            //get genesis/main net lock tokens, subtract remaining lock amount if it exists
            get_table_rows_params mtable_row_params = get_table_rows_params{
                    .json        = true,
                    .code        = fio_code,
                    .scope       = fio_scope,
                    .table       = fio_mainnet_locks_table,
                    .lower_bound = boost::lexical_cast<string>(account.value),
                    .upper_bound = boost::lexical_cast<string>(account.value),
                    .key_type       = "i64",
                    .index_position = "1"};

            get_table_rows_result mrows_result =
                    get_table_rows_ex<key_value_index>(mtable_row_params, sys_abi);

            uint64_t lockamount = 0;
            if (!mrows_result.rows.empty()) {

                FIO_404_ASSERT(mrows_result.rows.size() == 1, "Unexpected number of results found for main net locks",
                               fioio::ErrorUnexpectedNumberResults);

                lockamount = mrows_result.rows[0]["remaining_locked_amount"].as_uint64();
            }

            //get the locked tokens, subtract the remaining lock amount if it exists.
            get_table_rows_params gtable_row_params = get_table_rows_params{
                    .json        = true,
                    .code        = fio_code,
                    .scope       = fio_scope,
                    .table       = fio_locks_table,
                    .lower_bound = boost::lexical_cast<string>(account.value),
                    .upper_bound = boost::lexical_cast<string>(account.value),
                    .key_type       = "i64",
                    .index_position = "2"};

            get_table_rows_result grows_result = get_table_rows_by_seckey<index64_index, uint64_t>(
                    gtable_row_params, sys_abi, [](uint64_t v) -> uint64_t {
                        return v;
                    });

            uint64_t nowepoch = db.head_block_time().sec_since_epoch();

            uint64_t additional_available_fio_locks = 0;
            if (!grows_result.rows.empty()) {

                if (grows_result.rows.size() > 1) {
                    dlog(" multiple lock table entries for account " + account.to_string());
                    }

                uint64_t timestamp = grows_result.rows[0]["timestamp"].as_uint64();
                uint32_t payouts_performed = grows_result.rows[0]["payouts_performed"].as_uint64();
                uint64_t timesincelockcreated = 0;

                if (nowepoch > timestamp) {
                    timesincelockcreated = nowepoch - timestamp;
                }

                if (timesincelockcreated > 0) {
                    //traverse the locks and compute the amount available but not yet accounted by the system.
                    //this makes the available accurate when the user has not called transfer, or vote yet
                    //but has locked funds that are eligible for spending in their general lock.
                    for (int i = 0; i < grows_result.rows[0]["periods"].size(); i++) {
                        uint64_t duration = grows_result.rows[0]["periods"][i]["duration"].as_uint64();
                        uint64_t amount = grows_result.rows[0]["periods"][i]["amount"].as_uint64();
                        if (duration > timesincelockcreated) {
                            break;
                        }
                        if (i > ((int) payouts_performed - 1)) {
                            additional_available_fio_locks += amount;
                        }
                    }
                }

                //correct the remaining lock amount to account for tokens that are unlocked before system
                //accounting is updated by calling transfer or vote.
                uint64_t remains = grows_result.rows[0]["remaining_lock_amount"].as_uint64();
                if (((remains >= 0) && (additional_available_fio_locks >= 0)) &&
                    (remains > additional_available_fio_locks)) {
                    lockamount += remains - additional_available_fio_locks;
                }
            }


            //get the account staking info

            const abi_def staking_abi = eosio::chain_apis::get_abi(db, fio_staking_code);

            get_table_rows_params staking_table_row_params = get_table_rows_params{
                    .json        = true,
                    .code        = fio_staking_code,
                    .scope       = fio_staking_scope,
                    .table       = fio_accountstake_table,
                    .lower_bound = boost::lexical_cast<string>(account.value),
                    .upper_bound = boost::lexical_cast<string>(account.value),
                    .key_type       = "i64",
                    .index_position = "2"};

            get_table_rows_result staking_rows_result = get_table_rows_by_seckey<index64_index, uint64_t>(
                    staking_table_row_params, staking_abi, [](uint64_t v) -> uint64_t {
                        return v;
                    });

            uint64_t stakeamount = 0;
            uint64_t srpamount = 0;
            if (!staking_rows_result.rows.empty()) {

                FIO_404_ASSERT(staking_rows_result.rows.size() == 1, "Unexpected number of results found in accountstake",
                               fioio::ErrorUnexpectedNumberResults);

                stakeamount = staking_rows_result.rows[0]["total_staked_fio"].as_uint64();
                srpamount = staking_rows_result.rows[0]["total_srp"].as_uint64();
            }


            //end get account staking info
            if (!cursor.empty()) {
                const abi_def abi = eosio::chain_apis::get_abi(db, N(fio.staking));
                const abi_serializer abis{abi, abi_serializer_max_time};
                const auto &d = db.db();
                uint64_t stakedtokenpool =  get_staking_row(d, abi, abis, abi_serializer_max_time,
                        shorten_abi_errors)["staked_token_pool"].as_uint64();
                uint64_t combinedtokenpool =  get_staking_row(d, abi, abis, abi_serializer_max_time,
                                                            shorten_abi_errors)["last_combined_token_pool"].as_uint64();
                uint64_t globalsrpcount =  get_staking_row(d, abi, abis, abi_serializer_max_time,
                        shorten_abi_errors)["last_global_srp_count"].as_uint64();
                long double roesufspersrp =  0.5;
                const int32_t ENABLESTAKINGREWARDSEPOCHSEC = 1627686000;  //July 30 5:00PM MST 11:00PM GMT
                if (nowepoch > ENABLESTAKINGREWARDSEPOCHSEC) {
                    roesufspersrp = (long double)(combinedtokenpool) / (long double)(globalsrpcount);
                    //round it after the 15th decimal place
                    roesufspersrp = roundl(roesufspersrp * 1000000000000000.0) / 1000000000000000.00;
                }

                uint64_t rVal = (uint64_t) cursor[0].get_amount();
                result.balance = rVal;
                if( ((stakeamount >= 0) && (lockamount >=0)) && rVal > (stakeamount + lockamount)) {
                    result.available = rVal - stakeamount - lockamount;
                }else{
                    result.available = 0;
                }
                result.staked = stakeamount;
                result.srps = srpamount;
                char s[100];
                sprintf(s,"%.15Lf",roesufspersrp);
                result.roe = (string)s;
            }
            return result;
        } //get_fio_balance

        read_only::get_actor_result read_only::get_actor(const read_only::get_actor_params &p) const {

            FIO_400_ASSERT(fioio::isPubKeyValid(p.fio_public_key), "fio_public_key", p.fio_public_key.c_str(),
                           "Invalid FIO Public Key",
                           fioio::ErrorPubKeyValid);
            get_actor_result result;
            string account_name;
            fioio::key_to_account(p.fio_public_key, account_name);
            result.actor = account_name;
            return result;
        } //get_actor


        /*** v1/chain/get_fee
        * Retrieves the fee associated with the specified fio address and blockchain endpoint
        * @param p
        * @return result
        */
        read_only::get_fee_result read_only::get_fee(const read_only::get_fee_params &p) const {
            // assert if empty chain key
            get_fee_result result;
            result.fee = 0;

            FIO_400_ASSERT(!p.end_point.empty(), "end_point", p.end_point.c_str(), "Invalid end point",
                           fioio::ErrorNoEndpoint);

            FIO_400_ASSERT(p.end_point.size() <= FEEMAXLENGTH, "end_point", p.end_point.c_str(), "Invalid end point",
                           fioio::ErrorNoEndpoint);

            //get_fee
            const uint128_t endpointhash = fioio::string_to_uint128_t(p.end_point.c_str());

            //read the fees table.
            const abi_def abi = eosio::chain_apis::get_abi(db, fio_fee_code);

            std::string hexvalendpointhash = "0x";
            hexvalendpointhash.append(
                    fioio::to_hex_little_endian(reinterpret_cast<const char *>(&endpointhash), sizeof(endpointhash)));

            get_table_rows_params name_table_row_params = get_table_rows_params{.json=true,
                    .code=fio_fee_code,
                    .scope=fio_fee_scope,
                    .table=fio_fees_table,
                    .lower_bound=hexvalendpointhash,
                    .upper_bound=hexvalendpointhash,
                    .encode_type="hex",
                    .index_position ="2"};

            // Do secondary key lookup
            get_table_rows_result table_rows_result = get_table_rows_by_seckey<index128_index, uint128_t>(
                    name_table_row_params, abi, [](uint128_t v) -> uint128_t {
                        return v;
                    });

            FIO_400_ASSERT(!table_rows_result.rows.empty(), "end_point", p.end_point, "Invalid end point",
                           fioio::ErrorNoFeesFoundForEndpoint);
            FIO_404_ASSERT(table_rows_result.rows.size() == 1, "Multiple fees found for endpoint",
                           fioio::ErrorNoFeesFoundForEndpoint);

            bool isbundleeligible = ((uint64_t) (table_rows_result.rows[0]["type"].as_uint64()) == 1);
            uint64_t feeamount = (uint64_t) table_rows_result.rows[0]["suf_amount"].as_uint64();

            if (isbundleeligible && !p.fio_address.empty() ) {
                //read the fio names table using the specified address
                //read the fees table.
                const abi_def abi = eosio::chain_apis::get_abi(db, fio_system_code);

                fioio::FioAddress fa;
                fioio::getFioAddressStruct(p.fio_address.c_str(), fa);
                uint128_t name_hash = fioio::string_to_uint128_t(fa.fioaddress.c_str());

                FIO_400_ASSERT(validateFioNameFormat(fa), "fio_address", fa.fioaddress.c_str(), "Invalid FIO Address",
                            fioio::ErrorFioNameNotReg);

                std::string hexvalnamehash = "0x";
                hexvalnamehash.append(
                        fioio::to_hex_little_endian(reinterpret_cast<const char *>(&name_hash), sizeof(name_hash)));

                get_table_rows_params name_table_row_params = get_table_rows_params{.json=true,
                        .code=fio_system_code,
                        .scope=fio_system_scope,
                        .table=fio_address_table,
                        .lower_bound=hexvalnamehash,
                        .upper_bound=hexvalnamehash,
                        .encode_type="hex",
                        .index_position ="5"};

                get_table_rows_result names_table_rows_result = get_table_rows_by_seckey<index128_index, uint128_t>(
                        name_table_row_params, abi, [](uint128_t v) -> uint128_t {
                            return v;
                        });

                FIO_400_ASSERT(!names_table_rows_result.rows.empty(), "fio_address", fa.fioaddress.c_str(),
                               "No such FIO address",
                               fioio::ErrorFioNameNotReg);

                //128 bit indexes returned multiple rows unexpectedly during integration testing, this code ensures
                // that if the index search returns multiple rows unexpectedly the results are not processed.
                FIO_404_ASSERT(names_table_rows_result.rows.size() == 1, "Multiple names found for fio address",
                               fioio::ErrorNoFeesFoundForEndpoint);

                uint64_t bundleeligiblecountdown = (uint64_t) names_table_rows_result.rows[0]["bundleeligiblecountdown"].as_uint64();

                if (bundleeligiblecountdown < 1) {
                    result.fee = feeamount;
                }
            } else {
                //not bundle eligible
                result.fee = feeamount;
            }

            return result;
        } // get_fee

         /*** v1/chain/get_oracle_fees
        * Returns current fees for wrapping
        * @param
        * @return result
        */
        read_only::get_oracle_fees_result read_only::get_oracle_fees(const read_only::get_oracle_fees_params &p) const {
            get_oracle_fees_result result;
            vector <uint64_t> token_fees;
            vector <uint64_t> nft_fees;
            vector <oraclefee_record> final_fees;
            const abi_def oracle_abi = eosio::chain_apis::get_abi(db, fio_oracle_code);

            get_table_rows_params fio_table_row_params = get_table_rows_params{
                    .json           = true,
                    .code           = fio_oracle_code,
                    .scope          = fio_oracle_scope,
                    .table          = fio_oracles_table};

            get_table_rows_result oracle_result =
                    get_table_rows_ex<key_value_index>(fio_table_row_params, oracle_abi);

            uint8_t oracle_size = oracle_result.rows.size();
            FIO_404_ASSERT(3 <= oracle_size, "Not enough registered oracles.", fioio::ErrorPubAddressNotFound);

            size_t temp_int = 0;

            for (size_t i = 0; i < oracle_size; i++) {
                FIO_404_ASSERT(oracle_result.rows[i]["fees"].size() > 0, "Not enough oracle fee votes.", fioio::ErrorPubAddressNotFound);

                nft_fees.push_back(oracle_result.rows[i]["fees"][temp_int]["fee_amount"].as_uint64());
                token_fees.push_back(oracle_result.rows[i]["fees"][temp_int+1]["fee_amount"].as_uint64());
            }

            sort(nft_fees.begin(), nft_fees.end());
            sort(token_fees.begin(), token_fees.end());

            uint64_t feeNftFinal;
            uint64_t feeTokenFinal;

            if (oracle_size % 2 == 0) {
                feeNftFinal = ((nft_fees[oracle_size / 2 - 1] + nft_fees[oracle_size / 2]) / 2) * oracle_size;
                feeTokenFinal = ((token_fees[oracle_size / 2 - 1] + token_fees[oracle_size / 2]) / 2) * oracle_size;
            } else {
                feeNftFinal = nft_fees[oracle_size / 2] * oracle_size;
                feeTokenFinal = token_fees[oracle_size / 2] * oracle_size;
            }

            oraclefee_record domain = {"wrap_fio_domain", feeNftFinal};
            oraclefee_record tokens = {"wrap_fio_tokens", feeTokenFinal};
            final_fees.push_back(domain);
            final_fees.push_back(tokens);

            result.oracle_fees = final_fees;
            return result;
        }


        /*** v1/chain/get_whitelist
        * Retrieves the whitelist associated with the specified public key
        * @param p
        * @return result
        */
        read_only::get_whitelist_result read_only::get_whitelist(const read_only::get_whitelist_params &p) const {

            get_whitelist_result result;
            FIO_404_ASSERT(p.fio_public_key.length() == FIOPUBLICKEYLENGTH, "No FIO names", fioio::ErrorNoFIONames);

            string account_name;
            fioio::key_to_account(p.fio_public_key, account_name);

            name account = name{account_name};

            const abi_def abi = eosio::chain_apis::get_abi(db, fio_whitelst_code);

            get_table_rows_params table_row_params = get_table_rows_params{
                    .json        = true,
                    .code        = fio_whitelst_code,
                    .scope       = fio_whitelst_scope,
                    .table       = fio_whitelist_table,
                    .lower_bound = boost::lexical_cast<string>(account.value),
                    .upper_bound = boost::lexical_cast<string>(account.value),
                    .key_type       = "i64",
                    .index_position ="2"};

            get_table_rows_result table_rows_result = get_table_rows_by_seckey<index64_index, uint64_t>(
                    table_row_params, abi, [](uint64_t v) -> uint64_t {
                        return v;
                    });

            FIO_400_ASSERT(!table_rows_result.rows.empty(), "fio_public_key", p.fio_public_key, "No whitelist",
                           fioio::ErrorNoFeesFoundForEndpoint);

            for (size_t pos = 0; pos < table_rows_result.rows.size(); pos++) {
                string lookup_index = table_rows_result.rows[pos]["lookupindex"].as_string();
                string content = table_rows_result.rows[pos]["content"].as_string();

                whitelist_info wi{lookup_index, content};
                result.whitelisted_parties.push_back(wi);
            }

            return result;
        }

        /*** v1/chain/check_whitelist
        * returns true if the specified fio_public_key_hash is in the whitelist, false if not.
        * @param p
        * @return result
        */
        read_only::check_whitelist_result read_only::check_whitelist(const read_only::check_whitelist_params &p) const {

            check_whitelist_result result;
            FIO_404_ASSERT(p.fio_public_key_hash.size() == 19 || p.fio_public_key_hash.size() == 18, "No FIO names",
                           fioio::ErrorNoFIONames);

            result.in_whitelist = 0;

            uint64_t fio_pub_key_hash = eosio::string_to_uint64_t(p.fio_public_key_hash.c_str());

            const abi_def abi = eosio::chain_apis::get_abi(db, fio_whitelst_code);

            get_table_rows_params table_row_params = get_table_rows_params{
                    .json        = true,
                    .code        = fio_whitelst_code,
                    .scope       = fio_whitelst_scope,
                    .table       = fio_whitelist_table,
                    .lower_bound = boost::lexical_cast<string>(fio_pub_key_hash),
                    .upper_bound = boost::lexical_cast<string>(fio_pub_key_hash),
                    .key_type       = "i64",
                    .index_position ="3"};

            get_table_rows_result table_rows_result = get_table_rows_by_seckey<index64_index, uint64_t>(
                    table_row_params, abi, [](uint64_t v) -> uint64_t {
                        return v;
                    });

            if (!table_rows_result.rows.empty()) {
                result.in_whitelist = 1;
            }

            return result;
        }

        /***
        * Lookup address by FIO name.
        * @param p Input is FIO name(.fio_name) and chain name(.chain). .chain is allowed to be null/empty, in which case this will bea domain only lookup.
        * @return .is_registered will be true if a match is found, else false. .is_domain is true upon domain match. .address is set if found. .expiration is set upon match.
        */
        read_only::get_pub_address_result
        read_only::get_pub_address(const read_only::get_pub_address_params &p) const {
            fioio::FioAddress fa;
            fioio::getFioAddressStruct(p.fio_address, fa);
            // assert if empty fio name

            FIO_400_ASSERT(validateFioNameFormat(fa), "fio_address", fa.fioaddress, "Invalid FIO Address",
                           fioio::ErrorInvalidFioNameFormat);
            FIO_400_ASSERT(!fa.domainOnly, "fio_address", fa.fioaddress, "Invalid FIO Address",
                           fioio::ErrorInvalidFioNameFormat);
            FIO_400_ASSERT(fioio::validateTokenNameFormat(p.token_code), "token_code", p.token_code,
                           "Invalid Token Code",
                           fioio::ErrorTokenCodeInvalid);
            FIO_400_ASSERT(fioio::validateChainNameFormat(p.chain_code), "chain_code", p.chain_code,
                           "Invalid Chain Code",
                           fioio::ErrorTokenCodeInvalid);

            const name code = ::eosio::string_to_name("fio.address");
            const abi_def abi = eosio::chain_apis::get_abi(db, code);
            const uint128_t name_hash = fioio::string_to_uint128_t(fa.fioaddress.c_str());
            const uint128_t domain_hash = fioio::string_to_uint128_t(fa.fiodomain.c_str());
            const string chainCode = fioio::makeLowerCase(p.chain_code);
            const string tokenCode = fioio::makeLowerCase(p.token_code);
            const string defaultCode = "*";

            //these are the results for the table searches for domain ansd fio name
            get_table_rows_result domain_result;
            get_table_rows_result fioname_result;
            get_table_rows_result name_result;

            get_pub_address_result result;

            result.public_address = "";

            std::string hexvaldomainhash = "0x";
            hexvaldomainhash.append(
                    fioio::to_hex_little_endian(reinterpret_cast<const char *>(&domain_hash), sizeof(domain_hash)));

            get_table_rows_params name_table_row_params = get_table_rows_params{.json=true,
                    .code=code,
                    .scope=fio_system_scope,
                    .table=fio_domains_table,
                    .lower_bound=hexvaldomainhash,
                    .upper_bound=hexvaldomainhash,
                    .encode_type="hex",
                    .index_position ="4"};

            domain_result = get_table_rows_by_seckey<index128_index, uint128_t>(
                    name_table_row_params, abi, [](uint128_t v) -> uint128_t {
                        return v;
                    });

            FIO_404_ASSERT(!domain_result.rows.empty(), "Public address not found", fioio::ErrorPubAddressNotFound);

            uint32_t domain_expiration = (uint32_t) (domain_result.rows[0]["expiration"].as_uint64());
            uint32_t present_time = (uint32_t) time(0);
            FIO_400_ASSERT(!(present_time > domain_expiration), "fio_address", p.fio_address, "Invalid FIO Address",
                           fioio::ErrorFioNameEmpty);

            //set name result to be the domain results.
            name_result = domain_result;

            if (!fa.fioname.empty()) {

                std::string hexvalnamehash = "0x";
                hexvalnamehash.append(
                        fioio::to_hex_little_endian(reinterpret_cast<const char *>(&name_hash), sizeof(name_hash)));

                get_table_rows_params name_table_row_params = get_table_rows_params{.json=true,
                        .code=fio_system_code,
                        .scope=fio_system_scope,
                        .table=fio_address_table,
                        .lower_bound=hexvalnamehash,
                        .upper_bound=hexvalnamehash,
                        .encode_type="hex",
                        .index_position ="5"};

                fioname_result = get_table_rows_by_seckey<index128_index, uint128_t>(
                        name_table_row_params, abi, [](uint128_t v) -> uint128_t {
                            return v;
                        });

                FIO_404_ASSERT(!fioname_result.rows.empty(), "Public address not found",
                               fioio::ErrorPubAddressNotFound);

                uint32_t name_expiration = 4294967295; //Sunday, February 7, 2106 6:28:15 AM GMT+0000 (Max 32 bit expiration)
                FIO_400_ASSERT(!(present_time > domain_expiration), "fio_address", p.fio_address, "Invalid FIO Address",
                               fioio::ErrorFioNameEmpty);

                //set the result to the name results
                name_result = fioname_result;
            } else {
                FIO_404_ASSERT(!p.fio_address.empty(), "Public address not found", fioio::ErrorPubAddressNotFound);
            }

            string defaultPubAddress = "";

            for (int i = 0; i < name_result.rows[0]["addresses"].size(); i++) {
                string tToken = fioio::makeLowerCase(name_result.rows[0]["addresses"][i]["token_code"].as_string());
                string tChain = fioio::makeLowerCase(name_result.rows[0]["addresses"][i]["chain_code"].as_string());

                if ((tToken == tokenCode) && (tChain == chainCode)) {
                    result.public_address = name_result.rows[0]["addresses"][i]["public_address"].as_string();
                    break;
                }
                if ((tToken == defaultCode) && (tChain == chainCode)) {
                    defaultPubAddress = name_result.rows[0]["addresses"][i]["public_address"].as_string();
                }
                if (i == name_result.rows[0]["addresses"].size() - 1 && defaultPubAddress != "" ) {
                    result.public_address = defaultPubAddress;
                }
            }
            //   // Pick out chain specific key and populate result
            FIO_404_ASSERT(!(result.public_address == ""), "Public address not found", fioio::ErrorPubAddressNotFound);

            return result;
        } // get_pub_address


        /***
        * Get all addresses by FIO name.
        * @param p Input is FIO name(.fio_name) and chain name(.chain). .chain is allowed to be null/empty, in which case this will bea domain only lookup.
        * @return n/a
        */
        read_only::get_pub_addresses_result
        read_only::get_pub_addresses(const read_only::get_pub_addresses_params &p) const {
            fioio::FioAddress fa;
            fioio::getFioAddressStruct(p.fio_address, fa);
            // assert if empty fio name

            FIO_400_ASSERT(validateFioNameFormat(fa), "fio_address", fa.fioaddress, "Invalid FIO Address format",
                           fioio::ErrorInvalidFioNameFormat);
            FIO_400_ASSERT(!fa.domainOnly, "fio_address", fa.fioaddress, "Invalid FIO Address format",
                           fioio::ErrorInvalidFioNameFormat);

            FIO_400_ASSERT(p.limit >= 0, "limit", to_string(p.limit), "Invalid limit",
                           fioio::ErrorPagingInvalid);
            FIO_400_ASSERT(p.offset >= 0, "offset", to_string(p.offset), "Invalid offset",
                           fioio::ErrorPagingInvalid);

            uint32_t search_limit = p.limit, search_offset = p.offset;

            const name code = ::eosio::string_to_name("fio.address");
            const abi_def abi = eosio::chain_apis::get_abi(db, code);
            const uint128_t name_hash = fioio::string_to_uint128_t(fa.fioaddress.c_str());
            const uint128_t domain_hash = fioio::string_to_uint128_t(fa.fiodomain.c_str());

            //these are the results for the table searches for domain ansd fio name
            get_table_rows_result domain_result;
            get_table_rows_result fioname_result;
            get_table_rows_result name_result;

            get_pub_addresses_result result;

            std::string hexvaldomainhash = "0x";
            hexvaldomainhash.append(
                    fioio::to_hex_little_endian(reinterpret_cast<const char *>(&domain_hash), sizeof(domain_hash)));

            get_table_rows_params name_table_row_params = get_table_rows_params{.json=true,
                    .code=code,
                    .scope=fio_system_scope,
                    .table=fio_domains_table,
                    .lower_bound=hexvaldomainhash,
                    .upper_bound=hexvaldomainhash,
                    .encode_type="hex",
                    .index_position ="4"};

            domain_result = get_table_rows_by_seckey<index128_index, uint128_t>(
                    name_table_row_params, abi, [](uint128_t v) -> uint128_t {
                        return v;
                    });

            FIO_404_ASSERT(!domain_result.rows.empty(), "FIO Address does not exist", fioio::ErrorPubAddressNotFound);

            uint32_t domain_expiration = (uint32_t) (domain_result.rows[0]["expiration"].as_uint64());
            uint32_t present_time = (uint32_t) time(0);
            FIO_400_ASSERT(!(present_time > domain_expiration), "fio_address", p.fio_address, "FIO Address does not exist",
                           fioio::ErrorFioNameEmpty);

            //set name result to be the domain results.
            name_result = domain_result;

            if (!fa.fioname.empty()) {

                std::string hexvalnamehash = "0x";
                hexvalnamehash.append(
                        fioio::to_hex_little_endian(reinterpret_cast<const char *>(&name_hash), sizeof(name_hash)));

                get_table_rows_params name_table_row_params = get_table_rows_params{.json=true,
                        .code=fio_system_code,
                        .scope=fio_system_scope,
                        .table=fio_address_table,
                        .lower_bound=hexvalnamehash,
                        .upper_bound=hexvalnamehash,
                        .encode_type="hex",
                        .index_position ="5"};

                fioname_result = get_table_rows_by_seckey<index128_index, uint128_t>(
                        name_table_row_params, abi, [](uint128_t v) -> uint128_t {
                            return v;
                        });

                FIO_404_ASSERT(!fioname_result.rows.empty(), "FIO Address does not exist",
                               fioio::ErrorPubAddressNotFound);

                uint32_t name_expiration = 4294967295; //Sunday, February 7, 2106 6:28:15 AM GMT+0000 (Max 32 bit expiration)
                FIO_400_ASSERT(!(present_time > domain_expiration), "fio_address", p.fio_address, "FIO Address does not exist",
                               fioio::ErrorFioNameEmpty);

                //set the result to the name results
                name_result = fioname_result;
            } else {
              // This condition should never be met, all FIO Addresses will have at least 1 public address at minimum (The FIO Public Key)
                FIO_404_ASSERT(!p.fio_address.empty(), "Public Addresses not found", fioio::ErrorPubAddressNotFound);
            }

            address_info public_address_info;
            int i = 0;

            if (search_offset < name_result.rows[0]["addresses"].size()) {

              int64_t leftover = (name_result.rows[0]["addresses"].size() - 1) - (search_offset + search_limit);
              if(leftover < 0) {
                leftover = 0;
              }
              result.more = leftover;
                for (size_t pos = 0 + search_offset; pos < name_result.rows[0]["addresses"].size(); pos++) {
                if((search_limit > 0) && (pos - search_offset >= search_limit)) {
                    break;
                }
                public_address_info.public_address = name_result.rows[0]["addresses"][pos]["public_address"].as_string();
                public_address_info.token_code = name_result.rows[0]["addresses"][pos]["token_code"].as_string();
                public_address_info.chain_code = name_result.rows[0]["addresses"][pos]["chain_code"].as_string();
                result.public_addresses.push_back(public_address_info);
                result.more = (name_result.rows[0]["addresses"].size() - pos) - 1;
              }

            }
            // vector is empty, throw 404
            FIO_404_ASSERT(!result.public_addresses.empty(), "Public Addresses not found", fioio::ErrorPubAddressNotFound);

            return result;
        } // get_pub_addresses


        /***
        * Checks if a FIO Address or FIO Domain is available for registration.
        * @param p
        * @return result.fio_name, result.is_registered
        */
        read_only::avail_check_result read_only::avail_check(const read_only::avail_check_params &p) const {

            avail_check_result result;

            fioio::FioAddress fa;
            fioio::getFioAddressStruct(p.fio_name, fa);

            FIO_400_ASSERT(validateFioNameFormat(fa), "fio_name", fa.fioaddress, "Invalid FIO Name", fioio::ErrorInvalidFioNameFormat);

            const abi_def abi = eosio::chain_apis::get_abi(db, fio_system_code);
            const uint128_t name_hash = fioio::string_to_uint128_t(fa.fioaddress.c_str());
            const uint128_t domain_hash = fioio::string_to_uint128_t(fa.fiodomain.c_str());
            get_table_rows_result fioname_result;
            get_table_rows_result name_result;
            get_table_rows_result domain_result;

            std::string hexvaldomainhash = "0x";
            hexvaldomainhash.append(
                    fioio::to_hex_little_endian(reinterpret_cast<const char *>(&domain_hash), sizeof(domain_hash)));

            get_table_rows_params name_table_row_params = get_table_rows_params{.json=true,
                    .code=fio_system_code,
                    .scope=fio_system_scope,
                    .table=fio_domains_table,
                    .lower_bound=hexvaldomainhash,
                    .upper_bound=hexvaldomainhash,
                    .encode_type="hex",
                    .index_position ="4"};

            domain_result = get_table_rows_by_seckey<index128_index, uint128_t>(
                    name_table_row_params, abi, [](uint128_t v) -> uint128_t {
                        return v;
                    });

            if (!fa.fioname.empty()) {
                std::string hexvalnamehash = "0x";
                hexvalnamehash.append(
                        fioio::to_hex_little_endian(reinterpret_cast<const char *>(&name_hash), sizeof(name_hash)));

                get_table_rows_params name_table_row_params = get_table_rows_params{.json=true,
                        .code=fio_system_code,
                        .scope=fio_system_scope,
                        .table=fio_address_table,
                        .lower_bound=hexvalnamehash,
                        .upper_bound=hexvalnamehash,
                        .encode_type="hex",
                        .index_position ="5"};
                
                fioname_result = get_table_rows_by_seckey<index128_index, uint128_t>(
                        name_table_row_params, abi, [](uint128_t v) -> uint128_t {
                            return v;
                        });

                if (fioname_result.rows.empty()) {
                    return result;
                }

               //if the address is there then its registered, let the logic fall through.
            }

            if (domain_result.rows.empty()) {
                return result;
            }

            //if its there then it is_registered, regardless of expiration

            // name checked and set
            result.is_registered = true;
            return result;
        }
        /*****************End of FIO API******************************/
        /*************************************************************/


        read_only::get_table_rows_result read_only::get_table_rows(const read_only::get_table_rows_params &p) const {
            const abi_def abi = eosio::chain_apis::get_abi(db, p.code);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
            bool primary = false;
            auto table_with_index = get_table_index_name(p, primary);
            if (primary) {
                EOS_ASSERT(p.table == table_with_index, chain::contract_table_query_exception,
                           "Invalid table name ${t}", ("t", p.table));
                auto table_type = get_table_type(abi, p.table);
                if (table_type == KEYi64 || p.key_type == "i64" || p.key_type == "name") {
                    return get_table_rows_ex<key_value_index>(p, abi);
                }
                EOS_ASSERT(false, chain::contract_table_query_exception, "Invalid table type ${type}",
                           ("type", table_type)("abi", abi));
            } else {
                EOS_ASSERT(!p.key_type.empty(), chain::contract_table_query_exception,
                           "key type required for non-primary index");

                if (p.key_type == chain_apis::i64 || p.key_type == "name") {
                    return get_table_rows_by_seckey<index64_index, uint64_t>(p, abi, [](uint64_t v) -> uint64_t {
                        return v;
                    });
                } else if (p.key_type == chain_apis::i128) {
                    return get_table_rows_by_seckey<index128_index, uint128_t>(p, abi, [](uint128_t v) -> uint128_t {
                        return v;
                    });
                } else if (p.key_type == chain_apis::i256) {
                    if (p.encode_type == chain_apis::hex) {
                        using conv = keytype_converter<chain_apis::sha256, chain_apis::hex>;
                        return get_table_rows_by_seckey<conv::index_type, conv::input_type>(p, abi, conv::function());
                    }
                    using conv = keytype_converter<chain_apis::i256>;
                    return get_table_rows_by_seckey<conv::index_type, conv::input_type>(p, abi, conv::function());
                } else if (p.key_type == chain_apis::float64) {
                    return get_table_rows_by_seckey<index_double_index, double>(p, abi, [](double v) -> float64_t {
                        float64_t f = *(float64_t *) &v;
                        return f;
                    });
                } else if (p.key_type == chain_apis::float128) {
                    return get_table_rows_by_seckey<index_long_double_index, double>(p, abi,
                                                                                     [](double v) -> float128_t {
                                                                                         float64_t f = *(float64_t *) &v;
                                                                                         float128_t f128;
                                                                                         f64_to_f128M(f, &f128);
                                                                                         return f128;
                                                                                     });
                } else if (p.key_type == chain_apis::sha256) {
                    using conv = keytype_converter<chain_apis::sha256, chain_apis::hex>;
                    return get_table_rows_by_seckey<conv::index_type, conv::input_type>(p, abi, conv::function());
                } else if (p.key_type == chain_apis::ripemd160) {
                    using conv = keytype_converter<chain_apis::ripemd160, chain_apis::hex>;
                    return get_table_rows_by_seckey<conv::index_type, conv::input_type>(p, abi, conv::function());
                }
                EOS_ASSERT(false, chain::contract_table_query_exception, "Unsupported secondary index type: ${t}",
                           ("t", p.key_type));
            }
#pragma GCC diagnostic pop
        }

        read_only::get_table_by_scope_result
        read_only::get_table_by_scope(const read_only::get_table_by_scope_params &p) const {
            read_only::get_table_by_scope_result result;
            const auto &d = db.db();

            const auto &idx = d.get_index<chain::table_id_multi_index, chain::by_code_scope_table>();
            auto lower_bound_lookup_tuple = std::make_tuple(p.code.value, std::numeric_limits<uint64_t>::lowest(),
                                                            p.table.value);
            auto upper_bound_lookup_tuple = std::make_tuple(p.code.value, std::numeric_limits<uint64_t>::max(),
                                                            (p.table.empty() ? std::numeric_limits<uint64_t>::max()
                                                                             : p.table.value));

            if (p.lower_bound.size()) {
                uint64_t scope = convert_to_type<uint64_t>(p.lower_bound, "lower_bound scope");
                std::get<1>(lower_bound_lookup_tuple) = scope;
            }

            if (p.upper_bound.size()) {
                uint64_t scope = convert_to_type<uint64_t>(p.upper_bound, "upper_bound scope");
                std::get<1>(upper_bound_lookup_tuple) = scope;
            }

            if (upper_bound_lookup_tuple < lower_bound_lookup_tuple)
                return result;

            auto walk_table_range = [&](auto itr, auto end_itr) {
                auto cur_time = fc::time_point::now();
                auto end_time = cur_time + fc::microseconds(1000 * 10); /// 10ms max time
                for (unsigned int count = 0; cur_time <= end_time && count < p.limit &&
                                             itr != end_itr; ++itr, cur_time = fc::time_point::now()) {
                    if (p.table && itr->table != p.table) continue;

                    result.rows.push_back({itr->code, itr->scope, itr->table, itr->payer, itr->count});

                    ++count;
                }
                if (itr != end_itr) {
                    result.more = string(itr->scope);
                }
            };

            auto lower = idx.lower_bound(lower_bound_lookup_tuple);
            auto upper = idx.upper_bound(upper_bound_lookup_tuple);
            if (p.reverse && *p.reverse) {
                walk_table_range(boost::make_reverse_iterator(upper), boost::make_reverse_iterator(lower));
            } else {
                walk_table_range(lower, upper);
            }

            return result;
        }

        vector<asset> read_only::get_currency_balance(const read_only::get_currency_balance_params &p) const {

            const abi_def abi = eosio::chain_apis::get_abi(db, p.code);
            (void) get_table_type(abi, "accounts");

            vector<asset> results;
            walk_key_value_table(p.code, p.account, N(accounts), [&](const key_value_object &obj) {
                EOS_ASSERT(obj.value.size() >= sizeof(asset), chain::asset_type_exception, "Invalid data on table");

                asset cursor;
                fc::datastream<const char *> ds(obj.value.data(), obj.value.size());
                fc::raw::unpack(ds, cursor);

                EOS_ASSERT(cursor.get_symbol().valid(), chain::asset_type_exception, "Invalid asset");

                if (!p.symbol || boost::iequals(cursor.symbol_name(), *p.symbol)) {
                    results.emplace_back(cursor);
                }

                // return false if we are looking for one and found it, true otherwise
                return !(p.symbol && boost::iequals(cursor.symbol_name(), *p.symbol));
            });

            return results;
        }

        fc::variant read_only::get_currency_stats(const read_only::get_currency_stats_params &p) const {
            fc::mutable_variant_object results;

            const abi_def abi = eosio::chain_apis::get_abi(db, p.code);
            (void) get_table_type(abi, "stat");

            uint64_t scope = (eosio::chain::string_to_symbol(0, boost::algorithm::to_upper_copy(p.symbol).c_str())
                    >> 8);

            walk_key_value_table(p.code, scope, N(stat), [&](const key_value_object &obj) {
                EOS_ASSERT(obj.value.size() >= sizeof(read_only::get_currency_stats_result),
                           chain::asset_type_exception, "Invalid data on table");

                fc::datastream<const char *> ds(obj.value.data(), obj.value.size());
                read_only::get_currency_stats_result result;

                fc::raw::unpack(ds, result.supply);
                fc::raw::unpack(ds, result.max_supply);
                fc::raw::unpack(ds, result.issuer);

                results[result.supply.symbol_name()] = result;
                return true;
            });

            return results;
        }

        fc::variant get_global_row(const database &db, const abi_def &abi, const abi_serializer &abis,
                                   const fc::microseconds &abi_serializer_max_time_ms, bool shorten_abi_errors) {
            const auto table_type = get_table_type(abi, N(global));
            EOS_ASSERT(table_type == read_only::KEYi64, chain::contract_table_query_exception,
                       "Invalid table type ${type} for table global", ("type", table_type));

            const auto *const table_id = db.find<chain::table_id_object, chain::by_code_scope_table>(
                    boost::make_tuple(config::system_account_name, config::system_account_name, N(global)));
            EOS_ASSERT(table_id, chain::contract_table_query_exception, "Missing table global");

            const auto &kv_index = db.get_index<key_value_index, by_scope_primary>();
            const auto it = kv_index.find(boost::make_tuple(table_id->id, N(global)));
            EOS_ASSERT(it != kv_index.end(), chain::contract_table_query_exception, "Missing row in table global");

            vector<char> data;
            read_only::copy_inline_row(*it, data);
            return abis.binary_to_variant(abis.get_table_type(N(global)), data, abi_serializer_max_time_ms,
                                          shorten_abi_errors);
        }


        read_only::get_producers_result read_only::get_producers(const read_only::get_producers_params &p) const {
            const abi_def abi = eosio::chain_apis::get_abi(db, config::system_account_name);
            const auto table_type = get_table_type(abi, N(producers));
            const abi_serializer abis{abi, abi_serializer_max_time};

            EOS_ASSERT(table_type == KEYi64, chain::contract_table_query_exception,
                       "Invalid table type ${type} for table producers", ("type", table_type));

            const auto &d = db.db();
            const auto lower = name{p.lower_bound};

            FIO_400_ASSERT(p.limit >= 0, "limit", to_string(p.limit), "Invalid limit",
                           fioio::ErrorPagingInvalid);
            //FIO_400_ASSERT(!fioio::isStringInt(p.lower_bound), "lower_bound", p.lower_bound, "Invalid lower bound",
            //               fioio::ErrorPagingInvalid);

            static const uint8_t secondary_index_num = 0;
            const auto *const table_id = d.find<chain::table_id_object, chain::by_code_scope_table>(
                    boost::make_tuple(config::system_account_name, config::system_account_name, N(producers)));
            const auto *const secondary_table_id = d.find<chain::table_id_object, chain::by_code_scope_table>(
                    boost::make_tuple(config::system_account_name, config::system_account_name,
                                      N(producers) | secondary_index_num));
            EOS_ASSERT(table_id && secondary_table_id, chain::contract_table_query_exception,
                       "Missing producers table");

            const auto &kv_index = d.get_index<key_value_index, by_scope_primary>();
            const auto &secondary_index = d.get_index<index_double_index>().indices();
            const auto &secondary_index_by_primary = secondary_index.get<by_primary>();
            const auto &secondary_index_by_secondary = secondary_index.get<by_secondary>();

            read_only::get_producers_result result;
            const auto stopTime = fc::time_point::now() + fc::microseconds(1000 * 10); // 10ms
            vector<char> data;

            auto it = [&] {
                if (lower.value == 0)
                    return secondary_index_by_secondary.lower_bound(
                            boost::make_tuple(secondary_table_id->id,
                                              to_softfloat64(std::numeric_limits<double>::lowest()), 0));
                else
                    return secondary_index.project<by_secondary>(
                            secondary_index_by_primary.lower_bound(
                                    boost::make_tuple(secondary_table_id->id, lower.value)));
            }();

            for (; it != secondary_index_by_secondary.end() && it->t_id == secondary_table_id->id; ++it) {
                if (result.producers.size() >= p.limit || fc::time_point::now() > stopTime) {
                    result.more = name{it->primary_key}.to_string();
                    break;
                }
                copy_inline_row(*kv_index.find(boost::make_tuple(table_id->id, it->primary_key)), data);
                if (p.json)
                    result.producers.emplace_back(
                            abis.binary_to_variant(abis.get_table_type(N(producers)), data, abi_serializer_max_time,
                                                   shorten_abi_errors));
                else
                    result.producers.emplace_back(fc::variant(data));
            }

            result.total_producer_vote_weight = get_global_row(d, abi, abis, abi_serializer_max_time,
                                                               shorten_abi_errors)["total_producer_vote_weight"].as_double();
            return result;
        } /*catch (...) {
            read_only::get_producers_result result;

            for (auto p : db.active_producers().producers) {
                fc::variant row = fc::mutable_variant_object()
                        ("fio_address", p.producer_name)
                        ("producer_key", p.block_signing_key)
                        ("url", "")
                        ("total_votes", 0.0f);

                result.producers.push_back(row);
            }

            return result;
        } */

        read_only::get_producer_schedule_result
        read_only::get_producer_schedule(const read_only::get_producer_schedule_params &p) const {
            read_only::get_producer_schedule_result result;
            to_variant(db.active_producers(), result.active);
            if (!db.pending_producers().producers.empty())
                to_variant(db.pending_producers(), result.pending);
            auto proposed = db.proposed_producers();
            if (proposed && !proposed->producers.empty())
                to_variant(*proposed, result.proposed);
            return result;
        }

        template<typename Api>
        struct resolver_factory {
            static auto make(const Api *api, const fc::microseconds &max_serialization_time) {
                return [api, max_serialization_time](const account_name &name) -> optional<abi_serializer> {
                    const auto *accnt = api->db.db().template find<account_object, by_name>(name);
                    if (accnt != nullptr) {
                        abi_def abi;
                        if (abi_serializer::to_abi(accnt->abi, abi)) {
                            return abi_serializer(abi, max_serialization_time);
                        }
                    }

                    return optional<abi_serializer>();
                };
            }
        };

        template<typename Api>
        auto make_resolver(const Api *api, const fc::microseconds &max_serialization_time) {
            return resolver_factory<Api>::make(api, max_serialization_time);
        }


        read_only::get_scheduled_transactions_result
        read_only::get_scheduled_transactions(const read_only::get_scheduled_transactions_params &p) const {
            const auto &d = db.db();

            const auto &idx_by_delay = d.get_index<generated_transaction_multi_index, by_delay>();
            auto itr = ([&]() {
                if (!p.lower_bound.empty()) {
                    try {
                        auto when = time_point::from_iso_string(p.lower_bound);
                        return idx_by_delay.lower_bound(boost::make_tuple(when));
                    } catch (...) {
                        try {
                            auto txid = transaction_id_type(p.lower_bound);
                            const auto &by_txid = d.get_index<generated_transaction_multi_index, by_trx_id>();
                            auto itr = by_txid.find(txid);
                            if (itr == by_txid.end()) {
                                EOS_THROW(transaction_exception, "Unknown Transaction ID: ${txid}", ("txid", txid));
                            }

                            return d.get_index<generated_transaction_multi_index>().indices().project<by_delay>(itr);

                        } catch (...) {
                            return idx_by_delay.end();
                        }
                    }
                } else {
                    return idx_by_delay.begin();
                }
            })();

            read_only::get_scheduled_transactions_result result;

            auto resolver = make_resolver(this, abi_serializer_max_time);

            uint32_t remaining = p.limit;
            auto time_limit = fc::time_point::now() + fc::microseconds(1000 * 10); /// 10ms max time
            while (itr != idx_by_delay.end() && remaining > 0 && time_limit > fc::time_point::now()) {
                auto row = fc::mutable_variant_object()
                        ("trx_id", itr->trx_id)
                        ("sender", itr->sender)
                        ("sender_id", itr->sender_id)
                        ("payer", itr->payer)
                        ("delay_until", itr->delay_until)
                        ("expiration", itr->expiration)
                        ("published", itr->published);

                if (p.json) {
                    fc::variant pretty_transaction;

                    transaction trx;
                    fc::datastream<const char *> ds(itr->packed_trx.data(), itr->packed_trx.size());
                    fc::raw::unpack(ds, trx);

                    abi_serializer::to_variant(trx, pretty_transaction, resolver, abi_serializer_max_time);
                    row("transaction", pretty_transaction);
                } else {
                    auto packed_transaction = bytes(itr->packed_trx.begin(), itr->packed_trx.end());
                    row("transaction", packed_transaction);
                }

                result.transactions.emplace_back(std::move(row));
                ++itr;
                remaining--;
            }

            if (itr != idx_by_delay.end()) {
                result.more = string(itr->trx_id);
            }

            return result;
        }

        fc::variant read_only::get_block(const read_only::get_block_params &params) const {
            signed_block_ptr block;
            optional<uint64_t> block_num;

            EOS_ASSERT(!params.block_num_or_id.empty() && params.block_num_or_id.size() <= 64,
                       chain::block_id_type_exception,
                       "Invalid Block number or ID, must be greater than 0 and less than 64 characters"
            );

            try {
                block_num = fc::to_uint64(params.block_num_or_id);
            } catch (...) {}

            if (block_num.valid()) {
                block = db.fetch_block_by_number(*block_num);
            } else {
                try {
                    block = db.fetch_block_by_id(fc::variant(params.block_num_or_id).as<block_id_type>());
                } EOS_RETHROW_EXCEPTIONS(chain::block_id_type_exception, "Invalid block ID: ${block_num_or_id}",
                                         ("block_num_or_id", params.block_num_or_id))
            }

            EOS_ASSERT(block, unknown_block_exception, "Could not find block: ${block}",
                       ("block", params.block_num_or_id));

            fc::variant pretty_output;
            abi_serializer::to_variant(*block, pretty_output, make_resolver(this, abi_serializer_max_time),
                                       abi_serializer_max_time);

            uint32_t ref_block_prefix = block->id()._hash[1];

            return fc::mutable_variant_object(pretty_output.get_object())
                    ("id", block->id())
                    ("block_num", block->block_num())
                    ("ref_block_prefix", ref_block_prefix);
        }

        fc::variant read_only::get_block_header_state(const get_block_header_state_params &params) const {
            block_state_ptr b;
            optional<uint64_t> block_num;
            std::exception_ptr e;
            try {
                block_num = fc::to_uint64(params.block_num_or_id);
            } catch (...) {}

            if (block_num.valid()) {
                b = db.fetch_block_state_by_number(*block_num);
            } else {
                try {
                    b = db.fetch_block_state_by_id(fc::variant(params.block_num_or_id).as<block_id_type>());
                } EOS_RETHROW_EXCEPTIONS(chain::block_id_type_exception, "Invalid block ID: ${block_num_or_id}",
                                         ("block_num_or_id", params.block_num_or_id))
            }

            EOS_ASSERT(b, unknown_block_exception, "Could not find reversible block: ${block}",
                       ("block", params.block_num_or_id));

            fc::variant vo;
            fc::to_variant(static_cast<const block_header_state &>(*b), vo);
            return vo;
        }

        void read_write::push_block(read_write::push_block_params &&params,
                                    next_function<read_write::push_block_results> next) {
            try {
                app().get_method<incoming::methods::block_sync>()(std::make_shared<signed_block>(std::move(params)));
                next(read_write::push_block_results{});
            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } catch (const std::bad_alloc &) {
                chain_plugin::handle_bad_alloc();
            } CATCH_AND_CALL(next);
        }


/***
 *  new_funds_request- This api method will invoke the fio.request.obt smart contract for newfundsreq this api method is
 * intended add a new request for funds to the index tables of the chain..
 * @param p Accepts a variant object of from a pushed fio transaction that contains a public key in packed actions
 * @return result, result.processed (fc::variant) json blob meeting the api specification.
 */
        void read_write::new_funds_request(const new_funds_request_params &params,
                                           chain::plugin_interface::next_function<new_funds_request_results> next) {
            try {
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("new_funds_request called");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                } EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);

                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.reqobt", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "newfundsreq", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }

                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::new_funds_request_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }

        void read_write::wrap_fio_tokens(const read_write::wrap_fio_tokens_params &params,
                                         next_function<read_write::wrap_fio_tokens_results> next) {
            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("wrap_fio_tokens called");

                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                } EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);

                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.oracle", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "wraptokens", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }

                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::wrap_fio_tokens_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }

        void read_write::wrap_fio_domains(const read_write::wrap_fio_domains_params &params,
                                         next_function<read_write::wrap_fio_domains_results> next) {
            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("wrap_fio_domains called");

                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                } EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);

                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.oracle", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "wrapdomains", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }

                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::wrap_fio_domains_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }

/***
* cancel funds request - This api method will invoke the fio.request.obt smart contract for cancelfndreq. this api method is
* intended to add the json passed into this method to the block log so that it can be scraped as necessary.
* @param p Accepts a variant object of from a pushed fio transaction that contains a public key in packed actions
* @return result, result.processed (fc::variant) json blob meeting the api specification.
*/
        void read_write::cancel_funds_request(const cancel_funds_request_params &params,
                                              chain::plugin_interface::next_function<cancel_funds_request_results> next) {
            try {
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("cancel_funds_request called");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                } EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);

                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.reqobt", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "cancelfndreq", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }

                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::cancel_funds_request_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }

        /***
* reject funds request - This api method will invoke the fio.request.obt smart contract for rejectfndreq. this api method is
* intended to add the json passed into this method to the block log so that it can be scraped as necessary.
* @param p Accepts a variant object of from a pushed fio transaction that contains a public key in packed actions
* @return result, result.processed (fc::variant) json blob meeting the api specification.
*/
        void read_write::reject_funds_request(const reject_funds_request_params &params,
                                              chain::plugin_interface::next_function<reject_funds_request_results> next) {
            try {
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("reject_funds_request called");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                } EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);

                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.reqobt", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "rejectfndreq", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }

                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::reject_funds_request_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }


        /***
* record_obt_data - This api method will invoke the fio.request.obt smart contract for recordobt. this api method is
* intended to add the json passed into this method to the block log so that it can be scraped as necessary.
* @param p Accepts a variant object of from a pushed fio transaction that contains a public key in packed actions
* @return result, result.processed (fc::variant) json blob meeting the api specification.
*/
        void read_write::record_obt_data(const record_obt_data_params &params,
                                         chain::plugin_interface::next_function<record_obt_data_results> next) {
            try {
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                } EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);

                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.reqobt", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "recordobt", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }

                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::record_obt_data_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }

/***
* add_nft - Add an nft to the nfts table table for a registered fio_address
* @param p Accepts a variant object of from a pushed fio transaction that contains a public key in packed actions
* @return result, result.transaction_id (chain::transaction_id_type), result.processed (fc::variant)
*/
        void read_write::add_nft(const read_write::add_nft_params &params,
                                              next_function<read_write::add_nft_results> next) {
            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("add_nft called");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                } EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.address", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "addnft", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }
                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::add_nft_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }


/***
* remove_nft - Remove an nft from the nfts table table for a registered fio_address
* @param p Accepts a variant object of from a pushed fio transaction that contains a public key in packed actions
* @return result, result.transaction_id (chain::transaction_id_type), result.processed (fc::variant)
*/
        void read_write::remove_nft(const read_write::remove_nft_params &params,
                                              next_function<read_write::remove_nft_results> next) {
            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("remove_nft called");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                } EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.address", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "remnft", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }
                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::remove_nft_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }

  /***
  * remove_all_nfts - Push registered nfts for a fio_address to the nft burn queue to be erased later
  * @param p Accepts a variant object of from a pushed fio transaction that contains a public key in packed actions
  * @return result, result.transaction_id (chain::transaction_id_type), result.processed (fc::variant)
  */
          void read_write::remove_all_nfts(const read_write::remove_all_nfts_params &params,
                                                next_function<read_write::remove_all_nfts_results> next) {
              try {
                  FIO_403_ASSERT(params.size() == 4,
                                 fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                  auto pretty_input = std::make_shared<packed_transaction>();
                  auto resolver = make_resolver(this, abi_serializer_max_time);
                  transaction_metadata_ptr ptrx;
                  dlog("remove_all_nfts called");
                  try {
                      abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                      ptrx = std::make_shared<transaction_metadata>(pretty_input);
                  } EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                  transaction trx = pretty_input->get_transaction();
                  vector<action> &actions = trx.actions;
                  dlog("\n");
                  dlog(actions[0].name.to_string());
                  FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);
                  FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                  FIO_403_ASSERT(actions[0].account.to_string() == "fio.address", fioio::InvalidAccountOrAction);
                  FIO_403_ASSERT(actions[0].name.to_string() == "remallnfts", fioio::InvalidAccountOrAction);

                  app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                          const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                      if (result.contains<fc::exception_ptr>()) {
                          next(result.get<fc::exception_ptr>());
                      } else {
                          auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                          try {
                              fc::variant output;
                              try {
                                  output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                              } catch (chain::abi_exception &) {
                                  output = *trx_trace_ptr;
                              }
                              const chain::transaction_id_type &id = trx_trace_ptr->id;
                              next(read_write::remove_all_nfts_results{id, output});
                          } CATCH_AND_CALL(next);
                      }
                  });


              } catch (boost::interprocess::bad_alloc &) {
                  chain_plugin::handle_db_exhaustion();
              } CATCH_AND_CALL(next);
          }


        /***
* add_fio_permission -  add a permission to a grantee account, for details of permissions see fio.perms.hpp
* @param p Accepts a variant object of from a pushed fio transaction that contains a public key in packed actions
* @return result, result.transaction_id (chain::transaction_id_type), result.processed (fc::variant)
*/
        void read_write::add_fio_permission(const read_write::add_fio_permission_params &params,
                                              next_function<read_write::add_fio_permission_results> next) {
            try {


                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction);
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("add fio permission called ");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                } EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.perms", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "addperm", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }
                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::add_fio_permission_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }


        /***
* remove_fio_permission -  remove a permission on a grantee account, for details of permissions see fio.perms.hpp
* @param p Accepts a variant object of from a pushed fio transaction that contains a public key in packed actions
* @return result, result.transaction_id (chain::transaction_id_type), result.processed (fc::variant)
*/
        void read_write::remove_fio_permission(const read_write::remove_fio_permission_params &params,
                                            next_function<read_write::remove_fio_permission_results> next) {
            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction);
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("remove fio permission called");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                } EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.perms", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "remperm", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }
                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::remove_fio_permission_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }


        /***
* Register_fio_name - Register a fio_address or fio_domain into the fionames (fioaddresses) or fiodomains tables respectively
* @param p Accepts a variant object of from a pushed fio transaction that contains a public key in packed actions
* @return result, result.transaction_id (chain::transaction_id_type), result.processed (fc::variant)
*/
        void read_write::register_fio_address(const read_write::register_fio_address_params &params,
                                              next_function<read_write::register_fio_address_results> next) {
            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("register_fio_address called");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                } EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.address", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "regaddress", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }
                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::register_fio_address_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }

/***
* register_fio_domain_address - Registers a fio domain and address
* @param p Accepts a variant object of from a pushed fio transaction that contains a public key in packed actions
* @return result, result.transaction_id (chain::transaction_id_type), result.processed (fc::variant)
*/
void read_write::register_fio_domain_address(const read_write::register_fio_domain_address_params &params,
                                    next_function<read_write::register_fio_domain_address_results> next) {
    try {
        FIO_403_ASSERT(params.size() == 4,
                        fioio::ErrorTransaction); // variant object contains authorization, account, name, data
        auto pretty_input = std::make_shared<packed_transaction>();
        auto resolver = make_resolver(this, abi_serializer_max_time);
        transaction_metadata_ptr ptrx;
        dlog("register_fio_domain_address called");
        try {
            abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
            ptrx = std::make_shared<transaction_metadata>(pretty_input);
        } EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

        transaction trx = pretty_input->get_transaction();
        vector<action> &actions = trx.actions;
        dlog("\n");
        dlog(actions[0].name.to_string());
        FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);
        FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
        FIO_403_ASSERT(actions[0].account.to_string() == "fio.address", fioio::InvalidAccountOrAction);
        FIO_403_ASSERT(actions[0].name.to_string() == "regdomadd", fioio::InvalidAccountOrAction);

        app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
            if (result.contains<fc::exception_ptr>()) {
                next(result.get<fc::exception_ptr>());
            } else {
                auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                try {
                    fc::variant output;
                    try {
                        output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                    } catch (chain::abi_exception &) {
                        output = *trx_trace_ptr;
                    }
                    const chain::transaction_id_type &id = trx_trace_ptr->id;
                    next(read_write::register_fio_domain_address_results{id, output});
                } CATCH_AND_CALL(next);
            }
        });


    } catch (boost::interprocess::bad_alloc &) {
        chain_plugin::handle_db_exhaustion();
    } CATCH_AND_CALL(next);
}

/***
 * set_fio_domain_public - By default all FIO Domains are non-public, meaning only the owner can register FIO Addresses on that domain. Setting them to public allows anyone to register a FIO Address on that domain.
 * @param p Accepts a variant object of from a pushed fio transaction that contains a public key in packed actions
 * @return result, result.transaction_id (chain::transaction_id_type), result.processed (fc::variant)
 */
        void read_write::set_fio_domain_public(const read_write::set_fio_domain_public_params &params,
                                               next_function<read_write::set_fio_domain_public_results> next) {
            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("set_fio_domain_public called");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                }
                EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);

                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.address", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "setdomainpub", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }
                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::set_fio_domain_public_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }


        void read_write::register_fio_domain(const read_write::register_fio_domain_params &params,
                                             next_function<read_write::register_fio_domain_results> next) {
            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("register_fio_domain called");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                } EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.address", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "regdomain", fioio::InvalidAccountOrAction);


                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }
                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::register_fio_domain_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }

/***
* add_pub_address - Registers a public address onto the address container
* @param p Accepts a variant object of from a pushed fio transaction that contains a public key in packed actions
* @return result, result.transaction_id (chain::transaction_id_type), result.processed (fc::variant)
*/
        void read_write::add_pub_address(const read_write::add_pub_address_params &params,
                                         next_function<read_write::add_pub_address_results> next) {
            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("add_pub_address called");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                } EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.address", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "addaddress", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }
                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::add_pub_address_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }

        /***
* remove_pub_address - Removes a public address
* @param p Accepts a variant object of from a pushed fio transaction that contains a public key in packed actions
* @return result, result.transaction_id (chain::transaction_id_type), result.processed (fc::variant)
*/
        void read_write::remove_pub_address(const read_write::remove_pub_address_params &params,
                                         next_function<read_write::remove_pub_address_results> next) {
            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("remove_pub_address called");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                } EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.address", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "remaddress", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }
                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::remove_pub_address_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }


        /***
* remove_all_pub_addresses - Removes all public addresses except the FIO address
* @param p Accepts a variant object of from a pushed fio transaction that contains a public key in packed actions
* @return result, result.transaction_id (chain::transaction_id_type), result.processed (fc::variant)
*/
        void read_write::remove_all_pub_addresses(const read_write::remove_all_pub_addresses_params &params,
                                         next_function<read_write::remove_all_pub_addresses_results> next) {
            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("remove_all_pub_addresses called");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                } EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.address", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "remalladdr", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }
                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::remove_all_pub_addresses_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }


        /***
         * transfer_tokens_pub_key - Transfers FIO tokens from actor to fio pub address
         * @param p Accepts a variant object of from a pushed fio transaction that contains a public key in packed actions
         * @return result, result.transaction_id (chain::transaction_id_type), result.processed (fc::variant)
         */
        void read_write::transfer_tokens_pub_key(const read_write::transfer_tokens_pub_key_params &params,
                                                 next_function<read_write::transfer_tokens_pub_key_results> next) {
            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("transfer_tokens_pub_key called");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                } EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);

                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.token", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "trnsfiopubky", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }
                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::transfer_tokens_pub_key_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }



        //FIP-38 begin
        void read_write::new_fio_chain_account(const read_write::new_fio_chain_account_params &params,
                                                 next_function<read_write::new_fio_chain_account_results> next) {
            try {
                FIO_403_ASSERT(params.size() == 6,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("new_fio_chain_account called");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                } EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);

                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.system", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "newfioacc", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }
                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::new_fio_chain_account_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }
        //FIP-38 end



        /***
         * transfer_locked_tokens - Transfers locked FIO tokens from actor to fio pub address
         * @param p Accepts a variant object of from a pushed fio transaction that contains a public key in packed actions
         * @return result, result.transaction_id (chain::transaction_id_type), result.processed (fc::variant)
         */
        void read_write::transfer_locked_tokens(const read_write::transfer_locked_tokens_params &params,
                                                 next_function<read_write::transfer_locked_tokens_results> next) {
            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                } EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);

                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.token", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "trnsloctoks", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }
                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::transfer_locked_tokens_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }

        /***
 * burn_expired - This enpoint will burn the next 100 expired addresses.
 * @param p Accepts a variant object of from a pushed fio transaction that contains a public key in packed actions
 * @return result, result.transaction_id (chain::transaction_id_type), result.processed (fc::variant)
 */
        void read_write::burn_expired(const read_write::burn_expired_params &params,
                                      next_function<read_write::burn_expired_results> next) {
            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("burn_expired called");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                }
                EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.address", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "burnexpired", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }
                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::burn_expired_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }

        /***
         * compute fees - This enpoint will compute fees based on pending votes..
         * @param p Accepts a variant object of from a pushed fio transaction that contains a public key in packed actions
         * @return result, result.transaction_id (chain::transaction_id_type), result.processed (fc::variant)
         */
        void read_write::compute_fees(const read_write::compute_fees_params &params,
                                      next_function <read_write::compute_fees_results> next) {
            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("compute_fees called");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                }
                EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector <action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.fee", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "computefees", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant <fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }
                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::compute_fees_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });
            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }

        void read_write::burn_fio_address(const read_write::burn_fio_address_params &params,
                                          next_function <read_write::burn_fio_address_results> next) {
            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("burn_fio_address called");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                }
                EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector <action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.address", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "burnaddress", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant <fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }
                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::burn_fio_address_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });

            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }

        void read_write::transfer_fio_domain(const read_write::transfer_fio_domain_params &params,
                                              next_function<read_write::transfer_fio_domain_results> next) {
            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("transfer_fio_domain called");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                } EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.address", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "xferdomain", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }
                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::transfer_fio_domain_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });

            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }

        void read_write::transfer_fio_address(const read_write::transfer_fio_address_params &params,
                                             next_function<read_write::transfer_fio_address_results> next) {
            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("transfer_fio_domain called");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                } EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.address", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "xferaddress", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }
                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::transfer_fio_address_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });

            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }

/***
* unregister_proxy - This enpoint will set the specified fio address account to no longer be a proxy.
* @param p Accepts a variant object of from a pushed fio transaction that contains a public key in packed actions
* @return result, result.transaction_id (chain::transaction_id_type), result.processed (fc::variant)
*/
        void read_write::unregister_proxy(const read_write::unregister_proxy_params &params,
                                          next_function<read_write::unregister_proxy_results> next) {
            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("unregister_proxy called");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                }
                EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);

                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "eosio", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "unregproxy", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }
                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::unregister_proxy_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }


/***
* register_proxy - This enpoint will set the specified fio address account to be a proxy.
* @param p Accepts a variant object of from a pushed fio transaction that contains a public key in packed actions
* @return result, result.transaction_id (chain::transaction_id_type), result.processed (fc::variant)
*/
        void read_write::register_proxy(const read_write::register_proxy_params &params,
                                        next_function<read_write::register_proxy_results> next) {
            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("register_proxy called");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                }
                EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "eosio", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "regproxy", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }
                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::register_proxy_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }

        void read_write::register_producer(const read_write::register_producer_params &params,
                                           next_function<read_write::register_producer_results> next) {
            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("register_producer called");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                }
                EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "eosio", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "regproducer", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }
                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::register_producer_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }

        void read_write::vote_producer(const read_write::vote_producer_params &params,
                                       next_function<read_write::vote_producer_results> next) {
            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("vote_producer called");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                }
                EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);

                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "eosio", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "voteproducer", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }
                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::vote_producer_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }

        void read_write::proxy_vote(const read_write::proxy_vote_params &params,
                                    next_function<read_write::proxy_vote_results> next) {
            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("proxy_vote called");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                }
                EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);

                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "eosio", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "voteproxy", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }
                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::proxy_vote_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }

        void read_write::submit_fee_ratios(const read_write::submit_fee_ratios_params &params,
                                           next_function<read_write::submit_fee_ratios_results> next) {
            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("submit_fee_ratios called");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                }
                EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);

                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.fee", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "setfeevote", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }
                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::submit_fee_ratios_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }


        void read_write::submit_fee_multiplier(const read_write::submit_fee_multiplier_params &params,
                                               next_function<read_write::submit_fee_multiplier_results> next) {
            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("proxy_vote called");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                }
                EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);

                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.fee", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "setfeemult", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }
                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::submit_fee_multiplier_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }

        void read_write::unregister_producer(const read_write::unregister_producer_params &params,
                                             next_function<read_write::unregister_producer_results> next) {
            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("unregister_proxy called");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                }
                EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);

                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "eosio", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "unregprod", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }
                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::unregister_producer_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }

/***
 * renew_domain - This endpoint will renew the specified domain.
 * @param p Accepts a variant object of from a pushed fio transaction that contains a public key in packed actions
 * @return result, result.transaction_id (chain::transaction_id_type), result.processed (fc::variant)
 */
        void read_write::renew_fio_domain(const read_write::renew_fio_domain_params &params,
                                          next_function<read_write::renew_fio_domain_results> next) {
            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("renew_domain called");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                }
                EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);

                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.address", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "renewdomain", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }
                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::renew_fio_domain_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }



        //FIP-39 begin
        /***
 * update_encrypt_key - This endpoint will update the encryption key used for obt content.
 * @param p Accepts a variant object of from a pushed fio transaction that contains a public key in packed actions
 * @return result, result.transaction_id (chain::transaction_id_type), result.processed (fc::variant)
 */
        void read_write::update_encrypt_key(const read_write::update_encrypt_key_params &params,
                                          next_function<read_write::update_encrypt_key_results> next) {
            try {
                FIO_403_ASSERT(params.size() == 5,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("update encrypt key called");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                }
                EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);

                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.address", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "updcryptkey", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }
                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::update_encrypt_key_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }

        //FIP-39 end


        /***
 * renew_address - This endpoint will renew the specified address.
 * @param p Accepts a variant object of from a pushed fio transaction that contains a public key in packed actions
 * @return result, result.transaction_id (chain::transaction_id_type), result.processed (fc::variant)
 */
        void read_write::renew_fio_address(const read_write::renew_fio_address_params &params,
                                           next_function<read_write::renew_fio_address_results> next) {
            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("renew_address called");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                }
                EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);

                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.address", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "renewaddress", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }
                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::renew_fio_address_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }


/***
 * pay_tpid_rewards - This endpoint will pay TPIDs pending rewards payment.
 * @param p Accepts a variant object of from a pushed fio transaction that contains a public key in packed actions
 * @return result, result.transaction_id (chain::transaction_id_type), result.processed (fc::variant)
 */
        void read_write::pay_tpid_rewards(const read_write::pay_tpid_rewards_params &params,
                                          next_function<read_write::pay_tpid_rewards_results> next) {
            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("pay_tpid_rewards called");
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                }
                EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);

                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.treasury", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "tpidclaim", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }
                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::pay_tpid_rewards_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }

        void read_write::submit_bundled_transaction(const read_write::submit_bundled_transaction_params &params,
                                                    next_function<read_write::submit_bundled_transaction_results> next) {

            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("submit_bundled_transaction called");

                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                } EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);

                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.fee", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "bundlevote", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }

                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::submit_bundled_transaction_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });

            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }

        void read_write::claim_bp_rewards(const read_write::claim_bp_rewards_params &params,
                                          next_function<read_write::claim_bp_rewards_results> next) {

            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("claim_bp_rewards called");

                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                } EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);

                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.treasury", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "bpclaim", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }

                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::claim_bp_rewards_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }

        void read_write::add_bundled_transactions(const read_write::add_bundled_transactions_params &params,
                                          next_function<read_write::add_bundled_transactions_results> next) {

            try {
                FIO_403_ASSERT(params.size() == 4,
                               fioio::ErrorTransaction); // variant object contains authorization, account, name, data
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                dlog("add_bundled_transactions called");

                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                } EOS_RETHROW_EXCEPTIONS(chain::fio_invalid_trans_exception, "Invalid transaction")

                transaction trx = pretty_input->get_transaction();
                vector<action> &actions = trx.actions;
                dlog("\n");
                dlog(actions[0].name.to_string());
                FIO_403_ASSERT(trx.total_actions() == 1, fioio::InvalidAccountOrAction);

                FIO_403_ASSERT(actions[0].authorization.size() > 0, fioio::ErrorTransaction);
                FIO_403_ASSERT(actions[0].account.to_string() == "fio.address", fioio::InvalidAccountOrAction);
                FIO_403_ASSERT(actions[0].name.to_string() == "addbundles", fioio::InvalidAccountOrAction);

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }

                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::add_bundled_transactions_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });


            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } CATCH_AND_CALL(next);
        }

        void read_write::push_transaction(const read_write::push_transaction_params &params,
                                          next_function<read_write::push_transaction_results> next) {
            try {
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                } EOS_RETHROW_EXCEPTIONS(chain::packed_transaction_type_exception, "Invalid packed transaction")

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);

                                // Create map of (closest_unnotified_ancestor_action_ordinal, global_sequence) with action trace
                                std::map<std::pair<uint32_t, uint64_t>, fc::mutable_variant_object> act_traces_map;
                                for (const auto &act_trace : output["action_traces"].get_array()) {
                                    if (act_trace["receipt"].is_null() && act_trace["except"].is_null()) continue;
                                    auto closest_unnotified_ancestor_action_ordinal =
                                            act_trace["closest_unnotified_ancestor_action_ordinal"].as<fc::unsigned_int>().value;
                                    auto global_sequence = act_trace["receipt"].is_null() ?
                                                           std::numeric_limits<uint64_t>::max() :
                                                           act_trace["receipt"]["global_sequence"].as<uint64_t>();
                                    act_traces_map.emplace(std::make_pair(closest_unnotified_ancestor_action_ordinal,
                                                                          global_sequence),
                                                           act_trace.get_object());
                                }

                                std::function<vector<fc::variant>(uint32_t)> convert_act_trace_to_tree_struct =
                                        [&](uint32_t closest_unnotified_ancestor_action_ordinal) {
                                            vector<fc::variant> restructured_act_traces;
                                            auto it = act_traces_map.lower_bound(
                                                    std::make_pair(closest_unnotified_ancestor_action_ordinal, 0)
                                            );
                                            for (;
                                                    it != act_traces_map.end() && it->first.first ==
                                                                                  closest_unnotified_ancestor_action_ordinal; ++it) {
                                                auto &act_trace_mvo = it->second;

                                                auto action_ordinal = act_trace_mvo["action_ordinal"].as<fc::unsigned_int>().value;
                                                act_trace_mvo["inline_traces"] = convert_act_trace_to_tree_struct(
                                                        action_ordinal);
                                                if (act_trace_mvo["receipt"].is_null()) {
                                                    act_trace_mvo["receipt"] = fc::mutable_variant_object()
                                                            ("abi_sequence", 0)
                                                            ("act_digest", digest_type::hash(
                                                                    trx_trace_ptr->action_traces[action_ordinal -
                                                                                                 1].act))
                                                            ("auth_sequence", flat_map<account_name, uint64_t>())
                                                            ("code_sequence", 0)
                                                            ("global_sequence", 0)
                                                            ("receiver", act_trace_mvo["receiver"])
                                                            ("recv_sequence", 0);
                                                }
                                                restructured_act_traces.push_back(std::move(act_trace_mvo));
                                            }
                                            return restructured_act_traces;
                                        };

                                fc::mutable_variant_object output_mvo(output);
                                output_mvo["action_traces"] = convert_act_trace_to_tree_struct(0);

                                output = output_mvo;
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }

                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::push_transaction_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });
            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } catch (const std::bad_alloc &) {
                chain_plugin::handle_bad_alloc();
            } CATCH_AND_CALL(next);
        }

        static void
        push_recurse(read_write *rw, int index, const std::shared_ptr<read_write::push_transactions_params> &params,
                     const std::shared_ptr<read_write::push_transactions_results> &results,
                     const next_function<read_write::push_transactions_results> &next) {
            auto wrapped_next = [=](
                    const fc::static_variant<fc::exception_ptr, read_write::push_transaction_results> &result) {
                if (result.contains<fc::exception_ptr>()) {
                    const auto &e = result.get<fc::exception_ptr>();
                    results->emplace_back(read_write::push_transaction_results{transaction_id_type(),
                                                                               fc::mutable_variant_object("error",
                                                                                                          e->to_detail_string())});
                } else {
                    const auto &r = result.get<read_write::push_transaction_results>();
                    results->emplace_back(r);
                }

                size_t next_index = index + 1;
                if (next_index < params->size()) {
                    push_recurse(rw, next_index, params, results, next);
                } else {
                    next(*results);
                }
            };

            rw->push_transaction(params->at(index), wrapped_next);
        }

        void read_write::push_transactions(const read_write::push_transactions_params &params,
                                           next_function<read_write::push_transactions_results> next) {
            try {
                EOS_ASSERT(params.size() <= 1000, too_many_tx_at_once, "Attempt to push too many transactions at once");
                auto params_copy = std::make_shared<read_write::push_transactions_params>(params.begin(), params.end());
                auto result = std::make_shared<read_write::push_transactions_results>();
                result->reserve(params.size());

                push_recurse(this, 0, params_copy, result, next);
            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } catch (const std::bad_alloc &) {
                chain_plugin::handle_bad_alloc();
            } CATCH_AND_CALL(next);
        }

        void read_write::send_transaction(const read_write::send_transaction_params &params,
                                          next_function<read_write::send_transaction_results> next) {

            try {
                auto pretty_input = std::make_shared<packed_transaction>();
                auto resolver = make_resolver(this, abi_serializer_max_time);
                transaction_metadata_ptr ptrx;
                try {
                    abi_serializer::from_variant(params, *pretty_input, resolver, abi_serializer_max_time);
                    ptrx = std::make_shared<transaction_metadata>(pretty_input);
                } EOS_RETHROW_EXCEPTIONS(chain::packed_transaction_type_exception, "Invalid packed transaction")

                app().get_method<incoming::methods::transaction_async>()(ptrx, true, [this, next](
                        const fc::static_variant<fc::exception_ptr, transaction_trace_ptr> &result) -> void {
                    if (result.contains<fc::exception_ptr>()) {
                        next(result.get<fc::exception_ptr>());
                    } else {
                        auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                        try {
                            fc::variant output;
                            try {
                                output = db.to_variant_with_abi(*trx_trace_ptr, abi_serializer_max_time);
                            } catch (chain::abi_exception &) {
                                output = *trx_trace_ptr;
                            }

                            const chain::transaction_id_type &id = trx_trace_ptr->id;
                            next(read_write::send_transaction_results{id, output});
                        } CATCH_AND_CALL(next);
                    }
                });
            } catch (boost::interprocess::bad_alloc &) {
                chain_plugin::handle_db_exhaustion();
            } catch (const std::bad_alloc &) {
                chain_plugin::handle_bad_alloc();
            } CATCH_AND_CALL(next);
        }

        read_only::get_abi_results read_only::get_abi(const get_abi_params &params) const {
            get_abi_results result;
            result.account_name = params.account_name;
            const auto &d = db.db();
            const auto &accnt = d.get<account_object, by_name>(params.account_name);

            abi_def abi;
            if (abi_serializer::to_abi(accnt.abi, abi)) {
                result.abi = std::move(abi);
            }

            return result;
        }

        read_only::get_code_results read_only::get_code(const get_code_params &params) const {
            get_code_results result;
            result.account_name = params.account_name;
            const auto &d = db.db();
            const auto &accnt_obj = d.get<account_object, by_name>(params.account_name);
            const auto &accnt_metadata_obj = d.get<account_metadata_object, by_name>(params.account_name);

            EOS_ASSERT(params.code_as_wasm, unsupported_feature, "Returning WAST from get_code is no longer supported");

            if (accnt_metadata_obj.code_hash != digest_type()) {
                const auto &code_obj = d.get<code_object, by_code_hash>(accnt_metadata_obj.code_hash);
                result.wasm = string(code_obj.code.begin(), code_obj.code.end());
                result.code_hash = code_obj.code_hash;
            }

            abi_def abi;
            if (abi_serializer::to_abi(accnt_obj.abi, abi)) {
                result.abi = std::move(abi);
            }

            return result;
        }

        read_only::get_code_hash_results read_only::get_code_hash(const get_code_hash_params &params) const {
            get_code_hash_results result;
            result.account_name = params.account_name;
            const auto &d = db.db();
            const auto &accnt = d.get<account_metadata_object, by_name>(params.account_name);

            if (accnt.code_hash != digest_type())
                result.code_hash = accnt.code_hash;

            return result;
        }

        read_only::get_raw_code_and_abi_results
        read_only::get_raw_code_and_abi(const get_raw_code_and_abi_params &params) const {
            get_raw_code_and_abi_results result;
            result.account_name = params.account_name;

            const auto &d = db.db();
            const auto &accnt_obj = d.get<account_object, by_name>(params.account_name);
            const auto &accnt_metadata_obj = d.get<account_metadata_object, by_name>(params.account_name);
            if (accnt_metadata_obj.code_hash != digest_type()) {
                const auto &code_obj = d.get<code_object, by_code_hash>(accnt_metadata_obj.code_hash);
                result.wasm = blob{{code_obj.code.begin(), code_obj.code.end()}};
            }
            result.abi = blob{{accnt_obj.abi.begin(), accnt_obj.abi.end()}};

            return result;
        }

        read_only::get_raw_abi_results read_only::get_raw_abi(const get_raw_abi_params &params) const {
            get_raw_abi_results result;
            result.account_name = params.account_name;

            const auto &d = db.db();
            const auto &accnt_obj = d.get<account_object, by_name>(params.account_name);
            const auto &accnt_metadata_obj = d.get<account_metadata_object, by_name>(params.account_name);
            result.abi_hash = fc::sha256::hash(accnt_obj.abi.data(), accnt_obj.abi.size());
            if (accnt_metadata_obj.code_hash != digest_type())
                result.code_hash = accnt_metadata_obj.code_hash;
            if (!params.abi_hash || *params.abi_hash != result.abi_hash)
                result.abi = blob{{accnt_obj.abi.begin(), accnt_obj.abi.end()}};

            return result;
        }

        read_only::get_account_results read_only::get_account(const get_account_params &params) const {
            get_account_results result;
            result.account_name = params.account_name;

            const auto &d = db.db();
            const auto &rm = db.get_resource_limits_manager();

            result.head_block_num = db.head_block_num();
            result.head_block_time = db.head_block_time();

            rm.get_account_limits(result.account_name, result.ram_quota, result.net_weight, result.cpu_weight);

            const auto &accnt_obj = db.get_account(result.account_name);
            const auto &accnt_metadata_obj = db.db().get<account_metadata_object, by_name>(result.account_name);

            result.privileged = accnt_metadata_obj.is_privileged();
            result.last_code_update = accnt_metadata_obj.last_code_update;
            result.created = accnt_obj.creation_date;

            uint32_t greylist_limit = db.is_resource_greylisted(result.account_name) ? 1
                                                                                     : config::maximum_elastic_resource_multiplier;
            result.net_limit = rm.get_account_net_limit_ex(result.account_name, greylist_limit).first;
            result.cpu_limit = rm.get_account_cpu_limit_ex(result.account_name, greylist_limit).first;
            result.ram_usage = rm.get_account_ram_usage(result.account_name);

            const auto &permissions = d.get_index<permission_index, by_owner>();
            auto perm = permissions.lower_bound(boost::make_tuple(params.account_name));
            while (perm != permissions.end() && perm->owner == params.account_name) {
                /// TODO: lookup perm->parent name
                name parent;

                // Don't lookup parent if null
                if (perm->parent._id) {
                    const auto *p = d.find<permission_object, by_id>(perm->parent);
                    if (p) {
                        EOS_ASSERT(perm->owner == p->owner, invalid_parent_permission, "Invalid parent permission");
                        parent = p->name;
                    }
                }

                result.permissions.push_back(permission{perm->name, parent, perm->auth.to_authority()});
                ++perm;
            }

            const auto &code_account = db.db().get<account_object, by_name>(config::system_account_name);

            abi_def abi;
            if (abi_serializer::to_abi(code_account.abi, abi)) {
                abi_serializer abis(abi, abi_serializer_max_time);

                const auto token_code = N(fio.token);

                auto core_symbol = extract_core_symbol();

                if (params.expected_core_symbol.valid())
                    core_symbol = *(params.expected_core_symbol);

                const auto *t_id = d.find<chain::table_id_object, chain::by_code_scope_table>(
                        boost::make_tuple(token_code, params.account_name, N(accounts)));
                if (t_id != nullptr) {
                    const auto &idx = d.get_index<key_value_index, by_scope_primary>();
                    auto it = idx.find(boost::make_tuple(t_id->id, core_symbol.to_symbol_code()));
                    if (it != idx.end() && it->value.size() >= sizeof(asset)) {
                        asset bal;
                        fc::datastream<const char *> ds(it->value.data(), it->value.size());
                        fc::raw::unpack(ds, bal);

                        if (bal.get_symbol().valid() && bal.get_symbol() == core_symbol) {
                            result.core_liquid_balance = bal;
                        }
                    }
                }

                t_id = d.find<chain::table_id_object, chain::by_code_scope_table>(
                        boost::make_tuple(config::system_account_name, params.account_name, N(userres)));
                if (t_id != nullptr) {
                    const auto &idx = d.get_index<key_value_index, by_scope_primary>();
                    auto it = idx.find(boost::make_tuple(t_id->id, params.account_name));
                    if (it != idx.end()) {
                        vector<char> data;
                        copy_inline_row(*it, data);
                        result.total_resources = abis.binary_to_variant("user_resources", data, abi_serializer_max_time,
                                                                        shorten_abi_errors);
                    }
                }

                t_id = d.find<chain::table_id_object, chain::by_code_scope_table>(
                        boost::make_tuple(config::system_account_name, params.account_name, N(delband)));
                if (t_id != nullptr) {
                    const auto &idx = d.get_index<key_value_index, by_scope_primary>();
                    auto it = idx.find(boost::make_tuple(t_id->id, params.account_name));
                    if (it != idx.end()) {
                        vector<char> data;
                        copy_inline_row(*it, data);
                        result.self_delegated_bandwidth = abis.binary_to_variant("delegated_bandwidth", data,
                                                                                 abi_serializer_max_time,
                                                                                 shorten_abi_errors);
                    }
                }

                t_id = d.find<chain::table_id_object, chain::by_code_scope_table>(
                        boost::make_tuple(config::system_account_name, params.account_name, N(refunds)));
                if (t_id != nullptr) {
                    const auto &idx = d.get_index<key_value_index, by_scope_primary>();
                    auto it = idx.find(boost::make_tuple(t_id->id, params.account_name));
                    if (it != idx.end()) {
                        vector<char> data;
                        copy_inline_row(*it, data);
                        result.refund_request = abis.binary_to_variant("refund_request", data, abi_serializer_max_time,
                                                                       shorten_abi_errors);
                    }
                }

                const abi_def system_abi = eosio::chain_apis::get_abi(db,"eosio");
                get_table_rows_params voter_table = get_table_rows_params{
                        .json        = true,
                        .code        = "eosio",
                        .scope       = "eosio",
                        .table       = "voters",
                        .lower_bound = boost::lexical_cast<string>(params.account_name.value),
                        .upper_bound = boost::lexical_cast<string>(params.account_name.value),
                        .key_type       = "i64",
                        .index_position = "3"
                };

                get_table_rows_result voter_result = get_table_rows_by_seckey<index64_index, uint64_t>(
                        voter_table, system_abi, [](uint64_t v) -> uint64_t {
                            return v;
                 });
                  if (!voter_result.rows.empty()) {
                    result.voter_info = voter_result.rows[0];
                  }
            }
            return result;
        }

        static variant action_abi_to_variant(const abi_def &abi, type_name action_type) {
            variant v;
            auto it = std::find_if(abi.structs.begin(), abi.structs.end(),
                                   [&](auto &x) { return x.name == action_type; });
            if (it != abi.structs.end())
                to_variant(it->fields, v);
            return v;
        };

        read_only::abi_json_to_bin_result
        read_only::abi_json_to_bin(const read_only::abi_json_to_bin_params &params) const try {
            abi_json_to_bin_result result;
            const auto code_account = db.db().find<account_object, by_name>(params.code);
            EOS_ASSERT(code_account != nullptr, contract_query_exception, "Contract can't be found ${contract}",
                       ("contract", params.code));

            abi_def abi;
            if (abi_serializer::to_abi(code_account->abi, abi)) {
                abi_serializer abis(abi, abi_serializer_max_time);
                auto action_type = abis.get_action_type(params.action);
                EOS_ASSERT(!action_type.empty(), action_validate_exception,
                           "Unknown action ${action} in contract ${contract}",
                           ("action", params.action)("contract", params.code));
                try {
                    result.binargs = abis.variant_to_binary(action_type, params.args, abi_serializer_max_time,
                                                            shorten_abi_errors);
                } EOS_RETHROW_EXCEPTIONS(chain::invalid_action_args_exception,
                                         "'${args}' is invalid args for action '${action}' code '${code}'. expected '${proto}'",
                                         ("args", params.args)("action", params.action)("code", params.code)("proto",
                                                                                                             action_abi_to_variant(
                                                                                                                     abi,
                                                                                                                     action_type)))
            } else {
                EOS_ASSERT(false, abi_not_found_exception, "No ABI found for ${contract}", ("contract", params.code));
            }
            return result;
        } FC_RETHROW_EXCEPTIONS(warn, "code: ${code}, action: ${action}, args: ${args}",
                                ("code", params.code)("action", params.action)("args", params.args))

        read_only::abi_bin_to_json_result
        read_only::abi_bin_to_json(const read_only::abi_bin_to_json_params &params) const {
            abi_bin_to_json_result result;
            const auto &code_account = db.db().get<account_object, by_name>(params.code);
            abi_def abi;
            if (abi_serializer::to_abi(code_account.abi, abi)) {
                abi_serializer abis(abi, abi_serializer_max_time);
                result.args = abis.binary_to_variant(abis.get_action_type(params.action), params.binargs,
                                                     abi_serializer_max_time, shorten_abi_errors);
            } else {
                EOS_ASSERT(false, abi_not_found_exception, "No ABI found for ${contract}", ("contract", params.code));
            }
            return result;
        }

        read_only::serialize_json_result
        read_only::serialize_json(const read_only::serialize_json_params &params) const try {
            serialize_json_result result;

            const int32_t HF1_BLOCK_TIME = 1600876800; //Wed Sep 23 16:00:00 UTC 2020
            string actionname;

            action_name nm = params.action;
            if ( db.head_block_time().sec_since_epoch() > HF1_BLOCK_TIME) {
                const fioaction_object *fioaction_item = nullptr;
                fioaction_item = db.db().find<fioaction_object, by_actionname>(nm);
                EOS_ASSERT(fioaction_item != nullptr, contract_query_exception, "Action can't be found ${contract}",
                           ("contract", params.action.to_string()));
                actionname = fioaction_item->contractname;
            }else{
                actionname = fioio::map_to_contract(params.action.to_string());
            }

            name code = ::eosio::string_to_name(actionname.c_str());

            const auto code_account = db.db().find<account_object, by_name>(code);
            EOS_ASSERT(code_account != nullptr, contract_query_exception, "Contract can't be found ${contract}",
                       ("contract", code));

            abi_def abi;
            if (abi_serializer::to_abi(code_account->abi, abi)) {
                abi_serializer abis(abi, abi_serializer_max_time);
                auto action_type = abis.get_action_type(params.action);
                EOS_ASSERT(!action_type.empty(), action_validate_exception,
                           "Unknown action ${action} in contract ${contract}",
                           ("action", params.action)("contract", code));
                try {
                    result.serialized_json = abis.variant_to_binary(action_type, params.json, abi_serializer_max_time,
                                                                    shorten_abi_errors);
                } EOS_RETHROW_EXCEPTIONS(chain::invalid_action_args_exception,
                                         "'${args}' is invalid args for action '${action}' code '${code}'. expected '${proto}'",
                                         ("args", params.json)("action", params.action)("code", code)("proto",
                                                                                                      action_abi_to_variant(
                                                                                                              abi,
                                                                                                              action_type)))
            } else {
                EOS_ASSERT(false, abi_not_found_exception, "No ABI found for ${contract}", ("contract", code));
            }
            return result;
        } FC_RETHROW_EXCEPTIONS(warn, "action: ${action}, args: ${args}",
                                ("action", params.action)("args", params.json))

        read_only::get_required_keys_result read_only::get_required_keys(const get_required_keys_params &params) const {
            transaction pretty_input;
            auto resolver = make_resolver(this, abi_serializer_max_time);
            try {
                abi_serializer::from_variant(params.transaction, pretty_input, resolver, abi_serializer_max_time);
            } EOS_RETHROW_EXCEPTIONS(chain::transaction_type_exception, "Invalid transaction")

            auto required_keys_set = db.get_authorization_manager().get_required_keys(pretty_input,
                                                                                      params.available_keys,
                                                                                      fc::seconds(
                                                                                              pretty_input.delay_sec));
            get_required_keys_result result;
            result.required_keys = required_keys_set;
            return result;
        }

        read_only::get_transaction_id_result
        read_only::get_transaction_id(const read_only::get_transaction_id_params &params) const {
            return params.id();
        }

        namespace detail {
            struct ram_market_exchange_state_t {
                asset ignore1;
                asset ignore2;
                double ignore3;
                asset core_symbol;
                double ignore4;
            };
        }

        chain::symbol read_only::extract_core_symbol() const {
            symbol core_symbol(0);

            // The following code makes assumptions about the contract deployed on eosio account (i.e. the system contract) and how it stores its data.
            const auto &d = db.db();
            const auto *t_id = d.find<chain::table_id_object, chain::by_code_scope_table>(
                    boost::make_tuple(N(eosio), N(eosio), N(rammarket)));
            if (t_id != nullptr) {
                const auto &idx = d.get_index<key_value_index, by_scope_primary>();
                auto it = idx.find(boost::make_tuple(t_id->id, eosio::chain::string_to_symbol_c(4, "RAMCORE")));
                if (it != idx.end()) {
                    detail::ram_market_exchange_state_t ram_market_exchange_state;

                    fc::datastream<const char *> ds(it->value.data(), it->value.size());

                    try {
                        fc::raw::unpack(ds, ram_market_exchange_state);
                    } catch (...) {
                        return core_symbol;
                    }

                    if (ram_market_exchange_state.core_symbol.get_symbol().valid()) {
                        core_symbol = ram_market_exchange_state.core_symbol.get_symbol();
                    }
                }
            }

            return core_symbol;
        }

    } // namespace chain_apis
} // namespace eosio

FC_REFLECT(eosio::chain_apis::detail::ram_market_exchange_state_t, (ignore1)(ignore2)(ignore3)(core_symbol)(ignore4))
