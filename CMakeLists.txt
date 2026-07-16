cmake_minimum_required(VERSION 3.14)
project(FastCron
    VERSION 1.0.0
    DESCRIPTION "Ultra-fast, no-malloc, bitmask-based cron for Deep Sleep"
    LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(FASTCRON_BUILD_TESTS "Build unit tests" ON)
option(FASTCRON_BUILD_DOCS  "Build documentation" OFF)

# ---------------------------------------------------------------------------
# Core Library
# ---------------------------------------------------------------------------
add_library(fastcron STATIC
    src/fastcron.c
)

target_include_directories(fastcron
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_options(fastcron PRIVATE
    $<$<C_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic -Werror>
    $<$<C_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic -Werror>
    $<$<C_COMPILER_ID:MSVC>:/W4 /WX>
)

# ---------------------------------------------------------------------------
# Tests (Unity Framework via FetchContent)
# ---------------------------------------------------------------------------
if(FASTCRON_BUILD_TESTS)
    enable_testing()
    include(FetchContent)

    FetchContent_Declare(
        unity
        GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
        GIT_TAG        v2.6.0
    )
    FetchContent_MakeAvailable(unity)

    add_executable(fastcron_tests
        tests/test_fastcron.c
    )

    target_link_libraries(fastcron_tests PRIVATE fastcron unity)

    target_include_directories(fastcron_tests PRIVATE
        ${unity_SOURCE_DIR}/src
    )

    add_test(NAME FastCronTests COMMAND fastcron_tests)

    add_executable(fastcron_scheduler_tests
        tests/test_fastcron_scheduler.c
    )
    target_link_libraries(fastcron_scheduler_tests PRIVATE fastcron unity)
    target_include_directories(fastcron_scheduler_tests PRIVATE
        ${unity_SOURCE_DIR}/src
    )
    add_test(NAME FastCronSchedulerTests COMMAND fastcron_scheduler_tests)

    add_executable(fastcron_bench
        benchmarks/benchmark.c
        benchmarks/third_party/ccronexpr/ccronexpr.c
    )
    target_link_libraries(fastcron_bench PRIVATE fastcron)
    target_include_directories(fastcron_bench PRIVATE benchmarks/third_party/ccronexpr)

    add_executable(fastcron_usage
        benchmarks/usage.c
        benchmarks/third_party/ccronexpr/ccronexpr.c
    )
    target_link_libraries(fastcron_usage PRIVATE fastcron)
    target_include_directories(fastcron_usage PRIVATE benchmarks/third_party/ccronexpr)

    add_custom_target(check-size
        COMMAND size $<TARGET_FILE:fastcron>
        DEPENDS fastcron
        COMMENT "Printing footprint of libfastcron.a"
    )
endif()

# ---------------------------------------------------------------------------
# Documentation (Doxygen + Sphinx + Breathe)
# ---------------------------------------------------------------------------
if(FASTCRON_BUILD_DOCS)
    find_package(Doxygen REQUIRED)

    set(DOXYGEN_GENERATE_XML YES)
    set(DOXYGEN_GENERATE_HTML NO)
    set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doxygen)

    doxygen_add_docs(doxygen_xml
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        COMMENT "Generating Doxygen XML"
    )

    add_custom_target(docs
        COMMAND sphinx-build -b html
            ${CMAKE_CURRENT_SOURCE_DIR}/docs
            ${CMAKE_CURRENT_BINARY_DIR}/docs/html
        DEPENDS doxygen_xml
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Building Sphinx HTML documentation"
    )
endif()

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
install(TARGETS fastcron
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY include/ DESTINATION include)
