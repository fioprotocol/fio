file(GLOB HEADERS "include/eosio/relic_plugin/*.hpp")

add_library(relic_plugin
        relic_plugin.cpp
        ${HEADERS})

target_link_libraries(relic_plugin producer_plugin chain_plugin http_client_plugin appbase eosio_chain)
target_include_directories(relic_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
