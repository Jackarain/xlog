//
// test_init.cpp
// ~~~~~~~~~~~~~
//
// init_logging(path) 指定日志目录. 注意: 本进程内的第一次日志调用会
// 创建唯一的 writer 单例, 因此本文件只包含一个用例, 且必须最先调用
// init_logging.
//

#include <xlog/logging.hpp>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>

TEST(init, custom_log_directory)
{
	namespace fs = std::filesystem;
	std::error_code ec;

	const fs::path dir = "custom_logs";
	fs::remove_all(dir, ec);

	xlogger::toggle_console_logging(false);
	xlogger::turnon_logging();
	xlogger::toggle_write_logging(true);
	xlogger::init_logging(dir.string());

	const fs::path expected = dir / (std::string(LOG_APPNAME) + ".log");
	EXPECT_EQ(expected.string(), xlogger::log_path());

	XLOG_INFO << "custom-dir-marker";

	ASSERT_TRUE(fs::exists(expected, ec));
	std::ifstream ifs(expected, std::ios::binary);
	const std::string content{
		std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
	EXPECT_NE(std::string::npos, content.find("custom-dir-marker"));

	// 默认的 ./logs 目录不应该被创建.
	EXPECT_FALSE(fs::exists("./logs", ec));
}
