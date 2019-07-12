#include <fio.common/fio.common.hpp>
#include <fio.name/fio.name.hpp>
#include <string>
#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>

namespace fioio{
  using namespace eosio;

  // @abi table tpids i64
  struct [[eosio::action]] tpid {

    uint64_t fioaddhash;
    string fioaddress;
    uint64_t rewards;

    uint64_t primary_key() const {return fioaddhash;}

    EOSLIB_SERIALIZE(tpid, (fioaddhash)(fioaddress)(rewards))


  };

  typedef multi_index<"tpids"_n, tpid> tpids_table;
}
