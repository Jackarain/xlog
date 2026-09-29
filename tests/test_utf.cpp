//
// test_utf.cpp
// ~~~~~~~~~~~
//
// UTF 工具函数: 合法性检查, 码点追加, UTF-8/UTF-16 互转.
//

#include <xlog/logging.hpp>

#include <gtest/gtest.h>

#include <string>
#include <string_view>

namespace {

	namespace utf = xlogger::logger_aux__;
}

TEST(utf, validate)
{
	EXPECT_TRUE(utf::utf8_check_is_valid("hello"));
	EXPECT_TRUE(utf::utf8_check_is_valid("你好, 世界"));
	EXPECT_TRUE(utf::utf8_check_is_valid(""));

	EXPECT_FALSE(utf::utf8_check_is_valid(std::string_view("\xff\xfe", 2)));
	EXPECT_FALSE(utf::utf8_check_is_valid(std::string_view("\xe4\xbd", 2)));
}

TEST(utf, append_codepoint)
{
	std::string out;
	ASSERT_TRUE(utf::append(0x4f60, out));
	ASSERT_TRUE(utf::append(0x597d, out));
	EXPECT_EQ("你好", out);

	EXPECT_FALSE(utf::append(0xd800, out));	  // 代理区.
	EXPECT_FALSE(utf::append(0x110000, out)); // 超出 Unicode 范围.
}

TEST(utf, utf8_utf16_roundtrip)
{
#if defined(_WIN32) || defined(WIN32)
	const std::string source = "你好, world";

	auto wide = utf::utf8_utf16(source);
	ASSERT_TRUE(static_cast<bool>(wide));
	EXPECT_EQ(std::wstring(L"你好, world"), *wide);

	auto back = utf::utf16_utf8(*wide);
	ASSERT_TRUE(static_cast<bool>(back));
	EXPECT_EQ(source, *back);
#else
	// 已知问题: 非 Windows 平台 wchar_t 为 32 位, 这两个函数按 UTF-16
	// 处理输入, 结果不正确(见 docs/usage.md 的已知问题).
	GTEST_SKIP() << "utf8_utf16/utf16_utf8 are only correct on Windows";
#endif
}

TEST(utf, string_wide_ascii)
{
	auto wide = utf::string_wide("ascii text");
	ASSERT_TRUE(static_cast<bool>(wide));
	EXPECT_EQ(L"ascii text", *wide);
}

TEST(utf, from_u8string)
{
	const std::string source = "你好";
	EXPECT_EQ(source, utf::from_u8string(source));
	EXPECT_EQ(source, utf::from_u8string(std::string(source)));

#if defined(__cpp_lib_char8_t)
	std::u8string u8 = u8"你好";
	EXPECT_EQ(source, utf::from_u8string(u8));
#endif
}
