cmake_minimum_required(VERSION 3.10.2)
project(state_machine VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(COMMON_CXX_OPTIONS
    -Werror
    -Wall
    -Wextra
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Wunused
    -Woverloaded-virtual
    -Wpedantic
    -Wconversion
    -Wsign-conversion
    -Wnull-dereference
    -Wdouble-promotion
    -Wformat=2
)

set(CLANG_CXX_OPTIONS
    ${COMMON_CXX_OPTIONS}
)

set(GCC_CXX_OPTIONS
    ${COMMON_CXX_OPTIONS}
    -Wduplicated-cond
    -Wduplicated-branches
    -Wlogical-op
    -Wuseless-cast
    -Wmisleading-indentation
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(DEFAULT_CXX_OPTIONS ${CLANG_CXX_OPTIONS})
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(DEFAULT_CXX_OPTIONS ${GCC_CXX_OPTIONS})
endif()


option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

add_executable(example
    examples/main.cc
)

target_include_directories(example PUBLIC include)
target_compile_options(example PRIVATE ${DEFAULT_CXX_OPTIONS})
