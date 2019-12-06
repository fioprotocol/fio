add_contract(fio.token fio.token ${CMAKE_CURRENT_SOURCE_DIR}/src/fio.token.cpp)

target_include_directories(fio.token
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../fio.system/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../
        )

set_target_properties(fio.token
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
