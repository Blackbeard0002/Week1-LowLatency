cmake_minimum_required(VERSION 3.16)

project(csot_week1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CheckIPOSupported)
check_ipo_supported(RESULT CSOT_IPO_SUPPORTED OUTPUT CSOT_IPO_ERROR)
if(CSOT_IPO_SUPPORTED)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
else()
    message(STATUS "IPO/LTO not supported: ${CSOT_IPO_ERROR}")
endif()

add_executable(quant_runner
    src/main.cpp
    src/engine.cpp
    strategies/spec_strategy.cpp
)

target_include_directories(quant_runner PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(quant_runner PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        $<$<CONFIG:Release>:-O3;-DNDEBUG>
        $<$<CONFIG:RelWithDebInfo>:-O3;-DNDEBUG>
    )
endif()

add_library(null_strategy SHARED
    strategies/null_strategy.cpp
)

target_include_directories(null_strategy PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(null_strategy PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        $<$<CONFIG:Release>:-O3;-DNDEBUG>
        $<$<CONFIG:RelWithDebInfo>:-O3;-DNDEBUG>
    )
endif()

add_library(spec_strategy SHARED
    strategies/spec_strategy.cpp
)

target_include_directories(spec_strategy PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(spec_strategy PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        $<$<CONFIG:Release>:-O3;-DNDEBUG;-fno-exceptions;-fno-rtti>
        $<$<CONFIG:RelWithDebInfo>:-O3;-DNDEBUG;-fno-exceptions;-fno-rtti>
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench/bench.cpp")
    find_package(benchmark QUIET)
    if(benchmark_FOUND)
        add_executable(bench_runner
            bench/bench.cpp
            src/engine.cpp
            strategies/spec_strategy.cpp
        )

        target_include_directories(bench_runner PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
        )

        target_link_libraries(bench_runner PRIVATE
            benchmark::benchmark
        )

        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(bench_runner PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                $<$<CONFIG:Release>:-O3;-DNDEBUG>
                $<$<CONFIG:RelWithDebInfo>:-O3;-DNDEBUG>
            )
        endif()
    else()
        message(STATUS "Google Benchmark not found; bench_runner target disabled.")
    endif()
else()
    message(STATUS "bench/bench.cpp not found; bench_runner target disabled.")
endif()

find_program(CLANG_FORMAT_EXE NAMES clang-format)
if(CLANG_FORMAT_EXE)
    add_custom_target(format
        COMMAND "${CLANG_FORMAT_EXE}" -i
                "${CMAKE_CURRENT_SOURCE_DIR}/include/strategy.hpp"
                "${CMAKE_CURRENT_SOURCE_DIR}/include/engine.hpp"
                "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp"
                "${CMAKE_CURRENT_SOURCE_DIR}/src/engine.cpp"
                "${CMAKE_CURRENT_SOURCE_DIR}/bench/bench.cpp"
                "${CMAKE_CURRENT_SOURCE_DIR}/strategies/null_strategy.cpp"
                "${CMAKE_CURRENT_SOURCE_DIR}/strategies/spec_strategy.cpp"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Formatting source files with clang-format"
    )
else()
    message(STATUS "clang-format not found; 'format' target is unavailable.")
endif()