//
// consumer.cpp
// ~~~~~~~~~~~~
//
// 安装导出验证: 独立工程通过 find_package(xlog) 或 pkg-config 使用 xlog.
//

#include <xlog/logging.hpp>

#include <iostream>

int main()
{
	xlogger::toggle_write_logging(false);
	xlogger::toggle_console_logging(false);
	xlogger::set_log_level(xlogger::_logger_debug_id__);

	XLOG_INFO << "consumer " << 1;
	XLOG_FWARN("consumer {}", 2);

	std::cout << "xlog consumer ok" << std::endl;

	return xlogger::logging_level() == xlogger::_logger_debug_id__ ? 0 : 1;
}
