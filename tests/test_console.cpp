//
// test_console.cpp
// ~~~~~~~~~~~~~~~~
//
// 控制台输出: 把 stdout 重定向到文件, 校验前缀与消息内容(POSIX).
//

#include <xlog/logging.hpp>

#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#if defined(_WIN32) || defined(WIN32)
#define XLOG_TEST_NO_REDIRECT 1
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace {

	std::string read_file(const std::filesystem::path& path)
	{
		std::ifstream ifs(path, std::ios::binary);
		return std::string(std::istreambuf_iterator<char>(ifs),
			std::istreambuf_iterator<char>());
	}
}

TEST(console, writes_to_stdout)
{
#if defined(XLOG_TEST_NO_REDIRECT)
	GTEST_SKIP() << "stdout redirection is only implemented for POSIX";
#else
	xlogger::toggle_write_logging(false);
	xlogger::toggle_console_logging(true);
	xlogger::toggle_console_logging_color(false);
	xlogger::set_log_level(xlogger::_logger_debug_id__);

	const std::filesystem::path out = "console_capture.txt";
	const int saved = ::dup(STDOUT_FILENO);
	ASSERT_GE(saved, 0);

	const int fd =
		::open(out.string().c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
	ASSERT_GE(fd, 0);
	ASSERT_EQ(STDOUT_FILENO, ::dup2(fd, STDOUT_FILENO));
	::close(fd);

	XLOG_INFO << "console-marker";
	std::fflush(stdout);

	ASSERT_EQ(STDOUT_FILENO, ::dup2(saved, STDOUT_FILENO));
	::close(saved);

	const std::string content = read_file(out);
	EXPECT_NE(std::string::npos, content.find(" INFO  console-marker"));

	// 关闭颜色后不应残留 ANSI 转义序列.
	EXPECT_EQ(std::string::npos, content.find("\033["));
#endif
}
