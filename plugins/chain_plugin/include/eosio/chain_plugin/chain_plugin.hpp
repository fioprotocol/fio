/**
 *  @file
 *  @copyright defined in fio/LICENSE
 */
#pragma once

#include <appbase/application.hpp>
#include <eosio/chain/asset.hpp>
#include <eosio/chain/authority.hpp>
#include <eosio/chain/account_object.hpp>
#include <eosio/chain/fioaction_object.hpp>
#include <eosio/chain/block.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/contract_table_objects.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/plugin_interface.hpp>
#include <eosio/chain/types.hpp>

#include <boost/container/flat_set.hpp>
#include <boost/multiprecision/cpp_int.hpp>

#include <fc/static_variant.hpp>

namespace fc { class variant; }

namespace eosio {
    using chain::controller;
    using std::unique_ptr;
    using std::pair;
    using namespace appbase;
    using chain::name;
    using chain::uint128_t;
    using chain::public_key_type;
    using chain::transaction;
    using chain::transaction_id_type;
    using fc::optional;
    using boost::container::flat_set;
    using chain::asset;
    using chain::symbol;
    using chain::authority;
    using chain::account_name;
    using chain::action_name;
    using chain::abi_def;
    using chain::abi_serializer;

    namespace chain_apis {
        struct empty {
        };

        //FIP-46 begin
        /*
         * These 2 constants are added to provide those who host an API node the ability to modify these values
         * if there is ever a need to provide API nodes increased time for processing of table information within read only getters.
         * In FIO we provide 1/10 second of processing time to an API node for the processing of secondary index information,
         * this provides API nodes the ability to return reliable results when tables contain up to tens of thousdands of rows per secondary index.
         * EOSIO historically used a single constant WALKTIME for both primary key and secondary key reads, which were both set to 100 milliseconds, this
         * limited the number of rows per secondary index to something well under 5k rows per secondary index for reliable results in
         * a read only getter. SEE EOS ISSUE #3965 https://github.com/EOSIO/eos/issues/3965
         */
        const static int SECONDARY_INDEX_MAX_READ_TIME_MICROSECONDS = 1000 * 100; // 1/10 second read time permitted on secondary indices
        const static int PRIMARY_INDEX_MAX_READ_TIME_MICROSECONDS = 1000 * 10;  //100 milliseconds read time permitted for pirmary indices.
        //FIP-46 end

        struct permission {
            name perm_name;
            name parent;
            authority required_auth;
        };

        struct fiodomain_record {

            string fio_domain;
            string expiration;
            uint8_t is_public;
        };

        struct fioaddress_record {
            string fio_address;
            string expiration;
            uint64_t remaining_bundled_tx = 0;
        };

        struct oraclefee_record {
            string fee_name;
            uint64_t fee_amount;
        };

        struct request_record {
            uint64_t fio_request_id;     // one up index starting at 0
            string payer_fio_address;   // sender FIO address e.g. john.xyz
            string payee_fio_address;     // receiver FIO address e.g. jane.xyz
            string payer_fio_public_key;
            string payee_fio_public_key;
            string content;             // this is encrypted content
            string time_stamp;    // FIO blockchain request received timestamp
        };

        struct listing_record {
            uint64_t id;
            string commission_fee;
            string date_listed;
            string date_updated;
            string domain;
            string owner;
            uint64_t sale_price;
            uint64_t status;
        };

        struct obt_records {
            string payer_fio_address;   // sender FIO address e.g. john.xyz
            string payee_fio_address;     // receiver FIO address e.g. jane.xyz
            string payer_fio_public_key;
            string payee_fio_public_key;
            string content;             // this is encrypted content
            uint64_t fio_request_id;     // one up index starting at 0
            string time_stamp;    // FIO blockchain request received timestamp
            string status;          //the status of the request.
        };


        struct request_status_record {
            uint64_t fio_request_id;       // one up index starting at 0
            string payer_fio_address;   // sender FIO address e.g. john.xyz
            string payee_fio_address;     // receiver FIO address e.g. jane.xyz
            string payer_fio_public_key;
            string payee_fio_public_key;
            string content;      // this is encrypted content
            string time_stamp;        // FIO blockchain request received timestamp
            string status;          //the status of the request.
        };

        struct action_record {
            string action;
            string contract;
            string block_timestamp;
        };

        struct whitelist_info {
            string fio_public_key_hash;
            string content;
        };

        struct nft_info {
            optional<string> fio_address;
            string chain_code;
            string contract_address;
            string token_id;
            string url;
            string hash;
            string metadata;
        };

        template<typename>
        struct resolver_factory;

// see specializations for uint64_t and double in source file
        template<typename Type>
        Type convert_to_type(const string &str, const string &desc) {
            try {
                return fc::variant(str).as<Type>();
            } FC_RETHROW_EXCEPTIONS(warn, "Could not convert ${desc} string '${str}' to key type.",
                                    ("desc", desc)("str", str))
        }

        template<>
        uint64_t convert_to_type(const string &str, const string &desc);

        template<>
        double convert_to_type(const string &str, const string &desc);

        class read_only {
            const controller &db;
            const fc::microseconds abi_serializer_max_time;
            bool shorten_abi_errors = true;

        public:
            static const string KEYi64;

            read_only(const controller &db, const fc::microseconds &abi_serializer_max_time)
                    : db(db), abi_serializer_max_time(abi_serializer_max_time) {}

            void validate() const {}

            void set_shorten_abi_errors(bool f) { shorten_abi_errors = f; }

            using get_info_params = empty;

            struct get_info_results {
                string server_version;
                chain::chain_id_type chain_id;
                uint32_t head_block_num = 0;
                uint32_t last_irreversible_block_num = 0;
                chain::block_id_type last_irreversible_block_id;
                chain::block_id_type head_block_id;
                fc::time_point head_block_time;
                account_name head_block_producer;

                uint64_t virtual_block_cpu_limit = 0;
                uint64_t virtual_block_net_limit = 0;

                uint64_t block_cpu_limit = 0;
                uint64_t block_net_limit = 0;
                //string                  recent_slots;
                //double                  participation_rate = 0;
                optional<string> server_version_string;
                optional<uint32_t> fork_db_head_block_num;
                optional<chain::block_id_type> fork_db_head_block_id;
            };

            get_info_results get_info(const get_info_params &) const;

            struct get_activated_protocol_features_params {
                optional<uint32_t> lower_bound;
                optional<uint32_t> upper_bound;
                uint32_t limit = 10;
                bool search_by_block_num = false;
                bool reverse = false;
            };

            struct get_activated_protocol_features_results {
                fc::variants activated_protocol_features;
                optional<uint32_t> more;
            };

            get_activated_protocol_features_results
            get_activated_protocol_features(const get_activated_protocol_features_params &params) const;

            struct producer_info {
                name producer_name;
            };

            using account_resource_limit = chain::resource_limits::account_resource_limit;

            struct get_account_results {
                name account_name;
                uint32_t head_block_num = 0;
                fc::time_point head_block_time;

                bool privileged = false;
                fc::time_point last_code_update;
                fc::time_point created;

                optional<asset> core_liquid_balance;

                int64_t ram_quota = 0;
                int64_t net_weight = 0;
                int64_t cpu_weight = 0;

                account_resource_limit net_limit;
                account_resource_limit cpu_limit;
                int64_t ram_usage = 0;

                vector<permission> permissions;

                fc::variant total_resources;
                fc::variant self_delegated_bandwidth;
                fc::variant refund_request;
                fc::variant voter_info;
            };

            struct get_account_params {
                name account_name;
                optional<symbol> expected_core_symbol;
            };

            get_account_results get_account(const get_account_params &params) const;


            struct get_code_results {
                name account_name;
                string wast;
                string wasm;
                fc::sha256 code_hash;
                optional<abi_def> abi;
            };

            struct get_code_params {
                name account_name;
                bool code_as_wasm = false;
            };

            struct get_code_hash_results {
                name account_name;
                fc::sha256 code_hash;
            };

            struct get_code_hash_params {
                name account_name;
            };

            struct get_abi_results {
                name account_name;
                optional<abi_def> abi;
            };

            struct get_abi_params {
                name account_name;
            };

