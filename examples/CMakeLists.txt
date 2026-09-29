add_executable(xlog_example_basic basic.cpp)
add_executable(xlog_example_file_logging file_logging.cpp)
add_executable(xlog_example_async async.cpp)
add_executable(xlog_example_hook hook.cpp)
add_executable(xlog_example_disable disable.cpp)

target_compile_definitions(xlog_example_disable PRIVATE DISABLE_XLOGGER)

set(XLOG_EXAMPLES
    xlog_example_basic
    xlog_example_file_logging
    xlog_example_async
    xlog_example_hook
    xlog_example_disable)

if(TARGET xlog_pedantic)
    set(XLOG_EXAMPLE_WARNINGS xlog_pedantic)
endif()

foreach(example IN LISTS XLOG_EXAMPLES)
    target_link_libraries(${example}
        PRIVATE xlog::xlog ${XLOG_EXAMPLE_WARNINGS} xlog_internal_options)
endforeach()
