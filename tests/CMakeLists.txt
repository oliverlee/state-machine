add_subdirectory("${PROJECT_SOURCE_DIR}/extern/googletest" "extern/googletest")

macro(package_add_test testname)
    add_executable(${testname} ${ARGN})
    target_link_libraries(${testname}
        gtest
        gtest_main)
    add_test(NAME ${testname}
        COMMAND ${testname})
    set_target_properties(${testname}
        PROPERTIES FOLDER tests)
    target_include_directories(${testname}
        PUBLIC ${PROJECT_SOURCE_DIR}/include)
    target_compile_options(${testname}
        PRIVATE ${DEFAULT_CXX_OPTIONS})
endmacro()

function(expect_compile_failure file)
    try_compile(COMPILE_RESULT
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/${file}
        COMPILE_DEFINITIONS ${DEFAULT_CXX_OPTIONS} -I${PROJECT_SOURCE_DIR}/include
        OUTPUT_VARIABLE COMPILE_OUTPUT
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF)

    if (COMPILE_RESULT)
        message(FATAL_ERROR " ${file} compiled successfully when it should not have.")
    else()
        message(INFO "${COMPILE_OUTPUT}")
    endif()
endfunction()


# unit tests
package_add_test(test_backport
    test_backport.cc)

package_add_test(test_traits
    test_traits.cc)

package_add_test(test_containers
    test_containers.cc)

package_add_test(test_variant
    test_variant.cc)

# compilation tests
expect_compile_failure(failure_surjection_duplicate_keys.cc)
expect_compile_failure(failure_surjection_entries_not_pairs.cc)
expect_compile_failure(failure_bijection_duplicate_values.cc)
