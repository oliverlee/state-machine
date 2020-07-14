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
    -Wdouble-promotion
    -Wformat=2
)

set(CLANG_CXX_OPTIONS
    ${COMMON_CXX_OPTIONS}
)

set(GCC_CXX_OPTIONS
    ${COMMON_CXX_OPTIONS}
    -Wlogical-op
    -Wuseless-cast
)


if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(DEFAULT_CXX_OPTIONS ${CLANG_CXX_OPTIONS} -Wnull-dereference)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
        set(GCC_CXX_OPTIONS ${GCC_CXX_OPTIONS} -Wno-shadow)
    endif()
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 6)
        set(GCC_CXX_OPTIONS ${GCC_CXX_OPTIONS}
            -Wduplicated-cond
            -Wduplicated-branches
            -Wmisleading-indentation)
    endif()
    set(DEFAULT_CXX_OPTIONS ${GCC_CXX_OPTIONS})
endif()


option(BUILD_TESTS "Build tests" ON)
option(BUILD_COMPILE_TESTS "Build compile tests" ON)
if(BUILD_TESTS)
    enable_testing()
endif()

option(BUILD_COVERAGE "Build coverage" OFF)
if (BUILD_COVERAGE)
  include(cmake/CodeCoverage.cmake)

  append_coverage_compiler_flags()

  setup_target_for_coverage_lcov(
    NAME coverage
    EXECUTABLE ctest --output-on-failure -j ${n_cores} || true
    EXCLUDE
        "${PROJECT_SOURCE_DIR}/extern/*"
        "${PROJECT_SOURCE_DIR}/tests/*"
        "/Applications/*"
        "/usr/local/Cellar/*"
        "/usr/include/*"
    DEPENDENCIES
        build-unit-tests
  )

  # Note: Preventing constructor elision results in some tests failing due to
  # counting the number of constructor/destructor calls.
  set(DEFAULT_CXX_OPTIONS ${DEFAULT_CXX_OPTIONS}
        -fno-inline
        -fno-inline-small-functions
        -fno-default-inline
        -fno-elide-constructors
        -fkeep-inline-functions
  )
endif()

add_subdirectory(tests)

option(BUILD_EXAMPLES "Build examples" ON)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
