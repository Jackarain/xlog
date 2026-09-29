cmake_minimum_required(VERSION 3.16)

project(xlog
    VERSION 1.0.0
    DESCRIPTION "A feature-rich, header-only C++20 logging library"
    HOMEPAGE_URL "https://github.com/Jackarain/xlog"
    LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(XLOG_BUILD_TESTS      "Build the xlog test suite"                     ON)
option(XLOG_BUILD_EXAMPLES   "Build the xlog examples"                       ON)
option(XLOG_INSTALL          "Generate install target"                       ON)
option(XLOG_PEDANTIC         "Enable extra warnings"                         ON)
option(XLOG_USE_FMT          "Use {fmt} as the formatting backend"           OFF)
option(XLOG_ENABLE_BOOST     "Use Boost for filesystem/asio/date_time"       ON)
option(XLOG_ENABLE_COMPRESSION "Compress rotated log files with zlib"        ON)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(XLOG_IS_TOP_LEVEL ON)
else()
    set(XLOG_IS_TOP_LEVEL OFF)
endif()

# ---------------------------------------------------------------------------
# The header-only library target
# ---------------------------------------------------------------------------
add_library(xlog INTERFACE)
add_library(xlog::xlog ALIAS xlog)

# SYSTEM: 头文件对使用方而言是第三方代码, 其告警不应污染使用方构建
# (头文件自身在 CI 中单独做告警检查).
target_include_directories(xlog SYSTEM INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_compile_features(xlog INTERFACE cxx_std_20)

# std::thread / std::mutex are used by the asynchronous writer.
find_package(Threads REQUIRED)
target_link_libraries(xlog INTERFACE Threads::Threads)

# ---------------------------------------------------------------------------
# Optional dependencies.
#
# xlog is header-only and detects everything it needs with __has_include,
# but options that pull in a compiled component still need to be linked:
#   * {fmt}          - when the toolchain has no std::format (C++20 <format>)
#   * Boost.Filesystem - when <boost/filesystem.hpp> is used
#   * zlib           - when rotated log files are compressed
# ---------------------------------------------------------------------------
if(XLOG_USE_FMT)
    find_package(fmt 8.0 REQUIRED)
    target_link_libraries(xlog INTERFACE fmt::fmt)
    target_compile_definitions(xlog INTERFACE FORCE_USE_FMT_FORMAT)
else()
    find_package(fmt QUIET)
    if(fmt_FOUND)
        # Some toolchains (Android NDK, libc++ < 17) have no std::format;
        # the header falls back to {fmt} on its own, link it here so that
        # fallback is always resolvable.
        target_link_libraries(xlog INTERFACE fmt::fmt)
        message(STATUS "xlog: {fmt} found, also available as a fallback backend")
    endif()
endif()

# 头文件里 Boost 的可选集成靠 __has_include 探测, 但 boost::filesystem
# 需要链接库文件. 这里由 CMake 显式决定: 找得到就链接, 找不到就关掉该
# 集成, 避免"头文件用了 boost::filesystem 却没链接"的错配. 其余 Boost
# 集成(asio endpoint / posix_time / string_view)只有头文件依赖, 保持由
# 头文件自行探测.
set(XLOG_BOOST_FILESYSTEM_DEFINES LOGGING_DISABLE_BOOST_FILESYSTEM)
set(XLOG_BOOST_ALL_DEFINES
    ${XLOG_BOOST_FILESYSTEM_DEFINES}
    LOGGING_DISABLE_BOOST_ASIO_ENDPOINT
    LOGGING_DISABLE_BOOST_POSIX_TIME
    LOGGING_DISABLE_BOOST_STRING_VIEW)

if(XLOG_ENABLE_BOOST)
    find_package(Boost CONFIG QUIET COMPONENTS filesystem system)
    if(Boost_FOUND)
        target_link_libraries(xlog INTERFACE Boost::filesystem Boost::system)
        message(STATUS "xlog: Boost ${Boost_VERSION} found, using boost::filesystem")
    else()
        message(STATUS "xlog: Boost not found, falling back to std::filesystem")
        target_compile_definitions(xlog INTERFACE
            ${XLOG_BOOST_FILESYSTEM_DEFINES})
    endif()
else()
    target_compile_definitions(xlog INTERFACE ${XLOG_BOOST_ALL_DEFINES})
endif()

if(XLOG_ENABLE_COMPRESSION)
    find_package(ZLIB QUIET)
    if(ZLIB_FOUND)
        target_link_libraries(xlog INTERFACE ZLIB::ZLIB)
    else()
        message(STATUS "xlog: zlib not found, log compression disabled")
        target_compile_definitions(xlog INTERFACE LOGGING_DISABLE_COMPRESS_LOGS)
    endif()
else()
    target_compile_definitions(xlog INTERFACE LOGGING_DISABLE_COMPRESS_LOGS)
endif()

# 本仓库源码统一为 UTF-8 (with BOM). MSVC 默认把窄字符串字面量按当前 ANSI
# 代码页转换, 非 ASCII 字符(如中文)会退化成 '?', 与其他平台的 UTF-8 行为
# 不一致. 这里只作用于本仓库自身的测试/示例, 不作为接口选项导出给使用方.
if(XLOG_BUILD_TESTS OR XLOG_BUILD_EXAMPLES)
    add_library(xlog_internal_options INTERFACE)
    target_compile_options(xlog_internal_options INTERFACE
        $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
endif()

# 额外的告警只作用于本仓库自己的目标(测试/示例), 不导出给使用方.
if(XLOG_PEDANTIC AND (XLOG_BUILD_TESTS OR XLOG_BUILD_EXAMPLES))
    add_library(xlog_pedantic INTERFACE)
    target_compile_options(xlog_pedantic INTERFACE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>)
endif()

# ---------------------------------------------------------------------------
# Install rules (headers + CMake package config + pkg-config)
# ---------------------------------------------------------------------------
if(XLOG_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    install(TARGETS xlog EXPORT xlogTargets)
    install(EXPORT xlogTargets
        FILE xlogTargets.cmake
        NAMESPACE xlog::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xlog)

    # Dependencies referenced by the exported INTERFACE target must be
    # re-discovered by the consumer's find_package(xlog) call.
    set(XLOG_CONFIG_DEPENDENCIES "find_dependency(Threads)")
    if(fmt_FOUND OR XLOG_USE_FMT)
        string(APPEND XLOG_CONFIG_DEPENDENCIES "\nfind_dependency(fmt)")
    endif()
    if(Boost_FOUND AND XLOG_ENABLE_BOOST)
        string(APPEND XLOG_CONFIG_DEPENDENCIES
            "\nfind_dependency(Boost CONFIG COMPONENTS filesystem system)")
    endif()
    if(ZLIB_FOUND AND XLOG_ENABLE_COMPRESSION)
        string(APPEND XLOG_CONFIG_DEPENDENCIES "\nfind_dependency(ZLIB)")
    endif()

    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/xlogConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/xlogConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xlog)

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/xlogConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)

    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/xlogConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/xlogConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xlog)

    # pkg-config equivalent of the INTERFACE link libraries above.
    set(XLOG_PC_LIBS "")
    set(XLOG_PC_LIBS_PRIVATE "")
    if(fmt_FOUND OR XLOG_USE_FMT)
        string(APPEND XLOG_PC_LIBS " -lfmt")
    endif()
    if(Boost_FOUND AND XLOG_ENABLE_BOOST)
        string(APPEND XLOG_PC_LIBS " -lboost_filesystem -lboost_system")
    endif()
    if(ZLIB_FOUND AND XLOG_ENABLE_COMPRESSION)
        string(APPEND XLOG_PC_LIBS " -lz")
    endif()
    if(NOT WIN32)
        string(APPEND XLOG_PC_LIBS_PRIVATE " -lpthread")
    endif()
    string(STRIP "${XLOG_PC_LIBS}" XLOG_PC_LIBS)

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/xlog.pc.in
        ${CMAKE_CURRENT_BINARY_DIR}/xlog.pc @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/xlog.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif()

# ---------------------------------------------------------------------------
# Tests & examples
# ---------------------------------------------------------------------------
if(XLOG_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(XLOG_BUILD_EXAMPLES AND XLOG_IS_TOP_LEVEL)
    add_subdirectory(examples)
endif()
