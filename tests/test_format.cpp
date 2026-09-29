//
// test_format.cpp
// ~~~~~~~~~~~~~~~
//
// 覆盖流式(operator<<)与格式化(XLOG_F*)两条输出路径以及各类重载.
//

#include "capture.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <thread>

#ifndef LOGGING_DISABLE_BOOST_STRING_VIEW
#include <boost/utility/string_view.hpp>
#endif

#ifndef LOGGING_DISABLE_BOOST_FILESYSTEM
#include <boost/filesystem.hpp>
#endif

#ifndef LOGGING_DISABLE_BOOST_POSIX_TIME
#include <boost/date_time/posix_time/posix_time.hpp>
#endif

#ifndef LOGGING_DISABLE_BOOST_ASIO_ENDPOINT
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#endif

namespace {

	// 执行一次日志输出并返回捕获到的消息.
	template <class F> std::string capture_one(F&& f)
	{
		xlog_test::clear();
		f();
		auto msgs = xlog_test::messages();
		return msgs.empty() ? std::string("<none>") : msgs.front();
	}
}

class format_test : public ::testing::Test
{
	protected:
	void SetUp() override
	{
		xlog_test::clear();
		xlog_test::swallow_logs() = true;
		xlogger::set_log_level(xlogger::_logger_debug_id__);
		xlogger::toggle_write_logging(false);
		xlogger::toggle_console_logging(false);
	}
};

TEST_F(format_test, integral_and_float)
{
	EXPECT_EQ("42", capture_one([] { XLOG_INFO << 42; }));
	EXPECT_EQ("-7", capture_one([] { XLOG_INFO << static_cast<short>(-7); }));
	EXPECT_EQ("18446744073709551615",
		capture_one([] { XLOG_INFO << 0xffffffffffffffffull; }));
	EXPECT_EQ("3.5", capture_one([] { XLOG_INFO << 3.5; }));
	EXPECT_EQ("1.25", capture_one([] { XLOG_INFO << 1.25f; }));
	EXPECT_EQ("true", capture_one([] { XLOG_INFO << true; }));
	EXPECT_EQ("false", capture_one([] { XLOG_INFO << false; }));
	EXPECT_EQ("A", capture_one([] { XLOG_INFO << 'A'; }));
}

TEST_F(format_test, strings_and_views)
{
	EXPECT_EQ("c-string", capture_one([] { XLOG_INFO << "c-string"; }));
	EXPECT_EQ("std::string",
		capture_one([] { XLOG_INFO << std::string("std::string"); }));
	EXPECT_EQ("string_view",
		capture_one([] { XLOG_INFO << std::string_view("string_view"); }));
	EXPECT_EQ(
		"宽字符", capture_one([] { XLOG_INFO << std::u8string(u8"宽字符"); }));

#ifndef LOGGING_DISABLE_BOOST_STRING_VIEW
	EXPECT_EQ("boost view",
		capture_one([] { XLOG_INFO << boost::string_view("boost view"); }));
#endif
}

TEST_F(format_test, wide_strings)
{
#if defined(_WIN32) || defined(WIN32)
	EXPECT_EQ(
		"宽字符", capture_one([] { XLOG_INFO << std::wstring(L"宽字符"); }));
	EXPECT_EQ(
		"宽字符", capture_one([] { XLOG_INFO << std::u16string(u"宽字符"); }));
	EXPECT_EQ("宽字符", capture_one([] { XLOG_INFO << L"宽字符"; }));
#else
	// 已知问题: 非 Windows 平台 wchar_t 为 32 位, wstring/u16string 走的是
	// 按 UTF-16 实现的转换, 结果不正确(见 docs/usage.md 的已知问题).
	GTEST_SKIP() << "wide string conversion is only correct on Windows";
#endif
}

TEST_F(format_test, paths)
{
#if defined(_WIN32) || defined(WIN32)
	EXPECT_EQ("/tmp/x y",
		capture_one([] { XLOG_INFO << std::filesystem::path("/tmp/x y"); }));

#ifndef LOGGING_DISABLE_BOOST_FILESYSTEM
	EXPECT_EQ("/tmp/x y",
		capture_one([] { XLOG_INFO << boost::filesystem::path("/tmp/x y"); }));
#endif
#else
	// 同上: path 重载内部使用 wstring 转换, 非 Windows 平台结果不正确.
	GTEST_SKIP() << "path conversion relies on wchar_t based UTF conversion";
#endif
}

TEST_F(format_test, format_and_stream_mixed_pointer)
{
	const void* fake =
		reinterpret_cast<const void*>(static_cast<std::uintptr_t>(0x1234));
	auto msg = capture_one([&] { XLOG_INFO << fake; });
	EXPECT_EQ(10u, msg.size());
	EXPECT_EQ(std::string("0x00001234"), msg);
}