            struct get_raw_code_and_abi_results {
                name account_name;
                chain::blob wasm;
                chain::blob abi;
            };

            struct get_raw_code_and_abi_params {
                name account_name;
            };

            struct get_raw_abi_params {
                name account_name;
                optional<fc::sha256> abi_hash;
            };

            struct get_raw_abi_results {
                name account_name;
                fc::sha256 code_hash;
                fc::sha256 abi_hash;
                optional<chain::blob> abi;
            };


            get_code_results get_code(const get_code_params &params) const;

            get_code_hash_results get_code_hash(const get_code_hash_params &params) const;

            get_abi_results get_abi(const get_abi_params &params) const;

            get_raw_code_and_abi_results get_raw_code_and_abi(const get_raw_code_and_abi_params &params) const;

            get_raw_abi_results get_raw_abi(const get_raw_abi_params &params) const;


            struct abi_json_to_bin_params {
                name code;
                name action;
                fc::variant args;
            };
            struct abi_json_to_bin_result {
                vector<char> binargs;
            };

            abi_json_to_bin_result abi_json_to_bin(const abi_json_to_bin_params &params) const;


            struct abi_bin_to_json_params {
                name code;
                name action;
                vector<char> binargs;
            };
            struct abi_bin_to_json_result {
                fc::variant args;
            };

            abi_bin_to_json_result abi_bin_to_json(const abi_bin_to_json_params &params) const;


            struct serialize_json_params {

                name action;
                fc::variant json;
            };
            struct serialize_json_result {
                vector<char> serialized_json;
            };

            serialize_json_result serialize_json(const serialize_json_params &params) const;


            struct get_required_keys_params {
                fc::variant transaction;
                flat_set<public_key_type> available_keys;
            };
            struct get_required_keys_result {
                flat_set<public_key_type> required_keys;
            };

            get_required_keys_result get_required_keys(const get_required_keys_params &params) const;

            using get_transaction_id_params = transaction;
            using get_transaction_id_result = transaction_id_type;

            get_transaction_id_result get_transaction_id(const get_transaction_id_params &params) const;

            struct get_block_params {
                string block_num_or_id;
            };

            fc::variant get_block(const get_block_params &params) const;

            struct get_block_header_state_params {
                string block_num_or_id;
            };

            fc::variant get_block_header_state(const get_block_header_state_params &params) const;

            struct get_table_rows_params {
                bool json = false;
                name code;
                string scope;
                name table;
                string table_key;
                string lower_bound;
                string upper_bound;
                uint32_t limit = 1000000000;
                string key_type;  // type of key specified by index_position
                string index_position; // 1 - primary (first), 2 - secondary index (in order defined by multi_index), 3 - third index, etc
                string encode_type{"dec"}; //dec, hex , default=dec
                optional<bool> reverse;
                optional<bool> show_payer; // show RAM pyer
            };

            struct get_table_rows_result {
                vector<fc::variant> rows; ///< one row per item, either encoded as hex String or JSON object
                bool more = false; ///< true if last element in data is not the end and sizeof data() < limit
            };

            get_table_rows_result get_table_rows(const get_table_rows_params &params) const;


            struct get_table_by_scope_params {
                name code; // mandatory
                name table = 0; // optional, act as filter
                string lower_bound; // lower bound of scope, optional
                string upper_bound; // upper bound of scope, optional
                uint32_t limit = 10;
                optional<bool> reverse;
            };
            struct get_table_by_scope_result_row {
                name code;
                name scope;
                name table;
                name payer;
                uint32_t count;
            };
            struct get_table_by_scope_result {
                vector<get_table_by_scope_result_row> rows;
                string more; ///< fill lower_bound with this value to fetch more rows
            };

            get_table_by_scope_result get_table_by_scope(const get_table_by_scope_params &params) const;

            struct get_currency_balance_params {
                name code;
                name account;
                optional<string> symbol;
            };

            vector<asset> get_currency_balance(const get_currency_balance_params &params) const;

            struct get_currency_stats_params {
                name code;
                string symbol;
            };


            struct get_currency_stats_result {
                asset supply;
                asset max_supply;
                account_name issuer;
            };

            fc::variant get_currency_stats(const get_currency_stats_params &params) const;

            struct get_producers_params {
                bool json = true;
                string lower_bound;
                uint32_t limit = 50;
            };

            struct get_producers_result {
                vector<fc::variant> producers; ///< one row per item, either encoded as hex string or JSON object
                double total_producer_vote_weight;
                string more; ///< fill lower_bound with this value to fetch more rows
            };

            get_producers_result get_producers(const get_producers_params &params) const;

            struct get_producer_schedule_params {
            };

            struct get_producer_schedule_result {
                fc::variant active;
                fc::variant pending;
                fc::variant proposed;
            };

            get_producer_schedule_result get_producer_schedule(const get_producer_schedule_params &params) const;

            struct get_scheduled_transactions_params {
                bool json = false;
                string lower_bound;  /// timestamp OR transaction ID
                uint32_t limit = 50;
            };

            struct get_scheduled_transactions_result {
                fc::variants transactions;
                string more; ///< fill lower_bound with this to fetch next set of transactions
            };

            get_scheduled_transactions_result
            get_scheduled_transactions(const get_scheduled_transactions_params &params) const;

            ////////////////
            // FIO ESCROW //
            //begin get fio escrow listings by status
            struct get_escrow_listings_params {
                int32_t status;         // status = 1: on sale, status = 2: Sold, status = 3; Cancelled
                string actor;
                int32_t offset = 0;
                int32_t limit = 1000;
            };

            struct get_escrow_listings_result {
                vector <listing_record> listings;
                uint32_t more;
                optional<bool> time_limit_exceeded_error;
            };

            get_escrow_listings_result
            get_escrow_listings(const get_escrow_listings_params &params) const;

            //end get fio escrow listings by status

            ////////////////
            // FIO COMMON //
            void GetFIOAccount(name account, get_table_rows_result &account_result) const;

            //begin get pending fio requests
            struct get_pending_fio_requests_params {
                string fio_public_key;  // FIO public address to find requests for..
                int32_t offset = 0;
                int32_t limit = 1000;
            };

            struct get_pending_fio_requests_result {
                vector <request_record> requests;
                uint32_t more;
                optional<bool> time_limit_exceeded_error;
            };

            get_pending_fio_requests_result
            get_pending_fio_requests(const get_pending_fio_requests_params &params) const;

            //end get pending fio requests

            //begin get received fio requests
            struct get_received_fio_requests_params {
                string fio_public_key;  // FIO public address to find requests for..
                int32_t offset = 0;
                int32_t limit = 1000;
            };

            struct get_received_fio_requests_result {
                vector <request_status_record> requests;
                uint32_t more;
                optional<bool> time_limit_exceeded_error;
            };

            get_received_fio_requests_result
            get_received_fio_requests(const get_received_fio_requests_params &params) const;

            //end get pending fio requests


            //begin get actions fio requests
            struct get_actions_params {
                int32_t offset = 0;
                int32_t limit = 0;
            };

            struct get_actions_result {
                vector <action_record> actions;
                uint32_t more;
            };

            get_actions_result get_actions(const get_actions_params &params) const;
            //end get actions

            //begin get cancelled fio requests
            struct get_cancelled_fio_requests_params {
                string fio_public_key;  // FIO public address to find requests for..
                int32_t offset = 0;
                int32_t limit = 1000;
            };

            struct get_cancelled_fio_requests_result {
                vector <request_status_record> requests;
                uint32_t more;
                optional<bool> time_limit_exceeded_error;
            };

            get_cancelled_fio_requests_result
            get_cancelled_fio_requests(const get_cancelled_fio_requests_params &params) const;

            //end get pending fio requests

            //begin get sent fio requests
            struct get_sent_fio_requests_params {
                string fio_public_key;  // FIO public address to find requests for..
                int32_t offset = 0;
                int32_t limit = 1000;
            };

            struct get_sent_fio_requests_result {
                vector<request_status_record> requests;
                uint32_t more;
                optional<bool> time_limit_exceeded_error;
            };

            get_sent_fio_requests_result
            get_sent_fio_requests(const get_sent_fio_requests_params &params) const;
            //end get sent fio requests

