# xlog

一个 **header-only**、基于 **C++20** 的功能完整的日志库：支持日志压缩、
异步写入、日志级别门控，并对接 Android logcat、systemd journal 与
Windows DebugView 等日志系统。

```cpp
#include <xlog/logging.hpp>
```

## 特性

- **两套写法**：流式 `XLOG_INFO << "hello, " << "world"` 与
  `fmt` 风格 `XLOG_FINFO("{}, {}", "hello", "world")`，两者可混用。
- **异步日志**：`AXLOG_*` / `AXLOG_F*` 把消息投递到后台线程，热路径
  只做格式化与入队；退出时自动排空队列，崩溃时尽量不丢日志。
- **级别门控**：`set_log_level()` 设置全局最低级别，低于该级别的日志在
  构造时即被丢弃——不做格式化、不写盘，几乎零开销。
- **日志文件**：按大小或按小时滚动，旧文件自动 gzip 压缩并保留。
- **异步安全**：全局单点写入 + 互斥保护，多线程写入不丢失、不乱序。
- **多后端**：控制台（ANSI 颜色）、文件、Android logcat、systemd journal、
  Windows DebugView。
- **用户钩子**：通过 `tag_invoke` 把日志转发到自己的日志系统，或在中止
  时停止输出。
- **可裁剪**：`-DDISABLE_XLOGGER` 后所有日志宏退化为空操作。
- **可选依赖**：Boost（filesystem/asio/date_time/string_view）与 zlib 均
  按 `__has_include` 自动探测，缺失时自动降级为 `std::filesystem` 等。

## 环境要求

- 支持 C++20 的编译器（GCC 13+ / Clang 16+ / MSVC 19.29+）。
- 格式化后端：`std::format`，或 `{fmt}`（Android、libc++ < 17 及旧
  libstdc++ 会自动回退到 `{fmt}`，可用 `-DFORCE_USE_FMT_FORMAT` 强制）。
- Boost 头文件可选（`boost::filesystem` 需要链接库），zlib 可选。
- 构建系统需要 CMake ≥ 3.16（头文件本身无需构建步骤）。

## 集成方式

### CMake（FetchContent）

```cmake
include(FetchContent)
FetchContent_Declare(xlog
    GIT_REPOSITORY https://github.com/Jackarain/xlog
    GIT_TAG        v1.0.0)
FetchContent_MakeAvailable(xlog)

target_link_libraries(my_app PRIVATE xlog::xlog)
```

### CMake（安装后 find_package）

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
cmake --install build
```

```cmake
find_package(xlog REQUIRED)
target_link_libraries(my_app PRIVATE xlog::xlog)
```

### pkg-config

```bash
c++ -std=c++20 $(pkg-config --cflags --libs xlog) my_app.cpp
```

### 直接拷贝

把 `include/xlog/logging.hpp` 拷入你的工程并添加包含路径即可。

## 快速上手

### 流式与 fmt

```cpp
// 流式日志输出:
XLOG_DBG << "hello, " << "world";
XLOG_WARN << "hello, " << "world";
XLOG_ERR << "hello, " << "world";

// 支持 FMT 语法:
XLOG_FINFO("{}, {}", "hello", "world");

// 甚至还可以:
XLOG_FINFO("{}, {}", "hello", "world") << ", 你好, " << "世界!";
```

### 级别门控

```cpp
xlogger::set_log_level(xlogger::_logger_info_id__);

XLOG_DBG << "被丢弃";                       // 不做格式化, 无任何输出
XLOG_INFO << "会被输出";
```

### 写入日志文件

```cpp
xlogger::init_logging("/var/log/myapp");   // 默认 ./logs
xlogger::set_logfile_maxsize(10 * 1024 * 1024);  // 超过 10 MiB 滚动

XLOG_INFO << "log file: " << xlogger::log_path();
xlogger::shutdown_logging();
```

### 异步日志

```cpp
int main()
{
    INIT_ASYNC_LOGGING();                  // 退出时自动排空队列

    AXLOG_INFO << "written by the background thread";
    AXLOG_FWARN("fmt {}", 42);
}
```

### 用户钩子

```cpp
namespace xlogger {
    bool tag_invoke(logger_tag, int64_t time,
        const int& level, const std::string& message)
    {
        my_log_system(time, level, message);
        return true;                       // 已接管, 不再写文件/控制台
    }
}
```

## 文档

- [README.md](README.md) —— 本页
- [docs/usage.md](docs/usage.md) —— API 参考、编译期开关与集成方式
- [docs/design.md](docs/design.md) —— 内部结构、线程模型、滚动与已知限制

## 构建测试与示例

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

可选选项：

| 选项                         | 默认  | 说明                                    |
|------------------------------|-------|-----------------------------------------|
| `XLOG_BUILD_TESTS`           | `ON`  | 构建 GoogleTest 测试套件                |
| `XLOG_BUILD_EXAMPLES`        | `ON`  | 构建示例                                |
| `XLOG_INSTALL`               | `ON`  | 安装规则 + CMake 包配置 + pkg-config    |
| `XLOG_PEDANTIC`              | `ON`  | 本仓库目标启用 `-Wall -Wextra -Wpedantic` |
| `XLOG_USE_FMT`               | `OFF` | 强制使用 `{fmt}` 作为格式化后端         |
| `XLOG_ENABLE_BOOST`          | `ON`  | 探测并使用 Boost 可选集成               |
| `XLOG_ENABLE_COMPRESSION`    | `ON`  | 使用 zlib 压缩旋转日志                  |

测试套件覆盖级别门控、格式化重载、文件写入与滚动、gzip 压缩、控制台输出、
并发写入与用户钩子，并额外在 `-fno-exceptions -fno-rtti`、
`-DDISABLE_XLOGGER` 以及 AddressSanitizer + UndefinedBehaviorSanitizer
下验证通过。

## 已知问题

- 非 Windows 平台（`wchar_t` 为 32 位）下 `std::wstring`/`std::u16string`/
  `std::filesystem::path` 重载的输出不正确，详见
  [docs/usage.md](docs/usage.md#7-已知问题)。
- `-DDISABLE_XLOGGER` 下 `VXLOG_*` 只支持 `VXLOG_F*` 形式。

## License

以 Boost Software License 1.0 分发，详见 [LICENSE](LICENSE)。
