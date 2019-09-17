add_contract(fio.system fio.request.obt ${CMAKE_CURRENT_SOURCE_DIR}/fio.request.obt.cpp)

target_include_directories(fio.request.obt
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../fio.system/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../
        )


set_target_properties(fio.request.obt
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
