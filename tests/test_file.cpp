//
// test_file.cpp
// ~~~~~~~~~~~~~
//
// 落盘行为: 默认日志路径, 追加写入, 写开关与级别门控.
//

#include <xlog/logging.hpp>

#include <gtest/gtest.h>

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

class file_test : public ::testing::Test
{
	protected:
	void SetUp() override
	{
		xlogger::toggle_write_logging(true);
		xlogger::toggle_console_logging(false);
		xlogger::turnon_logging();
		xlogger::set_log_level(xlogger::_logger_debug_id__);
	}
};

TEST_F(file_test, default_log_path)
{
	const std::string path = xlogger::log_path();
	EXPECT_NE(std::string::npos, path.find("logs"));
	EXPECT_NE(std::string::npos, path.find(LOG_APPNAME));
	EXPECT_NE(std::string::npos, path.find(".log"));
}

TEST_F(file_test, message_is_written_to_file)
{
	XLOG_INFO << "file-message-marker";

	const std::string content = read_file(xlogger::log_path());
	ASSERT_FALSE(content.empty());
	EXPECT_NE(std::string::npos, content.find("file-message-marker"));

	// 每行形如: 2024-01-02 03:04:05.678 INFO  <message>\n
	const auto pos = content.find("file-message-marker");
	ASSERT_NE(std::string::npos, pos);
	const auto line_begin = content.rfind('\n', pos);
	const auto line_end = content.find('\n', pos);
	ASSERT_NE(std::string::npos, line_end);

	const size_t begin = (line_begin == std::string::npos) ? 0 : line_begin + 1;
	const std::string line = content.substr(begin, line_end - begin);
	EXPECT_NE(std::string::npos, line.find(" INFO  "));
	EXPECT_NE(std::string::npos, line.find("file-message-marker"));
}

TEST_F(file_test, messages_are_appended)
{
	xlogger::toggle_write_logging(true);
	XLOG_INFO << "first-appended-marker";
	XLOG_INFO << "second-appended-marker";

	const std::string content = read_file(xlogger::log_path());
	EXPECT_NE(std::string::npos, content.find("first-appended-marker"));
	EXPECT_NE(std::string::npos, content.find("second-appended-marker"));
}

TEST_F(file_test, write_toggle_controls_output)
{
	xlogger::toggle_write_logging(false);
	XLOG_INFO << "disabled-write-marker";
	EXPECT_EQ(std::string::npos,
		read_file(xlogger::log_path()).find("disabled-write-marker"));

	xlogger::toggle_write_logging(true);
	XLOG_INFO << "enabled-write-marker";
	EXPECT_NE(std::string::npos,
		read_file(xlogger::log_path()).find("enabled-write-marker"));
}

TEST_F(file_test, level_gates_file_output)
{
	xlogger::set_log_level(xlogger::_logger_error_id__);
	XLOG_INFO << "filtered-info-marker";
	XLOG_ERR << "kept-error-marker";

	const std::string content = read_file(xlogger::log_path());
	EXPECT_EQ(std::string::npos, content.find("filtered-info-marker"));
	EXPECT_NE(std::string::npos, content.find("kept-error-marker"));

	xlogger::set_log_level(xlogger::_logger_debug_id__);
}

TEST_F(file_test, turnoff_logging_stops_output)
{
	xlogger::turnoff_logging();
	XLOG_ERR << "turnoff-marker";
	EXPECT_EQ(std::string::npos,
		read_file(xlogger::log_path()).find("turnoff-marker"));

	xlogger::turnon_logging();
	XLOG_ERR << "turnon-marker";
	EXPECT_NE(std::string::npos,
		read_file(xlogger::log_path()).find("turnon-marker"));
}

TEST_F(file_test, maxsize_rejects_too_small_values)
{
	xlogger::set_logfile_maxsize(1024);
	EXPECT_EQ(static_cast<int64_t>(-1), xlogger::global_logfile_size___);

	xlogger::set_logfile_maxsize(10 * 1024 * 1024);
	EXPECT_EQ(static_cast<int64_t>(10 * 1024 * 1024),
		xlogger::global_logfile_size___);

	xlogger::set_logfile_maxsize(-1);
	EXPECT_EQ(static_cast<int64_t>(-1), xlogger::global_logfile_size___);
}
