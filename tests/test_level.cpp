//
// test_level.cpp
// ~~~~~~~~~~~~~~
//
// 日志级别门控: 全局最低级别过滤, 总开关, 异步路径同样受限.
//

#include "capture.hpp"

#include <gtest/gtest.h>

namespace {
	// 进程启动时的默认级别(未做任何修改).
	const xlogger::logger_level__ g_initial_level = xlogger::logging_level();
}

class level_test : public ::testing::Test
{
	protected:
	void SetUp() override
	{
		xlog_test::clear();
		xlog_test::swallow_logs() = true;
		xlog_test::abort_flag() = false;
		xlogger::toggle_write_logging(false);
		xlogger::toggle_console_logging(false);
		xlogger::turnon_logging();
		xlogger::set_log_level(xlogger::_logger_debug_id__);
	}

	void TearDown() override
	{
		xlogger::set_log_level(xlogger::_logger_debug_id__);
		xlogger::turnon_logging();
		xlog_test::clear();
	}
};

TEST_F(level_test, default_level_is_debug)
{
	EXPECT_EQ(xlogger::_logger_debug_id__, g_initial_level);
}

TEST_F(level_test, debug_is_logged_by_default)
{
	XLOG_DBG << "debug message";

	ASSERT_EQ(1u, xlog_test::count());
	EXPECT_EQ(std::string("debug message"), xlog_test::messages()[0]);
}

TEST_F(level_test, lower_levels_are_filtered)
{
	xlogger::set_log_level(xlogger::_logger_info_id__);

	XLOG_DBG << "dropped";
	XLOG_INFO << "kept";
	XLOG_WARN << "kept too";

	ASSERT_EQ(2u, xlog_test::count());
	EXPECT_EQ(std::string("kept"), xlog_test::messages()[0]);
	EXPECT_EQ(std::string("kept too"), xlog_test::messages()[1]);
}

TEST_F(level_test, error_level_filters_warn)
{
	xlogger::set_log_level(xlogger::_logger_error_id__);

	XLOG_WARN << "dropped";
	XLOG_ERR << "kept";

	ASSERT_EQ(1u, xlog_test::count());
	EXPECT_EQ(std::string("kept"), xlog_test::messages()[0]);
	EXPECT_EQ(
		static_cast<int>(xlogger::_logger_error_id__), xlog_test::levels()[0]);
}

TEST_F(level_test, file_level_is_never_filtered)
{
	xlogger::set_log_level(xlogger::_logger_file_id__);

	XLOG_ERR << "dropped";
	XLOG_FILE << "kept";

	ASSERT_EQ(1u, xlog_test::count());
	EXPECT_EQ(std::string("kept"), xlog_test::messages()[0]);
}

TEST_F(level_test, format_macros_are_filtered)
{
	xlogger::set_log_level(xlogger::_logger_warn_id__);

	XLOG_FINFO("dropped {}", 1);
	XLOG_FWARN("kept {}", 2);
	XLOG_FERR("kept {}", 3);

	ASSERT_EQ(2u, xlog_test::count());
	EXPECT_EQ(std::string("kept 2"), xlog_test::messages()[0]);
	EXPECT_EQ(std::string("kept 3"), xlog_test::messages()[1]);
}

TEST_F(level_test, logging_level_roundtrip)
{
	xlogger::set_log_level(xlogger::_logger_warn_id__);
	EXPECT_EQ(xlogger::_logger_warn_id__, xlogger::logging_level());

	xlogger::set_log_level(xlogger::_logger_debug_id__);
	EXPECT_EQ(xlogger::_logger_debug_id__, xlogger::logging_level());
}

TEST_F(level_test, turnoff_logging_drops_everything)
{
	xlogger::turnoff_logging();

	XLOG_ERR << "dropped";
	XLOG_FILE << "dropped";

	EXPECT_EQ(0u, xlog_test::count());

	xlogger::turnon_logging();
	XLOG_ERR << "kept";
	EXPECT_EQ(1u, xlog_test::count());
}

TEST_F(level_test, async_logging_respects_level)
{
	xlogger::set_log_level(xlogger::_logger_info_id__);

	AXLOG_DBG << "dropped";
	AXLOG_INFO << "kept";

	// 异步日志在后台线程写出, shutdown_logging 会等待队列排空.
	xlogger::shutdown_logging();

	ASSERT_EQ(1u, xlog_test::count());
	EXPECT_EQ(std::string("kept"), xlog_test::messages()[0]);
}
