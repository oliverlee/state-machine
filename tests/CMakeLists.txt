add_subdirectory("${PROJECT_SOURCE_DIR}/extern/googletest" "extern/googletest")

macro(package_add_test TESTNAME)
    add_executable(${TESTNAME} ${ARGN})
    target_link_libraries(${TESTNAME}
        gtest
        gtest_main)
    add_test(NAME ${TESTNAME}
        COMMAND ${TESTNAME})
    set_target_properties(${TESTNAME}
        PROPERTIES FOLDER tests)
    target_include_directories(${TESTNAME}
        PUBLIC ${PROJECT_SOURCE_DIR}/include)
    target_compile_options(${TESTNAME}
        PRIVATE ${DEFAULT_CXX_OPTIONS})
endmacro()

package_add_test(test_backport
    test_backport.cc)

package_add_test(test_traits
    test_traits.cc)

package_add_test(test_containers
    test_containers.cc)
