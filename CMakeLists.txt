# cmake-format: off
# /CMakeLists.txt -*-makefile-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# cmake-format: on

cmake_minimum_required(VERSION 3.30...4.3)

project(
    beman.redis
    VERSION 0.1.0
    DESCRIPTION "Experimental sender/receiver Redis client"
    LANGUAGES CXX
)

option(
    BEMAN_REDIS_BUILD_TESTS
    "Enable building tests and test infrastructure. Default: ${PROJECT_IS_TOP_LEVEL}. Values: { ON, OFF }."
    ${PROJECT_IS_TOP_LEVEL}
)

option(
    BEMAN_REDIS_BUILD_EXAMPLES
    "Enable building examples. Default: ${PROJECT_IS_TOP_LEVEL}. Values: { ON, OFF }."
    ${PROJECT_IS_TOP_LEVEL}
)

option(
    BEMAN_REDIS_INSTALL
    "Enable install rules. Default: ${PROJECT_IS_TOP_LEVEL}. Values: { ON, OFF }."
    ${PROJECT_IS_TOP_LEVEL}
)

option(
    BEMAN_REDIS_USE_LOCAL_BEMAN_DEPS
    "Use sibling Beman source checkouts for local development instead of installed packages. Values: { ON, OFF }."
    OFF
)

set(
    BEMAN_REDIS_LOCAL_BEMAN_ROOT
    "${PROJECT_SOURCE_DIR}/.."
    CACHE PATH "Directory containing sibling Beman dependency repositories."
)

if(BEMAN_REDIS_USE_LOCAL_BEMAN_DEPS)
    include(FetchContent)

    set(
        FETCHCONTENT_SOURCE_DIR_EXECUTION
        "${BEMAN_REDIS_LOCAL_BEMAN_ROOT}/execution"
        CACHE PATH "Local beman.execution source directory."
        FORCE
    )
    set(
        FETCHCONTENT_SOURCE_DIR_TASK
        "${BEMAN_REDIS_LOCAL_BEMAN_ROOT}/task"
        CACHE PATH "Local beman.task source directory."
        FORCE
    )

    if(NOT EXISTS "${FETCHCONTENT_SOURCE_DIR_EXECUTION}/CMakeLists.txt")
        message(FATAL_ERROR "BEMAN_REDIS_USE_LOCAL_BEMAN_DEPS requires ${FETCHCONTENT_SOURCE_DIR_EXECUTION}")
    endif()
    if(NOT EXISTS "${FETCHCONTENT_SOURCE_DIR_TASK}/CMakeLists.txt")
        message(FATAL_ERROR "BEMAN_REDIS_USE_LOCAL_BEMAN_DEPS requires ${FETCHCONTENT_SOURCE_DIR_TASK}")
    endif()
    if(NOT EXISTS "${BEMAN_REDIS_LOCAL_BEMAN_ROOT}/net/CMakeLists.txt")
        message(FATAL_ERROR "BEMAN_REDIS_USE_LOCAL_BEMAN_DEPS requires ${BEMAN_REDIS_LOCAL_BEMAN_ROOT}/net")
    endif()

    add_subdirectory("${BEMAN_REDIS_LOCAL_BEMAN_ROOT}/net" "${CMAKE_BINARY_DIR}/_deps/beman.net")
endif()

if(NOT TARGET beman::execution AND NOT TARGET beman::execution_headers)
    find_package(beman.execution REQUIRED)
endif()

if(NOT TARGET beman::net AND NOT TARGET beman::net_headers)
    find_package(beman.net REQUIRED)
endif()

find_package(beman.task QUIET)

add_subdirectory(src/beman/redis)

enable_testing()

if(BEMAN_REDIS_BUILD_TESTS)
    add_subdirectory(tests/beman/redis)
endif()

if(BEMAN_REDIS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(BEMAN_REDIS_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    install(
        TARGETS beman.redis
        EXPORT beman.redisTargets
        FILE_SET public_headers
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(
        EXPORT beman.redisTargets
        NAMESPACE beman::
        FILE beman.redisTargets.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/beman.redis
    )

    configure_package_config_file(
        cmake/beman_redisConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/beman.redisConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/beman.redis
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/beman.redisConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/beman.redisConfig.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/beman.redisConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/beman.redis
    )
endif()
