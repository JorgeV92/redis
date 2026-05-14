# cmake-format: off
# tests/beman/redis/CMakeLists.txt -*-makefile-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# cmake-format: on

set(TESTS
    connection
    pending_queue
    request
    resp_parse
    response
)

foreach(TEST ${TESTS})
    set(TEST_TARGET beman.redis.tests.${TEST})
    add_executable(${TEST_TARGET})
    target_sources(${TEST_TARGET} PRIVATE ${TEST}.test.cpp)
    target_link_libraries(${TEST_TARGET} PRIVATE beman::redis)
    target_compile_features(${TEST_TARGET} PRIVATE cxx_std_20)
    add_test(NAME beman.redis.${TEST} COMMAND $<TARGET_FILE:${TEST_TARGET}>)
endforeach()
