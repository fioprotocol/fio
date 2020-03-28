cmake_minimum_required(VERSION 3.5)
project(contracttests)
set(EOSIO_VERSION_MIN "0.9")
set(EOSIO_VERSION_SOFT_MAX "1.7")
#set(EOSIO_VERSION_HARD_MAX "")

enable_testing()

configure_file(${CMAKE_SOURCE_DIR}/contracts.hpp.in ${CMAKE_BINARY_DIR}/contracts.hpp)

include_directories(${CMAKE_BINARY_DIR})

file(GLOB UNIT_TESTS "*.cpp" "*.hpp")

#add_eosio_test(unit_test ${UNIT_TESTS})
