//
// test_signal.cpp
// ~~~~~~~~~~~~~~~
//
// 信号语义: 致命信号(SIGSEGV 等)必须终止进程, 不得因处理函数返回而陷入
// 无限循环; 库内不注册 SIGTERM, 进程仍保持默认的终止语义. 用例在 fork
// 出的子进程中触发信号, 由父进程断言子进程以对应信号退出.
//

#include <xlog/logging.hpp>

#include <gtest/gtest.h>

#if defined(_WIN32) || defined(WIN32)

TEST(signal, signal_semantics)
{
	GTEST_SKIP() << "fork based signal test is POSIX only";
}

#else

#include <csignal>
#include <ctime>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

	enum class child_action
	{
		raise_sigterm,
		crash_sigsegv,
	};

	// 写 PROT_NONE 映射制造真实缺页, 得到硬件触发的 SIGSEGV. 不用空指针
	// 解引用: 那是 UB, 会被 UBSan/ASan 提前拦截而走不到信号处理函数.
	void trigger_segv()
	{
		void* page = ::mmap(nullptr, 4096, PROT_NONE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (page == MAP_FAILED)
			::_exit(1);

		*static_cast<volatile int*>(page) = 1;
	}

	// 子进程: 触发指定信号后正常走到 _exit(0)(父进程据此判定信号被吞掉);
	// 关掉 core dump 以免测试目录留下大文件.
	void trigger_in_child(child_action action)
	{
		rlimit no_core{ 0, 0 };
		::setrlimit(RLIMIT_CORE, &no_core);

		switch (action)
		{
		case child_action::raise_sigterm:
			::raise(SIGTERM);
			break;
		case child_action::crash_sigsegv:
			trigger_segv();
			break;
		}

		::_exit(0);
	}

	// 等待子进程退出; 超时(说明信号被吞掉后仍在运行, 或处理函数陷入循环)
	// 时强杀, 返回 false.
	bool wait_child(::pid_t pid, int& status, int timeout_ms)
	{
		for (int waited = 0; waited < timeout_ms; waited += 50)
		{
			if (::waitpid(pid, &status, WNOHANG) == pid)
				return true;

			timespec delay{ 0, 50 * 1000 * 1000 };
			::nanosleep(&delay, nullptr);
		}

		::kill(pid, SIGKILL);
		::waitpid(pid, &status, 0);
		return false;
	}

	// 在 fork 出的子进程中触发信号, 断言其以 expect_signal 终止.
	void expect_child_dies_by_signal(child_action action, int expect_signal)
	{
		int status = 0;
		const ::pid_t pid = ::fork();
		ASSERT_NE(-1, pid);
		if (pid == 0)
			trigger_in_child(action);

		ASSERT_TRUE(wait_child(pid, status, 5000));
		ASSERT_TRUE(WIFSIGNALED(status));
		EXPECT_EQ(expect_signal, WTERMSIG(status));
	}
}

TEST(signal, sigterm_is_not_swallowed)
{
	expect_child_dies_by_signal(child_action::raise_sigterm, SIGTERM);
}

TEST(signal, fatal_signal_terminates_process)
{
	expect_child_dies_by_signal(child_action::crash_sigsegv, SIGSEGV);
}

#endif