            //begin get obt data
            struct get_obt_data_params {
                string fio_public_key;  // FIO public address to find requests for..
                int32_t offset = 0;
                int32_t limit = 1000;
            };

            struct get_obt_data_result {
                vector<obt_records> obt_data_records;
                uint32_t more;
                optional<bool> time_limit_exceeded_error;
            };

            get_obt_data_result
            get_obt_data(const get_obt_data_params &params) const;
            //end get obt data

            // get_nfts_fio_address
            struct get_nfts_fio_address_params {
                string fio_address;  // FIO public address to find requests for..
                int32_t offset = 0;
                int32_t limit = 1000;
            };

            struct get_nfts_fio_address_result {
                vector<nft_info> nfts;
                uint32_t more;
                optional<bool> time_limit_exceeded_error;
            };

            get_nfts_fio_address_result
            get_nfts_fio_address(const get_nfts_fio_address_params &params) const;
            ////////////////////////

            // get_nfts_hash
            struct get_nfts_hash_params {
                string hash;
                int32_t offset = 0;
                int32_t limit = 1000;
            };

            struct get_nfts_hash_result {
                vector<nft_info> nfts;
                uint32_t more;
                optional<bool> time_limit_exceeded_error;
            };

            get_nfts_hash_result
            get_nfts_hash(const get_nfts_hash_params &params) const;
            ////////////////////////


            // get_nfts_contract
            struct get_nfts_contract_params {
                string chain_code;
                string contract_address;
                string token_id;
                int32_t offset = 0;
                int32_t limit = 1000;
            };

            struct get_nfts_contract_result {
                vector<nft_info> nfts;
                uint32_t more;
                optional<bool> time_limit_exceeded_error;
            };

            get_nfts_contract_result
            get_nfts_contract(const get_nfts_contract_params &params) const;
            ////////////////////////

            struct get_whitelist_params {
                string fio_public_key;
            };

            struct get_whitelist_result {
                vector<whitelist_info> whitelisted_parties;
            };

            get_whitelist_result
            get_whitelist(const get_whitelist_params &params) const;

            struct check_whitelist_params {
                string fio_public_key_hash;
            };

            struct check_whitelist_result {
                uint8_t in_whitelist;
            };

            check_whitelist_result
            check_whitelist(const check_whitelist_params &params) const;

            //FIP-36 begin
            struct get_account_fio_public_key_params {
                string account;
            };

            struct get_account_fio_public_key_result {
                string fio_public_key;
            };
            //FIP-36 end

            struct get_fio_names_params {
                string fio_public_key;
            };
            struct get_fio_names_result {
                vector<fiodomain_record> fio_domains;
                vector<fioaddress_record> fio_addresses;
            };

            struct get_fio_domains_params {
                string fio_public_key;
                int32_t offset = 0;
                int32_t limit = 0;
            };
            struct get_fio_domains_result {
                vector<fiodomain_record> fio_domains;
                uint32_t more;
            };

            struct get_fio_addresses_params {
                string fio_public_key;
                int32_t offset = 0;
                int32_t limit = 0;
            };
            struct get_fio_addresses_result {
                vector<fioaddress_record> fio_addresses;
                uint32_t more;
            };

            struct get_oracle_fees_params {
            };

            struct get_oracle_fees_result {
                vector<oraclefee_record> oracle_fees;
            };

            struct lockperiodv2 {
                uint64_t duration = 0;
                uint64_t amount = 0;
            };

            struct get_locks_params {
                fc::string fio_public_key;
            };

            struct get_locks_result {
                    uint64_t lock_amount=0;
                    uint64_t remaining_lock_amount = 0;
                    uint64_t time_stamp = 0;  //time the lock was created.
                    uint64_t payouts_performed = 0;
                    uint32_t can_vote = 0;
                    vector<lockperiodv2> unlock_periods;
            };

            get_locks_result get_locks(const get_locks_params &params) const;


            //FIP-39 begin

            struct get_encrypt_key_params {
                fc::string fio_address;
            };

            struct get_encrypt_key_result {
                string encrypt_public_key = "";
            };

            get_encrypt_key_result get_encrypt_key(const get_encrypt_key_params &params) const;

            //FIP-39 end

            //FIP-40
            struct get_grantee_permissions_params {
                fc::string grantee_account;
                int32_t offset = 0;
                int32_t limit = 0;
            };

            struct permission_info {
                fc::string grantee_account;
                fc::string permission_name;
                fc::string permission_info;
                fc::string object_name;
                fc::string grantor_account;
            };


            struct get_grantee_permissions_result {
                vector<permission_info> permissions;
                bool more;
            };

            get_grantee_permissions_result get_grantee_permissions(const get_grantee_permissions_params &params) const;

            struct get_grantor_permissions_params {
                fc::string grantor_account;
                int32_t offset = 0;
                int32_t limit = 0;
            };

            struct get_grantor_permissions_result {
                vector<permission_info> permissions;
                bool more;
            };

            get_grantor_permissions_result get_grantor_permissions(const get_grantor_permissions_params &params) const;

            struct get_object_permissions_params {
                fc::string permission_name;
                fc::string object_name;
                int32_t offset = 0;
                int32_t limit = 0;
            };

            struct get_object_permissions_result {
                vector<permission_info> permissions;
                bool more;
            };

            get_object_permissions_result get_object_permissions(const get_object_permissions_params &params) const;


            struct get_fio_balance_params {
                fc::string fio_public_key;
            };

            struct get_fio_balance_result {
                uint64_t balance;
                uint64_t available;
                uint64_t staked = 0;
                uint64_t srps = 0;
                string roe = "";
            };

            get_fio_balance_result get_fio_balance(const get_fio_balance_params &params) const;

            struct get_actor_params {
                fc::string fio_public_key;
            };

            struct get_actor_result {
                string actor;
            };

            get_actor_result get_actor(const get_actor_params &params) const;

            //Fio API get_fee
            struct get_fee_params {
                string end_point;
                string fio_address;
            };
            struct get_fee_result {
                uint64_t fee;
            };

            get_fee_result get_fee(const get_fee_params &params) const;
            //Fio API get_fee

            struct get_pub_address_params {
                fc::string fio_address;
                fc::string token_code;
                fc::string chain_code;
            };

            struct get_pub_address_result {
                fc::string public_address;
            };

            get_pub_address_result get_pub_address(const get_pub_address_params &params) const;

            struct get_pub_addresses_params {
                fc::string fio_address;
                int32_t offset = 0;
                int32_t limit = 0;

            };

            struct address_info {
              fc::string chain_code;
              fc::string token_code;
              fc::string public_address;

            };
            struct get_pub_addresses_result {
                vector<address_info> public_addresses;
                bool more;
            };

            get_pub_addresses_result get_pub_addresses(const get_pub_addresses_params &params) const;

            //FIP-36 begin
            get_account_fio_public_key_result get_account_fio_public_key(const get_account_fio_public_key_params &params) const;
            //FIP-36 end


            /**
             * Lookup FIO domains and addresses based upon public address
             * @param params
             * @return
             */
            get_fio_names_result get_fio_names(const get_fio_names_params &params) const;
            get_fio_domains_result get_fio_domains(const get_fio_domains_params &params) const;
            get_fio_addresses_result get_fio_addresses(const get_fio_addresses_params &params) const;
            get_oracle_fees_result get_oracle_fees(const get_oracle_fees_params &params) const;

            //avail_check - FIO Address or Domain availability check
            struct avail_check_params {
                string fio_name;
            };

            struct avail_check_result {
                uint8_t is_registered = 0;
            };

            avail_check_result avail_check(const avail_check_params &params) const;

            //key lookups
            struct fio_key_lookup_params {
                string key;     // chain key e.g. for Ethereum: 0xC2D7CF95645D33006175B78989035C7c9061d3F9
                string chain;   // chain name e.g. BTC, ETH, EOS etc.
            };
            struct fio_key_lookup_result {
                string name = "";   // FIO name
                string expiration = "";
            };

            /**
             * Lookup FIO name based upon blockchain key(address|account)
             * @param params
             * @return
             */
            fio_key_lookup_result fio_key_lookup(const fio_key_lookup_params &params) const;


