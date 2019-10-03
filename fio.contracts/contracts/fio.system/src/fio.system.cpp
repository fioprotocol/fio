/** fio system contract file
 *  Description: this contract controls many of the core functions of the fio protocol.
 *  @author Adam Androulidakis, Casey Gardiner, Ed Rotthoff
 *  @file fio.system.cpp
 *  @copyright Dapix
 *
 *  Changes:
 */
#include <fio.system/fio.system.hpp>
#include <eosiolib/dispatcher.hpp>
#include <eosiolib/crypto.h>
#include "producer_pay.cpp"
#include "delegate_bandwidth.cpp"
#include "voting.cpp"
#include <fio.name/fio.name.hpp>
#include <fio.fee/fio.fee.hpp>

namespace eosiosystem {

    system_contract::system_contract(name s, name code, datastream<const char *> ds)
            : native(s, code, ds),
              _voters(_self, _self.value),
              _producers(_self, _self.value),
              _global(_self, _self.value),
              _global2(_self, _self.value),
              _global3(_self, _self.value),
              _fionames(SystemContract, SystemContract.value),
              _domains(SystemContract, SystemContract.value),
              _accountmap(SystemContract, SystemContract.value),
              _fiofees(FeeContract, FeeContract.value){
        _gstate = _global.exists() ? _global.get() : get_default_parameters();
        _gstate2 = _global2.exists() ? _global2.get() : eosio_global_state2{};
        _gstate3 = _global3.exists() ? _global3.get() : eosio_global_state3{};
    }

    eosiosystem::eosio_global_state eosiosystem::system_contract::get_default_parameters() {
        eosio_global_state dp;
        get_blockchain_parameters(dp);
        return dp;
    }

    time_point eosiosystem::system_contract::current_time_point() {
        const static time_point ct{microseconds{static_cast<int64_t>( current_time())}};
        return ct;
    }

    time_point_sec eosiosystem::system_contract::current_time_point_sec() {
        const static time_point_sec cts{current_time_point()};
        return cts;
    }

    block_timestamp eosiosystem::system_contract::current_block_time() {
        const static block_timestamp cbt{current_time_point()};
        return cbt;
    }

    eosiosystem::system_contract::~system_contract() {
        _global.set(_gstate, _self);
        _global2.set(_gstate2, _self);
        _global3.set(_gstate3, _self);
    }

    void eosiosystem::system_contract::setparams(const eosio::blockchain_parameters &params) {
        require_auth(_self);
        (eosio::blockchain_parameters & )(_gstate) = params;
        check(3 <= _gstate.max_authority_depth, "max_authority_depth should be at least 3");
        set_blockchain_parameters(params);
    }

    void eosiosystem::system_contract::setpriv(name account, uint8_t ispriv) {
        require_auth(_self);
        set_privileged(account.value, ispriv);
    }

    void eosiosystem::system_contract::setalimits(name account, int64_t ram, int64_t net, int64_t cpu) {
        require_auth(_self);

        user_resources_table userres(_self, account.value);
        auto ritr = userres.find(account.value);
        check(ritr == userres.end(), "only supports unlimited accounts");

        auto vitr = _voters.find(account.value);
        if (vitr != _voters.end()) {
            bool ram_managed = has_field(vitr->flags1, voter_info::flags1_fields::ram_managed);
            bool net_managed = has_field(vitr->flags1, voter_info::flags1_fields::net_managed);
            bool cpu_managed = has_field(vitr->flags1, voter_info::flags1_fields::cpu_managed);
            check(!(ram_managed || net_managed || cpu_managed),
                  "cannot use setalimits on an account with managed resources");
        }

        set_resource_limits(account.value, ram, net, cpu);
    }

