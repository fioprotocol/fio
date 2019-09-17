add_contract(fio.tpid fio.tpid ${CMAKE_CURRENT_SOURCE_DIR}/fio.tpid.cpp)

target_include_directories(fio.tpid
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../fio.system/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../
        )


set_target_properties(fio.tpid
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
