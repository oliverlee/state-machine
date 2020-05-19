macro(add_example_executable name)
    add_executable(${name} ${ARGN})
    target_include_directories(${name} PUBLIC ${PROJECT_SOURCE_DIR}/include)
    target_compile_options(${name} PRIVATE ${DEFAULT_CXX_OPTIONS})
endmacro()

add_example_executable(simple1
    simple1/main.cc
)
