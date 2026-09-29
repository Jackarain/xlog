//
// test_disabled.cpp
// ~~~~~~~~~~~~~~~~~
//
// 以 -DDISABLE_XLOGGER 编译: 所有日志宏退化为空操作, 且不产生任何输出.
//

#include <xlog/logging.hpp>

// 该宏由构建系统提供(-DDISABLE_XLOGGER), 见 tests/CMakeLists.txt.
#ifndef DISABLE_XLOGGER
#error "test_disabled.cpp must be compiled with -DDISABLE_XLOGGER"
#endif

#include <cassert>
#include <filesystem>
#include <string>

int main()
{
	XLOG_DBG << "debug";
	XLOG_INFO << "info" << 1 << 2.5 << std::string("text");
	XLOG_WARN << "warn";
	XLOG_ERR << "error";
	XLOG_FILE << "file";

	XLOG_FDBG("{}", 1);
	XLOG_FINFO("{}", 1);
	XLOG_FWARN("{}", 1);
	XLOG_FERR("{}", 1);
	XLOG_FFILE("{}", 1);

	AXLOG_DBG << "async";
	AXLOG_FINFO("{}", 1);

	// 注意: 禁用配置下 VXLOG_*/AXVLOG_* 的流式用法不受支持, 这里只用
	// 与启用配置一致的 fmt 形式.
	VXLOG_FDBG("{}", 1);
	VXLOG_FINFO("{}", 1);
	AXVLOG_WFMT("{}", 1);
	AXVLOG_EFMT("{}", 1);
	AXVLOG_FFMT("{}", 1);

	// 空 logger 不产生任何副作用.
	assert(!std::filesystem::exists("./logs"));
	return 0;
}
