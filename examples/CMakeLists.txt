# cmake-format: off
# examples/CMakeLists.txt -*-makefile-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# cmake-format: on

set(EXAMPLES
    coroutine_task_example
    ping
    pipeline
    set_get
)

foreach(EXAMPLE ${EXAMPLES})
    set(EXAMPLE_TARGET beman.redis.examples.${EXAMPLE})
    add_executable(${EXAMPLE_TARGET})
    target_sources(${EXAMPLE_TARGET} PRIVATE ${EXAMPLE}.cpp)
    target_link_libraries(${EXAMPLE_TARGET} PRIVATE beman::redis)
    target_compile_features(${EXAMPLE_TARGET} PRIVATE cxx_std_20)
endforeach()
