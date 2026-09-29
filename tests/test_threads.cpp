//
// test_threads.cpp
// ~~~~~~~~~~~~~~~
//
// 并发写入: 同步路径与异步路径都不能丢日志.
//

#include "capture.hpp"

#include <gtest/gtest.h>

#include <string>
#include <thread>
#include <vector>

class threads_test : public ::testing::Test
{
	protected:
	void SetUp() override
	{
		xlog_test::clear();
		xlog_test::swallow_logs() = true;
		xlogger::toggle_console_logging(false);
		xlogger::toggle_write_logging(false);
		xlogger::turnon_logging();
		xlogger::set_log_level(xlogger::_logger_debug_id__);
	}
};

namespace {

	template <class Log>
	void run_concurrently(int thread_count, int per_thread, Log log)
	{
		std::vector<std::thread> pool;
		pool.reserve(thread_count);
		for (int t = 0; t < thread_count; ++t)
		{
			pool.emplace_back([t, per_thread, log] {
				for (int i = 0; i < per_thread; ++i)
					log(t, i);
			});
		}
		for (auto& thread : pool)
			thread.join();
	}
}

TEST_F(threads_test, concurrent_sync_logging_is_complete)
{
	const int thread_count = 8;
	const int per_thread = 2000;

	run_concurrently(thread_count, per_thread,
		[](int t, int i) { XLOG_INFO << "thread " << t << " message " << i; });

	EXPECT_EQ(
		static_cast<size_t>(thread_count * per_thread), xlog_test::count());
}

TEST_F(threads_test, concurrent_async_logging_is_complete)
{
	const int thread_count = 4;
	const int per_thread = 2000;

	run_concurrently(thread_count, per_thread, [](int t, int i) {
		AXLOG_INFO << "async thread " << t << " message " << i;
	});

	// 等待后台线程把队列写空.
	xlogger::shutdown_logging();

	EXPECT_EQ(
		static_cast<size_t>(thread_count * per_thread), xlog_test::count());
}
