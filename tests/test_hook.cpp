//
// test_hook.cpp
// ~~~~~~~~~~~~~
//
// 用户钩子(tag_invoke): 日志截获, 时间/级别字段, 中断与吞掉开关.
//

#include "capture.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>

namespace {

	std::string read_file(const std::filesystem::path& path)
	{
		std::ifstream ifs(path, std::ios::binary);
		if (!ifs)
			return {};
		return std::string(std::istreambuf_iterator<char>(ifs),
			std::istreambuf_iterator<char>());
	}
}

class hook_test : public ::testing::Test
{
	protected:
	void SetUp() override
	{
		xlog_test::clear();
		xlog_test::swallow_logs() = false;
		xlog_test::abort_flag() = false;
		xlogger::toggle_write_logging(true);
		xlogger::toggle_console_logging(false);
		xlogger::turnon_logging();
		xlogger::set_log_level(xlogger::_logger_debug_id__);
	}

	void TearDown() override
	{
		xlog_test::swallow_logs() = true;
		xlog_test::abort_flag() = false;
	}
};

TEST_F(hook_test, hook_receives_time_and_level)
{
	const auto before = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch())
							.count();

	XLOG_WARN << "hook message";

	auto logs = xlog_test::captured();
	ASSERT_EQ(1u, logs.size());
	EXPECT_EQ(std::string("hook message"), logs[0].message);
	EXPECT_EQ(static_cast<int>(xlogger::_logger_warn_id__), logs[0].level);
	EXPECT_GE(logs[0].time, before);
}

TEST_F(hook_test, abort_tag_stops_writing)
{
	const std::filesystem::path path = xlogger::log_path();

	xlog_test::abort_flag() = true;
	XLOG_INFO << "aborted-marker";

	// 钩子先于中断检查执行, 所以消息被截获, 但不会落盘.
	ASSERT_EQ(1u, xlog_test::count());
	EXPECT_EQ(std::string::npos, read_file(path).find("aborted-marker"));

	xlog_test::abort_flag() = false;
	XLOG_INFO << "written-marker";

	const std::string content = read_file(path);
	EXPECT_NE(std::string::npos, content.find("written-marker"));
}

TEST_F(hook_test, hook_returning_true_swallows_output)
{
	const std::filesystem::path path = xlogger::log_path();

	xlog_test::swallow_logs() = true;
	XLOG_INFO << "swallowed-marker";

	ASSERT_EQ(1u, xlog_test::count());
	EXPECT_EQ(std::string::npos, read_file(path).find("swallowed-marker"));
}
