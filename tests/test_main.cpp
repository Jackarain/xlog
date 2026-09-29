//
// test_main.cpp
// ~~~~~~~~~~~~~
//
// 所有 GoogleTest 用例共用的入口: 把工作目录切到进程私有的临时目录,
// 日志文件(./logs)等副作用只落在临时目录内, 测试结束后清理.
//

#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <system_error>

#if defined(_WIN32) || defined(WIN32)
#include <process.h>
#define XLOG_TEST_PID() _getpid()
#else
#include <unistd.h>
#define XLOG_TEST_PID() getpid()
#endif

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);

	std::error_code ec;
	std::filesystem::path base = std::filesystem::temp_directory_path(ec);
	if (ec)
		base = std::filesystem::current_path(ec);

	std::filesystem::path work =
		base / ("xlog_test_" + std::to_string(XLOG_TEST_PID()));
	std::filesystem::remove_all(work, ec);
	std::filesystem::create_directories(work, ec);
	std::filesystem::current_path(work, ec);

	int result = RUN_ALL_TESTS();

	std::filesystem::current_path(base, ec);
	std::filesystem::remove_all(work, ec);

	return result;
}
