//
// async.cpp
// ~~~~~~~~~
//
// 异步日志: AXLOG_* 系列把格式化后的消息投递到后台线程写盘, 适合
// 高并发/低延迟路径; INIT_ASYNC_LOGGING 负责初始化与退出前排空队列.
//

#include <xlog/logging.hpp>

#include <iostream>
#include <thread>
#include <vector>

int main()
{
	xlogger::toggle_console_logging(true);

	// 声明一个进程级的异步日志初始化器: 构造时初始化日志, 析构时
	// 等待队列排空并停止后台线程.
	INIT_ASYNC_LOGGING();

	std::vector<std::thread> workers;
	for (int i = 0; i < 4; ++i)
	{
		workers.emplace_back([i] {
			for (int n = 0; n < 100; ++n)
				AXLOG_FINFO("worker {} message {}", i, n);
		});
	}

	for (auto& worker : workers)
		worker.join();

	AXLOG_INFO << "all workers finished";

	// INIT_ASYNC_LOGGING() 声明的对象在 main 结束时析构, 自动排空队列.
	std::cout << "queued asynchronously, flushing on exit" << std::endl;

	return 0;
}
