//
// capture.hpp
// ~~~~~~~~~~~
//
// 测试辅助: 通过 tag_invoke 钩子把日志截获到内存, 避免测试产生真实
// 的磁盘/控制台输出. 每个测试可执行文件只在唯一的测试 TU 中包含此头.
//

#ifndef XLOG_TEST_CAPTURE_HPP
#define XLOG_TEST_CAPTURE_HPP

#include <xlog/logging.hpp>

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace xlog_test {

	struct captured_log
	{
		int64_t time;
		int level;
		std::string message;
	};

	inline std::mutex& capture_mutex()
	{
		static std::mutex instance;
		return instance;
	}

	inline std::vector<captured_log>& captured_ref()
	{
		static std::vector<captured_log> instance;
		return instance;
	}

	// 返回捕获内容的副本.
	inline std::vector<captured_log> captured()
	{
		std::lock_guard<std::mutex> lock(capture_mutex());
		return captured_ref();
	}

	// 捕获的日志是否需要吞掉(不再写文件/控制台), 默认吞掉.
	inline bool& swallow_logs()
	{
		static bool instance = true;
		return instance;
	}

	// 返回 true 时中断后续日志输出(模拟 logger_abort_tag).
	inline bool& abort_flag()
	{
		static bool instance = false;
		return instance;
	}

	inline void clear()
	{
		std::lock_guard<std::mutex> lock(capture_mutex());
		captured_ref().clear();
	}

	inline size_t count()
	{
		std::lock_guard<std::mutex> lock(capture_mutex());
		return captured_ref().size();
	}

	inline std::vector<std::string> messages()
	{
		std::lock_guard<std::mutex> lock(capture_mutex());
		std::vector<std::string> out;
		out.reserve(captured_ref().size());
		for (const auto& item : captured_ref())
			out.push_back(item.message);
		return out;
	}

	inline std::vector<int> levels()
	{
		std::lock_guard<std::mutex> lock(capture_mutex());
		std::vector<int> out;
		out.reserve(captured_ref().size());
		for (const auto& item : captured_ref())
			out.push_back(item.level);
		return out;
	}
}

namespace xlogger {
	inline bool tag_invoke(
		logger_tag, int64_t time, const int& level, const std::string& message)
	{
		{
			std::lock_guard<std::mutex> lock(xlog_test::capture_mutex());
			xlog_test::captured_ref().push_back(
				xlog_test::captured_log{time, level, message});
		}

		// true: 日志已被测试捕获, 不再写盘与打印.
		return xlog_test::swallow_logs();
	}

	inline bool tag_invoke(logger_abort_tag)
	{
		return xlog_test::abort_flag();
	}
}

#endif // XLOG_TEST_CAPTURE_HPP
