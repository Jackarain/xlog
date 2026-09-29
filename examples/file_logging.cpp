//
// file_logging.cpp
// ~~~~~~~~~~~~~~~~
//
// 写入日志文件, 设置滚动上限(最小 10 MiB), 退出前关闭异步线程.
//

#include <xlog/logging.hpp>

#include <iostream>

int main()
{
	// 日志只写文件, 不打印到控制台.
	xlogger::toggle_console_logging(false);
	xlogger::toggle_write_logging(true);

	// 不传参数时写入 ./logs/<LOG_APPNAME>.log;
	// 也可以指定目录: xlogger::init_logging("/var/log/myapp").
	xlogger::init_logging();

	// 单个日志文件超过 10 MiB 后滚动(旧文件可压缩为 .gz).
	xlogger::set_logfile_maxsize(10 * 1024 * 1024);

	XLOG_INFO << "log file: " << xlogger::log_path();
	XLOG_FERR("formatted message {}", 42);

	// 等待异步队列排空, 停止后台线程.
	xlogger::shutdown_logging();

	std::cout << "done, see " << xlogger::log_path() << std::endl;

	return 0;
}
