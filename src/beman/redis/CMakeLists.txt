# cmake-format: off
# src/beman/redis/CMakeLists.txt -*-makefile-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# cmake-format: on

add_library(beman.redis)
add_library(beman::redis ALIAS beman.redis)

target_sources(
    beman.redis
    PRIVATE
        connection.cpp
        request.cpp
        resp_encode.cpp
        resp_parse.cpp
        response.cpp
        transport.cpp
    PUBLIC
        FILE_SET public_headers
            TYPE HEADERS
            BASE_DIRS ${PROJECT_SOURCE_DIR}/include
            FILES
                ${PROJECT_SOURCE_DIR}/include/beman/redis/config.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/redis/connect.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/redis/connection.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/redis/error.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/redis/exec.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/redis/redis.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/redis/request.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/redis/response.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/redis/detail/pending_queue.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/redis/detail/resp_encode.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/redis/detail/resp_parse.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/redis/detail/sender.hpp
                ${PROJECT_SOURCE_DIR}/include/beman/redis/detail/transport.hpp
)

target_include_directories(
    beman.redis
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(beman.redis PUBLIC cxx_std_23)

set_target_properties(
    beman.redis
    PROPERTIES
        EXPORT_NAME redis
        VERIFY_INTERFACE_HEADER_SETS OFF
)

if(TARGET beman::execution)
    target_link_libraries(beman.redis PUBLIC beman::execution)
elseif(TARGET beman::execution_headers)
    target_link_libraries(beman.redis PUBLIC beman::execution_headers)
else()
    message(FATAL_ERROR "beman.execution was found, but it did not define beman::execution or beman::execution_headers")
endif()

if(TARGET beman::net)
    target_link_libraries(beman.redis PUBLIC beman::net)
elseif(TARGET beman::net_headers)
    target_link_libraries(beman.redis PUBLIC beman::net_headers)
else()
    message(FATAL_ERROR "beman.net was found, but it did not define beman::net or beman::net_headers")
endif()
