cmake_minimum_required(VERSION 3.5)

project(contracts)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

add_subdirectory(eosio.bios)
add_subdirectory(eosio.msig)
add_subdirectory(eosio.system)
add_subdirectory(fio.token)
add_subdirectory(eosio.wrap)
add_subdirectory(fio.name)
add_subdirectory(fio.fee)
add_subdirectory(fio.request.obt)
