cmake_minimum_required(VERSION 3.10.2)
project(state_machine VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

add_executable(example
    examples/main.cc
)

target_include_directories(example PUBLIC include)
target_compile_options(example PRIVATE -Werror -Wall -Wextra -pedantic)
