add_contract(fio.system fio.name ${CMAKE_CURRENT_SOURCE_DIR}/fio.name.cpp)

target_include_directories(fio.name
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}
   ${CMAKE_CURRENT_SOURCE_DIR}/../eosio.system/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../
   )


set_target_properties(fio.name
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