            static void copy_inline_row(const chain::key_value_object &obj, vector<char> &data) {
                data.resize(obj.value.size());
                memcpy(data.data(), obj.value.data(), obj.value.size());
            }

            template<typename Function>
            void walk_key_value_table(const name &code, const name &scope, const name &table, Function f) const {
                const auto &d = db.db();
                const auto *t_id = d.find<chain::table_id_object, chain::by_code_scope_table>(
                        boost::make_tuple(code, scope, table));
                if (t_id != nullptr) {
                    const auto &idx = d.get_index<chain::key_value_index, chain::by_scope_primary>();
                    decltype(t_id->id) next_tid(t_id->id._id + 1);
                    auto lower = idx.lower_bound(boost::make_tuple(t_id->id));
                    auto upper = idx.lower_bound(boost::make_tuple(next_tid));

                    for (auto itr = lower; itr != upper; ++itr) {
                        if (!f(*itr)) {
                            break;
                        }
                    }
                }
            }

            static uint64_t get_table_index_name(const read_only::get_table_rows_params &p, bool &primary);

            template<typename IndexType, typename SecKeyType, typename ConvFn>
            read_only::get_table_rows_result
            get_table_rows_by_seckey(const read_only::get_table_rows_params &p, const abi_def &abi, ConvFn conv) const {
                read_only::get_table_rows_result result;
                const auto &d = db.db();

                uint64_t scope = convert_to_type<uint64_t>(p.scope, "scope");

                abi_serializer abis;
                abis.set_abi(abi, abi_serializer_max_time);
                bool primary = false;
                const uint64_t table_with_index = get_table_index_name(p, primary);
                const auto *t_id = d.find<chain::table_id_object, chain::by_code_scope_table>(
                        boost::make_tuple(p.code, scope, p.table));
                const auto *index_t_id = d.find<chain::table_id_object, chain::by_code_scope_table>(
                        boost::make_tuple(p.code, scope, table_with_index));
                if (t_id != nullptr && index_t_id != nullptr) {
                    using secondary_key_type = std::result_of_t<decltype(conv)(SecKeyType)>;
                    static_assert(
                            std::is_same<typename IndexType::value_type::secondary_key_type, secondary_key_type>::value,
                            "Return type of conv does not match type of secondary key for IndexType");

                    const auto &secidx = d.get_index<IndexType, chain::by_secondary>();
                    auto lower_bound_lookup_tuple = std::make_tuple(index_t_id->id._id,
                                                                    eosio::chain::secondary_key_traits<secondary_key_type>::true_lowest(),
                                                                    std::numeric_limits<uint64_t>::lowest());
                    auto upper_bound_lookup_tuple = std::make_tuple(index_t_id->id._id,
                                                                    eosio::chain::secondary_key_traits<secondary_key_type>::true_highest(),
                                                                    std::numeric_limits<uint64_t>::max());

                    if (p.lower_bound.size()) {
                        if (p.key_type == "name") {
                            name s(p.lower_bound);
                            SecKeyType lv = convert_to_type<SecKeyType>(s.to_string(),
                                                                        "lower_bound name"); // avoids compiler error
                            std::get<1>(lower_bound_lookup_tuple) = conv(lv);
                        } else {
                            SecKeyType lv = convert_to_type<SecKeyType>(p.lower_bound, "lower_bound");
                            std::get<1>(lower_bound_lookup_tuple) = conv(lv);
                        }
                    }

                    if (p.upper_bound.size()) {
                        if (p.key_type == "name") {
                            name s(p.upper_bound);
                            SecKeyType uv = convert_to_type<SecKeyType>(s.to_string(), "upper_bound name");
                            std::get<1>(upper_bound_lookup_tuple) = conv(uv);
                        } else {
                            SecKeyType uv = convert_to_type<SecKeyType>(p.upper_bound, "upper_bound");
                            std::get<1>(upper_bound_lookup_tuple) = conv(uv);
                        }
                    }

                    if (upper_bound_lookup_tuple < lower_bound_lookup_tuple)
                        return result;

                    auto walk_table_row_range = [&](auto itr, auto end_itr) {
                        auto cur_time = fc::time_point::now();
                        //FIP-46 begin
                        auto end_time = cur_time + fc::microseconds(SECONDARY_INDEX_MAX_READ_TIME_MICROSECONDS);
                        //FIP-46 end
                        vector<char> data;
                        for (unsigned int count = 0; cur_time <= end_time && count < p.limit &&
                                                     itr != end_itr; ++itr, cur_time = fc::time_point::now()) {
                            const auto *itr2 = d.find<chain::key_value_object, chain::by_scope_primary>(
                                    boost::make_tuple(t_id->id, itr->primary_key));
                            if (itr2 == nullptr) continue;
                            copy_inline_row(*itr2, data);

                            fc::variant data_var;
                            if (p.json) {
                                data_var = abis.binary_to_variant(abis.get_table_type(p.table), data,
                                                                  abi_serializer_max_time, shorten_abi_errors);
                            } else {
                                data_var = fc::variant(data);
                            }

                            if (p.show_payer && *p.show_payer) {
                                result.rows.emplace_back(
                                        fc::mutable_variant_object("data", std::move(data_var))("payer", itr->payer));
                            } else {
                                result.rows.emplace_back(std::move(data_var));
                            }

                            ++count;
                        }
                        if (itr != end_itr) {
                            result.more = true;
                        }
                    };

                    auto lower = secidx.lower_bound(lower_bound_lookup_tuple);
                    auto upper = secidx.upper_bound(upper_bound_lookup_tuple);
                    if (p.reverse && *p.reverse) {
                        walk_table_row_range(boost::make_reverse_iterator(upper), boost::make_reverse_iterator(lower));
                    } else {
                        walk_table_row_range(lower, upper);
                    }
                }
                return result;
            }

            template<typename IndexType>
            read_only::get_table_rows_result
            get_table_rows_ex(const read_only::get_table_rows_params &p, const abi_def &abi) const {
                read_only::get_table_rows_result result;
                const auto &d = db.db();

                uint64_t scope = convert_to_type<uint64_t>(p.scope, "scope");

                abi_serializer abis;
                abis.set_abi(abi, abi_serializer_max_time);
                const auto *t_id = d.find<chain::table_id_object, chain::by_code_scope_table>(
                        boost::make_tuple(p.code, scope, p.table));
                if (t_id != nullptr) {
                    const auto &idx = d.get_index<IndexType, chain::by_scope_primary>();
                    auto lower_bound_lookup_tuple = std::make_tuple(t_id->id, std::numeric_limits<uint64_t>::lowest());
                    auto upper_bound_lookup_tuple = std::make_tuple(t_id->id, std::numeric_limits<uint64_t>::max());

                    if (p.lower_bound.size()) {
                        if (p.key_type == "name") {
                            name s(p.lower_bound);
                            std::get<1>(lower_bound_lookup_tuple) = s.value;
                        } else {
                            auto lv = convert_to_type<typename IndexType::value_type::key_type>(p.lower_bound,
                                                                                                "lower_bound");
                            std::get<1>(lower_bound_lookup_tuple) = lv;
                        }
                    }

                    if (p.upper_bound.size()) {
                        if (p.key_type == "name") {
                            name s(p.upper_bound);
                            std::get<1>(upper_bound_lookup_tuple) = s.value;
                        } else {
                            auto uv = convert_to_type<typename IndexType::value_type::key_type>(p.upper_bound,
                                                                                                "upper_bound");
                            std::get<1>(upper_bound_lookup_tuple) = uv;
                        }
                    }

                    if (upper_bound_lookup_tuple < lower_bound_lookup_tuple)
                        return result;

                    auto walk_table_row_range = [&](auto itr, auto end_itr) {
                        auto cur_time = fc::time_point::now();
                        //FIP-46 begin
                        auto end_time = cur_time + fc::microseconds(PRIMARY_INDEX_MAX_READ_TIME_MICROSECONDS);
                        //FIP-46 end
                        vector<char> data;
                        for (unsigned int count = 0; cur_time <= end_time && count < p.limit &&
                                                     itr != end_itr; ++count, ++itr, cur_time = fc::time_point::now()) {
                            copy_inline_row(*itr, data);

                            fc::variant data_var;
                            if (p.json) {
                                data_var = abis.binary_to_variant(abis.get_table_type(p.table), data,
                                                                  abi_serializer_max_time, shorten_abi_errors);
                            } else {
                                data_var = fc::variant(data);
                            }

                            if (p.show_payer && *p.show_payer) {
                                result.rows.emplace_back(
                                        fc::mutable_variant_object("data", std::move(data_var))("payer", itr->payer));
                            } else {
                                result.rows.emplace_back(std::move(data_var));
                            }
                        }
                        if (itr != end_itr) {
                            result.more = true;
                        }
                    };

                    auto lower = idx.lower_bound(lower_bound_lookup_tuple);
                    auto upper = idx.upper_bound(upper_bound_lookup_tuple);
                    if (p.reverse && *p.reverse) {
                        walk_table_row_range(boost::make_reverse_iterator(upper), boost::make_reverse_iterator(lower));
                    } else {
                        walk_table_row_range(lower, upper);
                    }
                }
                return result;
            }

