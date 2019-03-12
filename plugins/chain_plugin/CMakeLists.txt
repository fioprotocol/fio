file(GLOB HEADERS "include/eosio/chain_plugin/*.hpp")
add_library( chain_plugin
             chain_plugin.cpp
             ${HEADERS} )

add_dependencies(chain_plugin eosio.system)

target_link_libraries( chain_plugin eosio_chain appbase )
target_include_directories( chain_plugin PUBLIC ${CMAKE_BINARY_DIR}/contracts "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/../chain_interface/include" "${CMAKE_CURRENT_SOURCE_DIR}/../../libraries/appbase/include")