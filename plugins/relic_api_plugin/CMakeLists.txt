file(GLOB HEADERS "include/eosio/relic_api_plugin/*.hpp")
add_library(relic_api_plugin
        relic_api_plugin.cpp
        ${HEADERS})

target_link_libraries(relic_api_plugin relic_plugin chain_plugin http_plugin appbase)
target_include_directories(relic_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