            chain::symbol extract_core_symbol() const;

            friend struct resolver_factory<read_only>;


        };

        class read_write {
            controller &db;
            const fc::microseconds abi_serializer_max_time;
        public:
            read_write(controller &db, const fc::microseconds &abi_serializer_max_time);

            void validate() const;

            using push_block_params = chain::signed_block;
            using push_block_results = empty;

            void
            push_block(push_block_params &&params, chain::plugin_interface::next_function<push_block_results> next);

            using push_transaction_params = fc::variant_object;
            struct push_transaction_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void push_transaction(const push_transaction_params &params,
                                  chain::plugin_interface::next_function<push_transaction_results> next);


            //FIP-40
            using add_fio_permission_params = fc::variant_object;

            struct add_fio_permission_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void add_fio_permission(const add_fio_permission_params &params,
                                    chain::plugin_interface::next_function<add_fio_permission_results> next);

            using remove_fio_permission_params = fc::variant_object;

            struct remove_fio_permission_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void remove_fio_permission(const remove_fio_permission_params &params,
                                       chain::plugin_interface::next_function<remove_fio_permission_results> next);



            using register_fio_address_params = fc::variant_object;

            struct register_fio_address_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void register_fio_address(const register_fio_address_params &params,
                                      chain::plugin_interface::next_function<register_fio_address_results> next);

            using add_nft_params = fc::variant_object;

            struct add_nft_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void add_nft(const add_nft_params &params,
                                      chain::plugin_interface::next_function<add_nft_results> next);


            using remove_nft_params = fc::variant_object;

            struct remove_nft_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void remove_nft(const remove_nft_params &params,
                                      chain::plugin_interface::next_function<remove_nft_results> next);


            using remove_all_nfts_params = fc::variant_object;

            struct remove_all_nfts_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void remove_all_nfts(const remove_all_nfts_params &params,
                                      chain::plugin_interface::next_function<remove_all_nfts_results> next);

            using register_fio_domain_address_params = fc::variant_object;
            struct register_fio_domain_address_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void register_fio_domain_address(const register_fio_domain_address_params &params,
                                       chain::plugin_interface::next_function<register_fio_domain_address_results> next);



            using set_fio_domain_public_params = fc::variant_object;
            struct set_fio_domain_public_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void set_fio_domain_public(const set_fio_domain_public_params &params,
                                       chain::plugin_interface::next_function<set_fio_domain_public_results> next);

            using register_fio_domain_params = fc::variant_object;
            struct register_fio_domain_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void register_fio_domain(const register_fio_domain_params &params,
                                     chain::plugin_interface::next_function<register_fio_domain_results> next);

            using add_pub_address_params = fc::variant_object;
            struct add_pub_address_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void add_pub_address(const add_pub_address_params &params,
                                 chain::plugin_interface::next_function<add_pub_address_results> next);

            using remove_pub_address_params = fc::variant_object;
            struct remove_pub_address_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void remove_pub_address(const remove_pub_address_params &params,
                                 chain::plugin_interface::next_function<remove_pub_address_results> next);

            using remove_all_pub_addresses_params = fc::variant_object;
            struct remove_all_pub_addresses_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void remove_all_pub_addresses(const remove_all_pub_addresses_params &params,
                                 chain::plugin_interface::next_function<remove_all_pub_addresses_results> next);


            using transfer_tokens_pub_key_params = fc::variant_object;
            struct transfer_tokens_pub_key_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void transfer_tokens_pub_key(const transfer_tokens_pub_key_params &params,
                                         chain::plugin_interface::next_function<transfer_tokens_pub_key_results> next);

            //FIP-38 begin
            using new_fio_chain_account_params = fc::variant_object;
            struct new_fio_chain_account_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void new_fio_chain_account(const new_fio_chain_account_params &params,
                                         chain::plugin_interface::next_function<new_fio_chain_account_results> next);
            //FIP-38 end


            using transfer_locked_tokens_params = fc::variant_object;
            struct transfer_locked_tokens_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void transfer_locked_tokens(const transfer_locked_tokens_params &params,
                                         chain::plugin_interface::next_function<transfer_locked_tokens_results> next);

            //FIP-39 begin
            using update_encrypt_key_params = fc::variant_object;
            struct update_encrypt_key_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void update_encrypt_key(const update_encrypt_key_params &params,
                                  chain::plugin_interface::next_function<update_encrypt_key_results> next);


            //FIP-39 end

            //begin renew_domain
            using renew_fio_domain_params = fc::variant_object;
            struct renew_fio_domain_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void renew_fio_domain(const renew_fio_domain_params &params,
                                  chain::plugin_interface::next_function<renew_fio_domain_results> next);

            //end renew_domain

            //begin renew_address
            using renew_fio_address_params = fc::variant_object;
            struct renew_fio_address_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void renew_fio_address(const renew_fio_address_params &params,
                                   chain::plugin_interface::next_function<renew_fio_address_results> next);

            //end renew_address

            //begin burn_fio_address
            using burn_fio_address_params = fc::variant_object;
            struct burn_fio_address_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void burn_fio_address(const burn_fio_address_params &params,
                                     chain::plugin_interface::next_function<burn_fio_address_results> next);

            //begin transfer_fio_domain
            using transfer_fio_domain_params = fc::variant_object;
            struct transfer_fio_domain_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void transfer_fio_domain(const transfer_fio_domain_params &params,
                                   chain::plugin_interface::next_function<transfer_fio_domain_results> next);

            //begin transfer_fio_address
            using transfer_fio_address_params = fc::variant_object;
            struct transfer_fio_address_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void transfer_fio_address(const transfer_fio_address_params &params,
                                     chain::plugin_interface::next_function<transfer_fio_address_results> next);


            using burn_expired_params = fc::variant_object;
            struct burn_expired_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void burn_expired(const burn_expired_params &params,
                              chain::plugin_interface::next_function<burn_expired_results> next);

            using compute_fees_params = fc::variant_object;
            struct compute_fees_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void compute_fees(const compute_fees_params &params,
                              chain::plugin_interface::next_function<compute_fees_results> next);

            using unregister_producer_params = fc::variant_object;
            struct unregister_producer_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void unregister_producer(const unregister_producer_params &params,
                                     chain::plugin_interface::next_function<unregister_producer_results> next);

            using register_producer_params = fc::variant_object;
            struct register_producer_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void register_producer(const register_producer_params &params,
                                   chain::plugin_interface::next_function<register_producer_results> next);

            using vote_producer_params = fc::variant_object;
            struct vote_producer_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void vote_producer(const vote_producer_params &params,
                               chain::plugin_interface::next_function<vote_producer_results> next);

            using proxy_vote_params = fc::variant_object;
            struct proxy_vote_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void proxy_vote(const proxy_vote_params &params,
                            chain::plugin_interface::next_function<proxy_vote_results> next);

            using submit_fee_ratios_params = fc::variant_object;
            struct submit_fee_ratios_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void submit_fee_ratios(const submit_fee_ratios_params &params,
                                   chain::plugin_interface::next_function<submit_fee_ratios_results> next);

