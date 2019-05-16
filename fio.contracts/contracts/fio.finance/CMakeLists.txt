add_contract(fio.system fio.finance ${CMAKE_CURRENT_SOURCE_DIR}/fio.finance.cpp)

target_include_directories(fio.finance
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../eosio.system/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../
        )


set_target_properties(fio.finance
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")