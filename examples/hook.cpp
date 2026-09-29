//
// hook.cpp
// ~~~~~~~
//
// 用户钩子: 用 tag_invoke 接管日志, 转发到自定义的日志系统.
// 钩子返回 true 表示该条日志已被处理, 不再写本地文件/控制台.
//

#include <xlog/logging.hpp>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

	struct log_entry
	{
		std::int64_t time;
		int level;
		std::string message;
	};

	std::vector<log_entry> g_sink;

	std::string level_name(int level)
	{
		return xlogger::logger_level_string__(
			static_cast<xlogger::logger_level__>(level));
	}
}

namespace xlogger {
	bool tag_invoke(
		logger_tag, int64_t time, const int& level, const std::string& message)
	{
		g_sink.push_back(log_entry{time, level, message});

		// true: 已被钩子接管, 不再输出到文件/控制台.
		return true;
	}
}

int main()
{
	XLOG_INFO << "captured by hook: " << 1;
	XLOG_FWARN("captured by hook: {}", 2);
	XLOG_ERR << "captured by hook: " << 3;

	std::cout << "hook captured " << g_sink.size() << " messages" << std::endl;
	for (const auto& entry : g_sink)
		std::cout << "[" << level_name(entry.level) << "] " << entry.message
				  << std::endl;

	return 0;
}