            using submit_fee_multiplier_params = fc::variant_object;
            struct submit_fee_multiplier_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void submit_fee_multiplier(const submit_fee_multiplier_params &params,
                                       chain::plugin_interface::next_function<submit_fee_multiplier_results> next);

            using submit_bundled_transaction_params = fc::variant_object;
            struct submit_bundled_transaction_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void submit_bundled_transaction(const submit_bundled_transaction_params &params,
                                            chain::plugin_interface::next_function<submit_bundled_transaction_results> next);

            //begin unregister_proxy
            using unregister_proxy_params = fc::variant_object;
            struct unregister_proxy_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void unregister_proxy(const unregister_proxy_params &params,
                                  chain::plugin_interface::next_function<unregister_proxy_results> next);
            //end unregister_proxy

            //begin register_proxy
            using register_proxy_params = fc::variant_object;
            struct register_proxy_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void register_proxy(const register_proxy_params &params,
                                chain::plugin_interface::next_function<register_proxy_results> next);
            //end register_proxy

            //Begin Added for reject request api method
            using reject_funds_request_params = fc::variant_object;
            struct reject_funds_request_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void reject_funds_request(const reject_funds_request_params &params,
                                      chain::plugin_interface::next_function<reject_funds_request_results> next);
            //End added for record send api method.

            //begin added for cancel request
            using cancel_funds_request_params = fc::variant_object;
            struct cancel_funds_request_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void cancel_funds_request(const cancel_funds_request_params &params,
                                      chain::plugin_interface::next_function<cancel_funds_request_results> next);
            //end added for cancel request

            //Begin Added for record send api method
            using record_obt_data_params = fc::variant_object;

            struct record_obt_data_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void record_obt_data(const record_obt_data_params &params,
                                 chain::plugin_interface::next_function<record_obt_data_results> next);
            //End added for record send api method.


            //Begin Added for record send api method
            using record_send_params = fc::variant_object;

            struct record_send_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void record_send(const record_send_params &params,
                             chain::plugin_interface::next_function<record_send_results> next);
            //End added for record send api method.

            //Begin Added for new funds request api method
            using new_funds_request_params = fc::variant_object;

            struct new_funds_request_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void new_funds_request(const new_funds_request_params &params,
                                   chain::plugin_interface::next_function<new_funds_request_results> next);

            //Begin Added for new pay_tpid_rewards  api method
            using pay_tpid_rewards_params = fc::variant_object;

            struct pay_tpid_rewards_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void pay_tpid_rewards(const pay_tpid_rewards_params &params,
                                  chain::plugin_interface::next_function<pay_tpid_rewards_results> next);

            //End added for new pay_tpid_rewards api method.

            //Begin Added for new claim_bp_rewards  api method
            using claim_bp_rewards_params = fc::variant_object;

            struct claim_bp_rewards_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void claim_bp_rewards(const claim_bp_rewards_params &params,
                                  chain::plugin_interface::next_function<claim_bp_rewards_results> next);

            //End added for new claim_bp_rewards api method.

            //Begin Added for add_bundled_transactions api method
            using add_bundled_transactions_params = fc::variant_object;

            struct add_bundled_transactions_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void add_bundled_transactions(const add_bundled_transactions_params &params,
                                  chain::plugin_interface::next_function<add_bundled_transactions_results> next);
            //end

            //Begin Added for wrap_fio_tokens api method
            using wrap_fio_tokens_params = fc::variant_object;

            struct wrap_fio_tokens_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void wrap_fio_tokens(const wrap_fio_tokens_params &params,
                                 chain::plugin_interface::next_function<wrap_fio_tokens_results> next);
            //end

             //Begin Added for wrap_fio_domains api method
            using wrap_fio_domains_params = fc::variant_object;

            struct wrap_fio_domains_results {
                chain::transaction_id_type transaction_id;
                fc::variant processed;
            };

            void wrap_fio_domains(const wrap_fio_domains_params &params,
                                 chain::plugin_interface::next_function<wrap_fio_domains_results> next);
            //end

            using push_transactions_params  = vector<push_transaction_params>;
            using push_transactions_results = vector<push_transaction_results>;

            void push_transactions(const push_transactions_params &params,
                                   chain::plugin_interface::next_function<push_transactions_results> next);

            using send_transaction_params = push_transaction_params;
            using send_transaction_results = push_transaction_results;

            void send_transaction(const send_transaction_params &params,
                                  chain::plugin_interface::next_function<send_transaction_results> next);

            friend resolver_factory<read_write>;
        };

        //support for --key_types [sha256,ripemd160] and --encoding [dec/hex]
        constexpr const char i64[] = "i64";
        constexpr const char i128[] = "i128";
        constexpr const char i256[] = "i256";
        constexpr const char float64[] = "float64";
        constexpr const char float128[] = "float128";
        constexpr const char sha256[] = "sha256";
        constexpr const char ripemd160[] = "ripemd160";
        constexpr const char dec[] = "dec";
        constexpr const char hex[] = "hex";


        template<const char *key_type, const char *encoding = chain_apis::dec>
        struct keytype_converter;

        template<>
        struct keytype_converter<chain_apis::sha256, chain_apis::hex> {
            using input_type = chain::checksum256_type;
            using index_type = chain::index256_index;

            static auto function() {
                return [](const input_type &v) {
                    chain::key256_t k;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
                    k[0] = ((uint128_t *) &v._hash)[0]; //0-127
                    k[1] = ((uint128_t *) &v._hash)[1]; //127-256
#pragma GCC diagnostic pop
                    return k;
                };
            }
        };

        //key160 support with padding zeros in the end of key256
        template<>
        struct keytype_converter<chain_apis::ripemd160, chain_apis::hex> {
            using input_type = chain::checksum160_type;
            using index_type = chain::index256_index;

            static auto function() {
                return [](const input_type &v) {
                    chain::key256_t k;
                    memset(k.data(), 0, sizeof(k));
                    memcpy(k.data(), v._hash, sizeof(v._hash));
                    return k;
                };
            }
        };

        template<>
        struct keytype_converter<chain_apis::i256> {
            using input_type = boost::multiprecision::uint256_t;
            using index_type = chain::index256_index;

            static auto function() {
                return [](const input_type v) {
                    chain::key256_t k;
                    k[0] = ((uint128_t *) &v)[0]; //0-127
                    k[1] = ((uint128_t *) &v)[1]; //127-256
                    return k;
                };
            }
        };

    } // namespace chain_apis

    class chain_plugin : public plugin<chain_plugin> {
    public:
        APPBASE_PLUGIN_REQUIRES()

        chain_plugin();

        virtual ~chain_plugin();

        virtual void set_program_options(options_description &cli, options_description &cfg) override;

        void plugin_initialize(const variables_map &options);

        void plugin_startup();

        void plugin_shutdown();

        chain_apis::read_only get_read_only_api() const {
            return chain_apis::read_only(chain(), get_abi_serializer_max_time());
        }

        chain_apis::read_write get_read_write_api() {
            return chain_apis::read_write(chain(), get_abi_serializer_max_time());
        }

        void accept_block(const chain::signed_block_ptr &block);

        void accept_transaction(const chain::packed_transaction &trx,
                                chain::plugin_interface::next_function<chain::transaction_trace_ptr> next);

        void accept_transaction(const chain::transaction_metadata_ptr &trx,
                                chain::plugin_interface::next_function<chain::transaction_trace_ptr> next);

        bool block_is_on_preferred_chain(const chain::block_id_type &block_id);

        static bool recover_reversible_blocks(const fc::path &db_dir,
                                              uint32_t cache_size,
                                              optional<fc::path> new_db_dir = optional<fc::path>(),
                                              uint32_t truncate_at_block = 0
        );

        static bool import_reversible_blocks(const fc::path &reversible_dir,
                                             uint32_t cache_size,
                                             const fc::path &reversible_blocks_file
        );

        static bool export_reversible_blocks(const fc::path &reversible_dir,
                                             const fc::path &reversible_blocks_file
        );

        // Only call this after plugin_initialize()!
        controller &chain();

        // Only call this after plugin_initialize()!
        const controller &chain() const;

        chain::chain_id_type get_chain_id() const;

        fc::microseconds get_abi_serializer_max_time() const;

        static void handle_guard_exception(const chain::guard_exception &e);

