add_contract(fio.system fio.fee ${CMAKE_CURRENT_SOURCE_DIR}/fio.fee.cpp)

target_include_directories(fio.fee
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../fio.system/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../
        )


set_target_properties(fio.fee
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
