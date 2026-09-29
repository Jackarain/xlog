//
// test_no_exceptions.cpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// 以 -fno-exceptions -fno-rtti 编译并运行, 证明头文件不依赖异常与 RTTI.
//

#include <xlog/logging.hpp>

#include <string>

int main()
{
	xlogger::toggle_write_logging(false);
	xlogger::toggle_console_logging(false);
	xlogger::set_log_level(xlogger::_logger_debug_id__);

	XLOG_DBG << "debug " << 1;
	XLOG_INFO << "info " << 2.5;
	XLOG_WARN << "warn " << std::string("text");
	XLOG_ERR << "error " << true;
	XLOG_FILE << "file";

	XLOG_FDBG("{} {}", "fmt", 1);
	XLOG_FINFO("{} {}", "fmt", 2);
	XLOG_FWARN("{} {}", "fmt", 3);
	XLOG_FERR("{} {}", "fmt", 4);
	XLOG_FFILE("{} {}", "fmt", 5);

	AXLOG_DBG << "async " << 1;
	AXLOG_INFO << "async " << 2;
	AXLOG_WARN << "async " << 3;
	AXLOG_ERR << "async " << 4;
	AXLOG_FILE << "async " << 5;

	AXVLOG_WARN << "verbose";
	VXLOG_ERR << "verbose";
	VXLOG_FINFO("verbose {}", 1);
	AXVLOG_EFMT("verbose {}", 2);
	AXVLOG_FMT("verbose {}", 3);

	xlogger::shutdown_logging();

	return xlogger::logging_level() == xlogger::_logger_debug_id__ ? 0 : 1;
}