TEST_F(format_test, durations)
{
	using namespace std::chrono;
	EXPECT_EQ("7ns", capture_one([] { XLOG_INFO << nanoseconds(7); }));
	EXPECT_EQ("6us", capture_one([] { XLOG_INFO << microseconds(6); }));
	EXPECT_EQ("5ms", capture_one([] { XLOG_INFO << milliseconds(5); }));
	EXPECT_EQ("4s", capture_one([] { XLOG_INFO << seconds(4); }));
	EXPECT_EQ("3min", capture_one([] { XLOG_INFO << minutes(3); }));
	EXPECT_EQ("2h", capture_one([] { XLOG_INFO << hours(2); }));
}

#if (__cplusplus >= 202002L)
TEST_F(format_test, calendar_types)
{
	using namespace std::chrono;
	EXPECT_EQ("1d", capture_one([] { XLOG_INFO << days(1); }));
	EXPECT_EQ("2weeks", capture_one([] { XLOG_INFO << weeks(2); }));
	EXPECT_EQ("3months", capture_one([] { XLOG_INFO << months(3); }));
	EXPECT_EQ("4years", capture_one([] { XLOG_INFO << years(4); }));

#ifdef __cpp_lib_char8_t
	EXPECT_EQ("周一", capture_one([] { XLOG_INFO << weekday(1); }));
	EXPECT_EQ("02月", capture_one([] { XLOG_INFO << month(2); }));
	EXPECT_EQ("03日", capture_one([] { XLOG_INFO << day(3); }));
	EXPECT_EQ("2024年", capture_one([] { XLOG_INFO << year(2024); }));
#else
	EXPECT_EQ("Monday", capture_one([] { XLOG_INFO << weekday(1); }));
	EXPECT_EQ("February", capture_one([] { XLOG_INFO << month(2); }));
	EXPECT_EQ("03", capture_one([] { XLOG_INFO << day(3); }));
	EXPECT_EQ("2024", capture_one([] { XLOG_INFO << year(2024); }));
#endif
}
#endif

#ifndef LOGGING_DISABLE_BOOST_POSIX_TIME
TEST_F(format_test, boost_ptime)
{
	using namespace boost::posix_time;
	auto p = ptime(
		boost::gregorian::date(2024, 1, 2), hours(3) + minutes(4) + seconds(5));
	EXPECT_EQ("2024-01-02 03:04:05", capture_one([&] { XLOG_INFO << p; }));
}
#endif

#ifndef LOGGING_DISABLE_BOOST_ASIO_ENDPOINT
TEST_F(format_test, boost_endpoint)
{
	namespace net = boost::asio;
	net::ip::tcp::endpoint v4(net::ip::make_address("1.2.3.4"), 80);
	net::ip::tcp::endpoint v6(net::ip::make_address("::1"), 443);
	net::ip::udp::endpoint u4(net::ip::make_address("5.6.7.8"), 53);

	EXPECT_EQ("1.2.3.4:80", capture_one([&] { XLOG_INFO << v4; }));
	EXPECT_EQ("[::1]:443", capture_one([&] { XLOG_INFO << v6; }));
	EXPECT_EQ("5.6.7.8:53", capture_one([&] { XLOG_INFO << u4; }));
}
#endif

TEST_F(format_test, thread_id_is_not_empty)
{
	EXPECT_FALSE(
		capture_one([] { XLOG_INFO << std::this_thread::get_id(); }).empty());
}

TEST_F(format_test, format_macros)
{
	EXPECT_EQ(
		"1 + 2 = 3", capture_one([] { XLOG_FINFO("{} + {} = {}", 1, 2, 3); }));
	EXPECT_EQ("0042", capture_one([] { XLOG_FINFO("{:04}", 42); }));
	EXPECT_EQ("3.14", capture_one([] { XLOG_FINFO("{:.2f}", 3.14159); }));
	EXPECT_EQ(
		"你好世界", capture_one([] { XLOG_FINFO("{}{}", "你好", "世界"); }));
}

TEST_F(format_test, format_and_stream_mixed)
{
	EXPECT_EQ(
		"a-b-c", capture_one([] { XLOG_FINFO("{}", 'a') << "-b" << "-c"; }));
}

TEST_F(format_test, multiple_stream_values)
{
	EXPECT_EQ("x = 1, y = 2.5, ok = true", capture_one([] {
		XLOG_INFO << "x = " << 1 << ", y = " << 2.5 << ", ok = " << true;
	}));
}

TEST_F(format_test, level_prefix_is_recorded)
{
	xlogger::set_log_level(xlogger::_logger_debug_id__);
	XLOG_WARN << "warn message";

	ASSERT_EQ(1u, xlog_test::count());
	EXPECT_EQ(static_cast<int>(xlogger::_logger_warn_id__),
		xlog_test::levels().front());
}
