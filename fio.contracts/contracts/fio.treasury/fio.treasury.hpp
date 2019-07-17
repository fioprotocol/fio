#include <fio.common/fio.common.hpp>
#include <fio.common/fio.rewards.hpp>
#include <fio.name/fio.name.hpp>
#include <fio.tpid/fio.tpid.hpp>
#include <string>
#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>

namespace fioio{
  using namespace eosio;

  // @abi table clockstate i64
  struct [[eosio::table]] treasurystate {
      uint64_t lasttpidpayout;
      uint64_t lastbppayout;

      // Set the primary key to a constant value to store only one row
      uint64_t primary_key() const { return lasttpidpayout; }
      EOSLIB_SERIALIZE(treasurystate,(lasttpidpayout)(lastbppayout))
  };
  typedef eosio::multi_index<"clockstate"_n,treasurystate> rewards_table;

}