        static void handle_db_exhaustion();

        static void handle_bad_alloc();

    private:
        static void log_guard_exception(const chain::guard_exception &e);

        unique_ptr<class chain_plugin_impl> my;

    };

}

FC_REFLECT(eosio::chain_apis::permission, (perm_name)(parent)(required_auth))
FC_REFLECT(eosio::chain_apis::empty,)
FC_REFLECT(eosio::chain_apis::read_only::get_info_results,
           (server_version)(chain_id)(head_block_num)(last_irreversible_block_num)(last_irreversible_block_id)(
                   head_block_id)(head_block_time)(head_block_producer)(virtual_block_cpu_limit)(
                   virtual_block_net_limit)(block_cpu_limit)(block_net_limit)(server_version_string)(
                   fork_db_head_block_num)(fork_db_head_block_id))
FC_REFLECT(eosio::chain_apis::read_only::get_activated_protocol_features_params,
           (lower_bound)(upper_bound)(limit)(search_by_block_num)(reverse))
FC_REFLECT(eosio::chain_apis::read_only::get_activated_protocol_features_results, (activated_protocol_features)(more))
FC_REFLECT(eosio::chain_apis::read_only::get_block_params, (block_num_or_id))
FC_REFLECT(eosio::chain_apis::read_only::get_block_header_state_params, (block_num_or_id))
FC_REFLECT(eosio::chain_apis::read_write::push_transaction_results, (transaction_id)(processed))
FC_REFLECT(eosio::chain_apis::read_only::get_table_rows_params,
           (json)(code)(scope)(table)(table_key)(lower_bound)(upper_bound)(limit)(key_type)(index_position)(
                   encode_type)(reverse)(show_payer))
FC_REFLECT(eosio::chain_apis::read_only::get_table_rows_result, (rows)(more));
FC_REFLECT(eosio::chain_apis::read_only::get_locks_params, (fio_public_key))
FC_REFLECT(eosio::chain_apis::read_only::lockperiodv2, (duration)(amount))
FC_REFLECT(eosio::chain_apis::read_only::get_locks_result, (lock_amount)(remaining_lock_amount)(time_stamp)(payouts_performed)(can_vote)(unlock_periods))
FC_REFLECT(eosio::chain_apis::read_only::get_pending_fio_requests_params, (fio_public_key)(offset)(limit))
FC_REFLECT(eosio::chain_apis::read_only::get_pending_fio_requests_result, (requests)(more)(time_limit_exceeded_error))
FC_REFLECT(eosio::chain_apis::read_only::get_received_fio_requests_params, (fio_public_key)(offset)(limit))
FC_REFLECT(eosio::chain_apis::read_only::get_received_fio_requests_result, (requests)(more)(time_limit_exceeded_error))
FC_REFLECT(eosio::chain_apis::read_only::get_cancelled_fio_requests_params, (fio_public_key)(offset)(limit))
FC_REFLECT(eosio::chain_apis::read_only::get_cancelled_fio_requests_result, (requests)(more)(time_limit_exceeded_error))
FC_REFLECT(eosio::chain_apis::read_only::get_sent_fio_requests_params, (fio_public_key)(offset)(limit))
FC_REFLECT(eosio::chain_apis::read_only::get_sent_fio_requests_result, (requests)(more))
FC_REFLECT(eosio::chain_apis::read_only::get_actions_params, (offset)(limit))
FC_REFLECT(eosio::chain_apis::read_only::get_actions_result, (actions)(more))
FC_REFLECT(eosio::chain_apis::read_only::get_obt_data_params, (fio_public_key)(offset)(limit))
FC_REFLECT(eosio::chain_apis::read_only::get_obt_data_result, (obt_data_records)(more)(time_limit_exceeded_error))
FC_REFLECT(eosio::chain_apis::read_only::get_nfts_fio_address_params, (fio_address)(offset)(limit))
FC_REFLECT(eosio::chain_apis::read_only::get_nfts_fio_address_result, (nfts)(more)(time_limit_exceeded_error))
FC_REFLECT(eosio::chain_apis::read_only::get_nfts_hash_params, (hash)(offset)(limit))
FC_REFLECT(eosio::chain_apis::read_only::get_nfts_hash_result, (nfts)(more)(time_limit_exceeded_error))
FC_REFLECT(eosio::chain_apis::read_only::get_nfts_contract_params, (chain_code)(contract_address)(token_id)(offset)(limit))
FC_REFLECT(eosio::chain_apis::read_only::get_nfts_contract_result, (nfts)(more)(time_limit_exceeded_error))
FC_REFLECT(eosio::chain_apis::nft_info, (fio_address)(chain_code)(contract_address)(token_id)(url)(hash)(metadata))
FC_REFLECT(eosio::chain_apis::read_only::get_whitelist_params, (fio_public_key))
FC_REFLECT(eosio::chain_apis::read_only::get_whitelist_result, (whitelisted_parties))
FC_REFLECT(eosio::chain_apis::read_only::get_escrow_listings_params, (status)(offset)(limit)(actor))
FC_REFLECT(eosio::chain_apis::read_only::get_escrow_listings_result, (listings)(more)(time_limit_exceeded_error))
FC_REFLECT(eosio::chain_apis::whitelist_info, (fio_public_key_hash)(content))
FC_REFLECT(eosio::chain_apis::read_only::check_whitelist_params, (fio_public_key_hash))
FC_REFLECT(eosio::chain_apis::read_only::check_whitelist_result, (in_whitelist))
FC_REFLECT(eosio::chain_apis::request_record,
           (fio_request_id)(payer_fio_address)(payee_fio_address)(payer_fio_public_key)(payee_fio_public_key)(content)
                   (time_stamp))
FC_REFLECT(eosio::chain_apis::request_status_record,
           (fio_request_id)(payer_fio_address)(payee_fio_address)(payer_fio_public_key)(payee_fio_public_key)(content)(
                   time_stamp)
                   (status))
FC_REFLECT(eosio::chain_apis::action_record,(action)(contract)(block_timestamp))

FC_REFLECT(eosio::chain_apis::obt_records,
           (payer_fio_address)(payee_fio_address)(payer_fio_public_key)(payee_fio_public_key)(content)(fio_request_id)(
                   time_stamp)
                   (status))
FC_REFLECT(eosio::chain_apis::listing_record,
           (id)(commission_fee)(date_listed)(date_updated)(domain)(owner)(sale_price)(status))

