//
// disable.cpp
// ~~~~~~~~~~~
//
// 以 -DDISABLE_XLOGGER 编译: 所有日志宏退化为空操作, 不产生任何代码
// 路径(也不依赖 fmt/zlib), 适合发布版彻底移除日志.
//

#include <xlog/logging.hpp>

#include <iostream>

int main()
{
	XLOG_DBG << "debug " << 1;
	XLOG_INFO << "info " << 2;
	XLOG_FINFO("fmt {} {}", 1, 2);
	AXLOG_ERR << "async " << 3;

	std::cout << "logging is compiled out" << std::endl;

	return 0;
}
