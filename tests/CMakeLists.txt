# ---------------------------------------------------------------------------
# Test suite for xlog
# ---------------------------------------------------------------------------

find_package(GTest QUIET)

if(GTest_FOUND)
    set(XLOG_GTEST_MAIN GTest::gtest_main)
else()
    message(STATUS "GTest not found, fetching via FetchContent...")
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(googletest)
    set(XLOG_GTEST_MAIN gtest_main)
endif()

if(TARGET xlog_pedantic)
    set(XLOG_TEST_WARNINGS xlog_pedantic)
endif()

set(XLOG_TESTS
    test_level
    test_format
    test_hook
    test_threads
    test_utf
    test_file
    test_init
    test_rotate
    test_console)

if(ZLIB_FOUND AND XLOG_ENABLE_COMPRESSION)
    list(APPEND XLOG_TESTS test_compress)
endif()

foreach(test_name IN LISTS XLOG_TESTS)
    add_executable(${test_name} ${test_name}.cpp test_main.cpp)
    target_link_libraries(${test_name}
        PRIVATE xlog::xlog ${XLOG_GTEST_MAIN} ${XLOG_TEST_WARNINGS})
    if(NOT MSVC)
        target_link_libraries(${test_name} PRIVATE pthread)
    endif()
    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES TIMEOUT 300)
endforeach()

# ---------------------------------------------------------------------------
# -fno-exceptions / -fno-rtti verification target.
#
# test_no_exceptions deliberately does NOT use gtest. Building and running it
# proves that the header is usable without exceptions or RTTI.
# ---------------------------------------------------------------------------
add_executable(test_no_exceptions test_no_exceptions.cpp)
target_link_libraries(test_no_exceptions
    PRIVATE xlog::xlog ${XLOG_TEST_WARNINGS})
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(test_no_exceptions PRIVATE -fno-exceptions -fno-rtti)
elseif(MSVC)
    target_compile_options(test_no_exceptions PRIVATE /EHs-c- /GR-)
endif()
add_test(NAME test_no_exceptions COMMAND test_no_exceptions)
set_tests_properties(test_no_exceptions PROPERTIES TIMEOUT 120)

# ---------------------------------------------------------------------------
# DISABLE_XLOGGER verification target: every logging macro must collapse to a
# no-op and the header must not create any output.
# ---------------------------------------------------------------------------
add_executable(test_disabled test_disabled.cpp)
target_link_libraries(test_disabled PRIVATE xlog::xlog ${XLOG_TEST_WARNINGS})
target_compile_definitions(test_disabled PRIVATE DISABLE_XLOGGER)
add_test(NAME test_disabled COMMAND test_disabled)
set_tests_properties(test_disabled PROPERTIES TIMEOUT 120)
