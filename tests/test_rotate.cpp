//
// test_rotate.cpp
// ~~~~~~~~~~~~~~~
//
// 超过 set_logfile_maxsize 上限后滚动日志: 旧文件被改名(可能再压缩),
// application.log 重新开始写入.
//

#include <xlog/logging.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
#include <thread>

namespace {

	namespace fs = std::filesystem;

	constexpr int64_t max_size = 10 * 1024 * 1024; // 最小可设置值.

	// 返回 logs 目录下除 application.log 之外的滚动文件.
	std::string rotated_file(const fs::path& dir)
	{
		std::error_code ec;
		const std::string active = std::string(LOG_APPNAME) + ".log";
		for (const auto& entry : fs::directory_iterator(dir, ec))
		{
			const std::string name = entry.path().filename().string();
			if (name == active)
				continue;
			return name;
		}
		return {};
	}

	std::string read_head(const fs::path& path, size_t size)
	{
		std::ifstream ifs(path, std::ios::binary);
		std::string out(size, '\0');
		ifs.read(out.data(), static_cast<std::streamsize>(size));
		out.resize(static_cast<size_t>(ifs.gcount()));
		return out;
	}
}

TEST(rotate, size_limit_rotates_file)
{
	namespace fs = std::filesystem;
	std::error_code ec;

	xlogger::toggle_console_logging(false);
	xlogger::toggle_write_logging(true);
	xlogger::turnon_logging();
	xlogger::set_log_level(xlogger::_logger_debug_id__);
	xlogger::init_logging();
	xlogger::set_logfile_maxsize(max_size);

	const fs::path dir = "./logs";
	const fs::path active = dir / (std::string(LOG_APPNAME) + ".log");

	// 每行约 550 字节, 写 26000 行总计约 13 MiB, 足以触发一次滚动.
	const std::string filler(512, 'x');
	std::string rotated;
	for (int i = 0; i < 26000 && rotated.empty(); ++i)
	{
		XLOG_INFO << filler;
		if ((i % 256) == 0)
			rotated = rotated_file(dir);
	}

	// 压缩在后台线程完成, 这里等待滚动文件出现.
	for (int i = 0; i < 200 && rotated.empty(); ++i)
	{
		rotated = rotated_file(dir);
		if (rotated.empty())
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	ASSERT_FALSE(rotated.empty()) << "log file was not rotated";
	EXPECT_NE(std::string::npos, rotated.find(".log"));
	EXPECT_TRUE(fs::exists(active, ec));
	EXPECT_FALSE(read_head(active, 4096).empty());

	// 滚动文件内容非空(压缩线程可能刚把它换成 .gz, 容忍这种竞态).
	bool rotated_has_data = false;
	for (int i = 0; i < 100 && !rotated_has_data; ++i)
	{
		std::error_code size_ec;
		const auto size = fs::file_size(dir / rotated_file(dir), size_ec);
		rotated_has_data = !size_ec && size > 0;
		if (!rotated_has_data)
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
	EXPECT_TRUE(rotated_has_data);
}
