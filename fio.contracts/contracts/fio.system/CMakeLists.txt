add_contract(fio.system fio.system ${CMAKE_CURRENT_SOURCE_DIR}/src/fio.system.cpp)

target_include_directories(fio.system
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../
        ${CMAKE_CURRENT_SOURCE_DIR}/../fio.token/include
        )

set_target_properties(fio.system
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
