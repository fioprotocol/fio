add_contract(fio.address fio.address ${CMAKE_CURRENT_SOURCE_DIR}/fio.address.cpp)

target_include_directories(fio.address
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../fio.address/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../
        )


set_target_properties(fio.address
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
