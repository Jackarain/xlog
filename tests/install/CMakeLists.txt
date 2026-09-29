cmake_minimum_required(VERSION 3.16)

project(xlog_install_consumer CXX)

find_package(xlog REQUIRED)

add_executable(xlog_install_consumer consumer.cpp)
target_link_libraries(xlog_install_consumer PRIVATE xlog::xlog)
