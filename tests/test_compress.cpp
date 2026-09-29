//
// test_compress.cpp
// ~~~~~~~~~~~~~~~~~
//
// 日志压缩: do_compress_gz 生成的 .gz 必须能被 zlib 解压还原.
//

#include <xlog/logging.hpp>

#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <zlib.h>

namespace {

	std::string gunzip(const std::filesystem::path& path)
	{
		gzFile gz = gzopen(path.string().c_str(), "rb");
		if (!gz)
			return {};

		std::string out;
		std::vector<char> buffer(64 * 1024);
		int n = 0;
		while ((n = gzread(gz, buffer.data(),
					static_cast<unsigned int>(buffer.size()))) > 0)
			out.append(buffer.data(), static_cast<size_t>(n));
		gzclose(gz);
		return out;
	}
}

TEST(compress, gzip_roundtrip)
{
	const std::filesystem::path source = "compress_source.log";

	std::string content;
	for (int i = 0; i < 20000; ++i)
		content += "2024-01-02 03:04:05.678 INFO  compressible line " +
				   std::to_string(i) + "\n";

	{
		std::ofstream ofs(source, std::ios::binary);
		ofs << content;
	}

	ASSERT_TRUE(xlogger::xlogging_compress__::do_compress_gz(source.string()));

	const std::filesystem::path gz = source.string() + ".gz";
	ASSERT_TRUE(std::filesystem::exists(gz));
	EXPECT_LT(std::filesystem::file_size(gz), content.size());

	EXPECT_EQ(content, gunzip(gz));
}