FC_REFLECT(eosio::chain_apis::read_only::get_pub_address_params, (fio_address)(token_code)(chain_code))
FC_REFLECT(eosio::chain_apis::read_only::get_pub_address_result, (public_address));
FC_REFLECT(eosio::chain_apis::read_only::address_info, (public_address)(token_code)(chain_code))
FC_REFLECT(eosio::chain_apis::read_only::get_pub_addresses_params, (fio_address)(offset)(limit))
FC_REFLECT(eosio::chain_apis::read_only::get_pub_addresses_result, (public_addresses)(more));
FC_REFLECT(eosio::chain_apis::fiodomain_record, (fio_domain)(expiration)(is_public))
FC_REFLECT(eosio::chain_apis::fioaddress_record, (fio_address)(expiration)(remaining_bundled_tx))
FC_REFLECT(eosio::chain_apis::oraclefee_record, (fee_name)(fee_amount))
//FIP-36 begin
FC_REFLECT(eosio::chain_apis::read_only::get_account_fio_public_key_params, (account))
FC_REFLECT(eosio::chain_apis::read_only::get_account_fio_public_key_result, (fio_public_key));
//FIP-36 end
FC_REFLECT(eosio::chain_apis::read_only::get_fio_names_params, (fio_public_key))
FC_REFLECT(eosio::chain_apis::read_only::get_fio_names_result, (fio_domains)(fio_addresses));
FC_REFLECT(eosio::chain_apis::read_only::get_fio_domains_params, (fio_public_key)(offset)(limit))
FC_REFLECT(eosio::chain_apis::read_only::get_fio_domains_result, (fio_domains)(more));
FC_REFLECT(eosio::chain_apis::read_only::get_fio_addresses_params, (fio_public_key)(offset)(limit))
FC_REFLECT(eosio::chain_apis::read_only::get_fio_addresses_result, (fio_addresses)(more));
FC_REFLECT_EMPTY(eosio::chain_apis::read_only::get_oracle_fees_params);
FC_REFLECT(eosio::chain_apis::read_only::get_oracle_fees_result, (oracle_fees));
FC_REFLECT(eosio::chain_apis::read_only::get_fee_params, (end_point)(fio_address))
FC_REFLECT(eosio::chain_apis::read_only::get_fee_result, (fee));
FC_REFLECT(eosio::chain_apis::read_only::avail_check_params, (fio_name))
FC_REFLECT(eosio::chain_apis::read_only::avail_check_result, (is_registered));
FC_REFLECT(eosio::chain_apis::read_only::fio_key_lookup_params, (key)(chain))
FC_REFLECT(eosio::chain_apis::read_only::fio_key_lookup_result, (name)(expiration));
FC_REFLECT(eosio::chain_apis::read_write::register_fio_address_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::add_nft_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::remove_nft_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::remove_all_nfts_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::burn_fio_address_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::transfer_fio_domain_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::transfer_fio_address_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::set_fio_domain_public_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::register_fio_domain_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::reject_funds_request_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::cancel_funds_request_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::record_obt_data_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::record_send_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::submit_bundled_transaction_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::add_pub_address_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::remove_pub_address_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::remove_all_pub_addresses_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::transfer_tokens_pub_key_results, (transaction_id)(processed));
//FIP-38 begin
FC_REFLECT(eosio::chain_apis::read_write::new_fio_chain_account_results, (transaction_id)(processed));
//FIP-38 end
FC_REFLECT(eosio::chain_apis::read_write::transfer_locked_tokens_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::burn_expired_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::compute_fees_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::unregister_producer_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::register_producer_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::vote_producer_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::proxy_vote_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::submit_fee_multiplier_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::submit_fee_ratios_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::unregister_proxy_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::register_proxy_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::renew_fio_domain_results, (transaction_id)(processed));
//FIP-39 begin
FC_REFLECT(eosio::chain_apis::read_write::update_encrypt_key_results, (transaction_id)(processed));
//FIP-39 end
//FIP-40
FC_REFLECT(eosio::chain_apis::read_only::get_grantee_permissions_params, (grantee_account)(offset)(limit));
FC_REFLECT(eosio::chain_apis::read_only::permission_info, (grantee_account)(permission_name)(permission_info)(object_name)(grantor_account));
FC_REFLECT(eosio::chain_apis::read_only::get_grantee_permissions_result, (permissions)(more));
FC_REFLECT(eosio::chain_apis::read_only::get_grantor_permissions_params, (grantor_account)(offset)(limit));
FC_REFLECT(eosio::chain_apis::read_only::get_grantor_permissions_result, (permissions)(more));
FC_REFLECT(eosio::chain_apis::read_only::get_object_permissions_params, (permission_name)(object_name)(offset)(limit));
FC_REFLECT(eosio::chain_apis::read_only::get_object_permissions_result, (permissions)(more));
FC_REFLECT(eosio::chain_apis::read_write::add_fio_permission_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::remove_fio_permission_results, (transaction_id)(processed));

FC_REFLECT(eosio::chain_apis::read_write::renew_fio_address_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::new_funds_request_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::pay_tpid_rewards_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::claim_bp_rewards_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::add_bundled_transactions_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::wrap_fio_tokens_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::wrap_fio_domains_results, (transaction_id)(processed));
FC_REFLECT(eosio::chain_apis::read_write::register_fio_domain_address_results, (transaction_id)(processed));

FC_REFLECT(eosio::chain_apis::read_only::get_table_by_scope_params,
           (code)(table)(lower_bound)(upper_bound)(limit)(reverse))
FC_REFLECT(eosio::chain_apis::read_only::get_table_by_scope_result_row, (code)(scope)(table)(payer)(count));
FC_REFLECT(eosio::chain_apis::read_only::get_table_by_scope_result, (rows)(more));

FC_REFLECT(eosio::chain_apis::read_only::get_currency_balance_params, (code)(account)(symbol));
FC_REFLECT(eosio::chain_apis::read_only::get_currency_stats_params, (code)(symbol));
FC_REFLECT(eosio::chain_apis::read_only::get_currency_stats_result, (supply)(max_supply)(issuer));
//FIP-39 begin
FC_REFLECT(eosio::chain_apis::read_only::get_encrypt_key_params, (fio_address));
FC_REFLECT(eosio::chain_apis::read_only::get_encrypt_key_result, (encrypt_public_key));
//FIP-39 end
FC_REFLECT(eosio::chain_apis::read_only::get_fio_balance_params, (fio_public_key));
FC_REFLECT(eosio::chain_apis::read_only::get_fio_balance_result, (balance)(available)(staked)(srps)(roe));
FC_REFLECT(eosio::chain_apis::read_only::get_actor_params, (fio_public_key));
FC_REFLECT(eosio::chain_apis::read_only::get_actor_result, (actor));
FC_REFLECT(eosio::chain_apis::read_only::get_producers_params, (json)(lower_bound)(limit))
FC_REFLECT(eosio::chain_apis::read_only::get_producers_result, (producers)(total_producer_vote_weight)(more));

FC_REFLECT_EMPTY(eosio::chain_apis::read_only::get_producer_schedule_params)
FC_REFLECT(eosio::chain_apis::read_only::get_producer_schedule_result, (active)(pending)(proposed));

FC_REFLECT(eosio::chain_apis::read_only::get_scheduled_transactions_params, (json)(lower_bound)(limit))
FC_REFLECT(eosio::chain_apis::read_only::get_scheduled_transactions_result, (transactions)(more));

FC_REFLECT(eosio::chain_apis::read_only::get_account_results,
           (account_name)(head_block_num)(head_block_time)(privileged)(last_code_update)(created)
                   (core_liquid_balance)(ram_quota)(net_weight)(cpu_weight)(net_limit)(cpu_limit)(ram_usage)(
                   permissions)
                   (total_resources)(self_delegated_bandwidth)(refund_request)(voter_info))
// @swap code_hash
FC_REFLECT(eosio::chain_apis::read_only::get_code_results, (account_name)(code_hash)(wast)(wasm)(abi))
FC_REFLECT(eosio::chain_apis::read_only::get_code_hash_results, (account_name)(code_hash))
FC_REFLECT(eosio::chain_apis::read_only::get_abi_results, (account_name)(abi))
FC_REFLECT(eosio::chain_apis::read_only::get_account_params, (account_name)(expected_core_symbol))
FC_REFLECT(eosio::chain_apis::read_only::get_code_params, (account_name)(code_as_wasm))
FC_REFLECT(eosio::chain_apis::read_only::get_code_hash_params, (account_name))
FC_REFLECT(eosio::chain_apis::read_only::get_abi_params, (account_name))
FC_REFLECT(eosio::chain_apis::read_only::get_raw_code_and_abi_params, (account_name))
FC_REFLECT(eosio::chain_apis::read_only::get_raw_code_and_abi_results, (account_name)(wasm)(abi))
FC_REFLECT(eosio::chain_apis::read_only::get_raw_abi_params, (account_name)(abi_hash))
FC_REFLECT(eosio::chain_apis::read_only::get_raw_abi_results, (account_name)(code_hash)(abi_hash)(abi))
FC_REFLECT(eosio::chain_apis::read_only::producer_info, (producer_name))
FC_REFLECT(eosio::chain_apis::read_only::abi_json_to_bin_params, (code)(action)(args))
FC_REFLECT(eosio::chain_apis::read_only::abi_json_to_bin_result, (binargs))
FC_REFLECT(eosio::chain_apis::read_only::abi_bin_to_json_params, (code)(action)(binargs))
FC_REFLECT(eosio::chain_apis::read_only::abi_bin_to_json_result, (args))
FC_REFLECT(eosio::chain_apis::read_only::serialize_json_params, (action)(json))
FC_REFLECT(eosio::chain_apis::read_only::serialize_json_result, (serialized_json))
FC_REFLECT(eosio::chain_apis::read_only::get_required_keys_params, (transaction)(available_keys))
FC_REFLECT(eosio::chain_apis::read_only::get_required_keys_result, (required_keys))