    void eosiosystem::system_contract::setacctram(name account, std::optional <int64_t> ram_bytes) {
        require_auth(_self);

        int64_t current_ram, current_net, current_cpu;
        get_resource_limits(account.value, &current_ram, &current_net, &current_cpu);

        int64_t ram = 0;

        if (!ram_bytes) {
            auto vitr = _voters.find(account.value);
            check(vitr != _voters.end() && has_field(vitr->flags1, voter_info::flags1_fields::ram_managed),
                  "RAM of account is already unmanaged");

            user_resources_table userres(_self, account.value);
            auto ritr = userres.find(account.value);

            ram = ram_gift_bytes;
            if (ritr != userres.end()) {
                ram += ritr->ram_bytes;
            }

            _voters.modify(vitr, same_payer, [&](auto &v) {
                v.flags1 = set_field(v.flags1, voter_info::flags1_fields::ram_managed, false);
            });
        } else {
            check(*ram_bytes >= 0, "not allowed to set RAM limit to unlimited");

            auto vitr = _voters.find(account.value);
            if (vitr != _voters.end()) {
                _voters.modify(vitr, same_payer, [&](auto &v) {
                    v.flags1 = set_field(v.flags1, voter_info::flags1_fields::ram_managed, true);
                });
            } else {
                _voters.emplace(account, [&](auto &v) {
                    v.owner = account;
                    v.flags1 = set_field(v.flags1, voter_info::flags1_fields::ram_managed, true);
                });
            }

            ram = *ram_bytes;
        }

        set_resource_limits(account.value, ram, current_net, current_cpu);
    }

    void eosiosystem::system_contract::setacctnet(name account, std::optional <int64_t> net_weight) {
        require_auth(_self);

        int64_t current_ram, current_net, current_cpu;
        get_resource_limits(account.value, &current_ram, &current_net, &current_cpu);

        int64_t net = 0;

        if (!net_weight) {
            auto vitr = _voters.find(account.value);
            check(vitr != _voters.end() && has_field(vitr->flags1, voter_info::flags1_fields::net_managed),
                  "Network bandwidth of account is already unmanaged");

            user_resources_table userres(_self, account.value);
            auto ritr = userres.find(account.value);

            if (ritr != userres.end()) {
                net = ritr->net_weight.amount;
            }

            _voters.modify(vitr, same_payer, [&](auto &v) {
                v.flags1 = set_field(v.flags1, voter_info::flags1_fields::net_managed, false);
            });
        } else {
            check(*net_weight >= -1, "invalid value for net_weight");

            auto vitr = _voters.find(account.value);
            if (vitr != _voters.end()) {
                _voters.modify(vitr, same_payer, [&](auto &v) {
                    v.flags1 = set_field(v.flags1, voter_info::flags1_fields::net_managed, true);
                });
            } else {
                _voters.emplace(account, [&](auto &v) {
                    v.owner = account;
                    v.flags1 = set_field(v.flags1, voter_info::flags1_fields::net_managed, true);
                });
            }

            net = *net_weight;
        }

        set_resource_limits(account.value, current_ram, net, current_cpu);
    }

    void eosiosystem::system_contract::setacctcpu(name account, std::optional <int64_t> cpu_weight) {
        require_auth(_self);

        int64_t current_ram, current_net, current_cpu;
        get_resource_limits(account.value, &current_ram, &current_net, &current_cpu);

        int64_t cpu = 0;

        if (!cpu_weight) {
            auto vitr = _voters.find(account.value);
            check(vitr != _voters.end() && has_field(vitr->flags1, voter_info::flags1_fields::cpu_managed),
                  "CPU bandwidth of account is already unmanaged");

            user_resources_table userres(_self, account.value);
            auto ritr = userres.find(account.value);

            if (ritr != userres.end()) {
                cpu = ritr->cpu_weight.amount;
            }

            _voters.modify(vitr, same_payer, [&](auto &v) {
                v.flags1 = set_field(v.flags1, voter_info::flags1_fields::cpu_managed, false);
            });
        } else {
            check(*cpu_weight >= -1, "invalid value for cpu_weight");

            auto vitr = _voters.find(account.value);
            if (vitr != _voters.end()) {
                _voters.modify(vitr, same_payer, [&](auto &v) {
                    v.flags1 = set_field(v.flags1, voter_info::flags1_fields::cpu_managed, true);
                });
            } else {
                _voters.emplace(account, [&](auto &v) {
                    v.owner = account;
                    v.flags1 = set_field(v.flags1, voter_info::flags1_fields::cpu_managed, true);
                });
            }

            cpu = *cpu_weight;
        }

        set_resource_limits(account.value, current_ram, current_net, cpu);
    }

