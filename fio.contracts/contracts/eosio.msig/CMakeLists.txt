add_contract(eosio.msig eosio.msig ${CMAKE_CURRENT_SOURCE_DIR}/src/eosio.msig.cpp)

target_include_directories(eosio.msig
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../fio.system/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../
        )

set_target_properties(eosio.msig
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
