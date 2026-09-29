//
// basic.cpp
// ~~~~~~~~~
//
// 基础用法: 控制台输出, 级别门控, 流式与 fmt 两种写法.
//

#include <xlog/logging.hpp>

#include <string>

int main()
{
	// 输出所有级别(DEBUG/INFO/WARN/ERROR), 默认即为 debug.
	xlogger::set_log_level(xlogger::_logger_debug_id__);

	// 流式写法.
	XLOG_DBG << "debug: " << 1;
	XLOG_INFO << "info: " << std::string("hello");
	XLOG_WARN << "warn: " << 2.5;
	XLOG_ERR << "error: " << true;

	// fmt 写法.
	XLOG_FINFO("fmt: {} + {} = {}", 1, 2, 3);

	// 两者可以混用.
	XLOG_FWARN("fmt then stream") << ", appended " << 42;

	// 提高门槛后, 低级别日志被直接丢弃(不做格式化, 无任何开销).
	xlogger::set_log_level(xlogger::_logger_warn_id__);
	XLOG_INFO << "filtered out";
	XLOG_WARN << "warn is the minimum level now";

	return 0;
}