    void eosiosystem::system_contract::rmvproducer(name producer) {
        require_auth(_self);
        auto prod = _producers.find(producer.value);
        check(prod != _producers.end(), "producer not found");
        _producers.modify(prod, same_payer, [&](auto &p) {
            p.deactivate();
        });
    }

    void eosiosystem::system_contract::updtrevision(uint8_t revision) {
        require_auth(_self);
        check(_gstate2.revision < 255, "can not increment revision"); // prevent wrap around
        check(revision == _gstate2.revision + 1, "can only increment revision by one");
        check(revision <= 1, // set upper bound to greatest revision supported in the code
              "specified revision is not yet supported by the code");
        _gstate2.revision = revision;
    }

    /**
     *  Called after a new account is created. This code enforces resource-limits rules
     *  for new accounts as well as new account naming conventions.
     *
     *  Account names containing '.' symbols must have a suffix equal to the name of the creator.
     *  This allows users who buy a premium name (shorter than 12 characters with no dots) to be the only ones
     *  who can create accounts with the creator's name as a suffix.
     *
     */
    void eosiosystem::native::newaccount(name creator,
                                         name newact,
                                         ignore <authority> owner,
                                         ignore <authority> active) {

        if (creator != _self) {
            uint64_t tmp = newact.value >> 4;
            bool has_dot = false;

            for (uint32_t i = 0; i < 12; ++i) {
                has_dot |= !(tmp & 0x1f);
                tmp >>= 5;
            }
            if (has_dot) { // or is less than 12 characters
                auto suffix = newact.suffix();
                if (suffix != newact) {
                    check(creator == suffix, "only suffix may create this account");
                }
            }
        }


       //in the FIO protocol we want all of our accounts to be created with unlimited
       //CPU NET and RAM. to do this we need our accounts to NOT have entrees in the
       //resources table. our accounts will all be unlimited CPU NET and RAM for the
       //foreseeable future of the FIO protocol.
       // user_resources_table userres(_self, newact.value);

       // userres.emplace(newact, [&](auto &res) {
       //     res.owner = newact;
           // res.net_weight = asset(0, system_contract::get_core_symbol());
           // res.cpu_weight = asset(0, system_contract::get_core_symbol());
       // });

       // set_resource_limits(newact.value, 0, 0, 0);
    }

    void eosiosystem::native::setabi(name acnt, const std::vector<char> &abi) {
        eosio::multi_index<"abihash"_n, abi_hash> table(_self, _self.value);
        auto itr = table.find(acnt.value);
        if (itr == table.end()) {
            table.emplace(acnt, [&](auto &row) {
                row.owner = acnt;
                sha256(const_cast<char *>(abi.data()), abi.size(), &row.hash);
            });
        } else {
            table.modify(itr, same_payer, [&](auto &row) {
                sha256(const_cast<char *>(abi.data()), abi.size(), &row.hash);
            });
        }
    }

    void eosiosystem::system_contract::init(unsigned_int version, symbol core) {
        require_auth(_self);
        check(version.value == 0, "unsupported version for init action");
    }

} /// fio.system


EOSIO_DISPATCH( eosiosystem::system_contract,
// native.hpp (newaccount definition is actually in fio.system.cpp)
(newaccount)(updateauth)(deleteauth)(linkauth)(unlinkauth)(canceldelay)(onerror)(setabi)
        // fio.system.cpp
        (init)(setparams)(setpriv)(setalimits)(setacctram)(setacctnet)(setacctcpu)
        (rmvproducer)(updtrevision)
        // delegate_bandwidth.cpp
        (updatepower)(refund)
        // voting.cpp
        (regproducer)(regiproducer)(unregprod)(vproducer)(voteproducer)(voteproxy)(setautoproxy)(crautoproxy)(
        unregproxy)(regiproxy)(regproxy)
        // producer_pay.cpp
        (onblock)
(claimrewards)(resetclaim)
)
