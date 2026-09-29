# 使用与 API 参考

本文档描述 xlog 的公开接口。所有符号位于 `xlogger` 命名空间，头文件
为 header-only：

```cpp
#include <xlog/logging.hpp>
```

编译需要 C++20（`std::format` 路径）或 `fmt` 库（`XLOG_USE_FMT=ON`
或工具链缺少 `std::format` 时）。

---

## 1. 日志级别

```cpp
enum logger_level__ {
    _logger_debug_id__,
    _logger_info_id__,
    _logger_warn_id__,
    _logger_error_id__,
    _logger_file_id__      // 仅写文件, 不打印控制台
};
```

| 接口 | 说明 |
| :--- | :--- |
| `void set_log_level(logger_level__)` | 设置全局最低输出级别，低于该级别的日志在构造时即被丢弃（不做格式化，无额外开销）。 |
| `logger_level__ logging_level()` | 读取当前最低级别。默认 `_logger_debug_id__`。 |
| `void turnon_logging()` / `void turnoff_logging()` | 日志总开关。关闭后所有日志为空操作。 |
| `void toggle_console_logging(bool)` | 控制台输出开关（默认 Debug 构建开启，Release 关闭）。 |
| `void toggle_console_logging_color(bool)` | 控制台 ANSI 颜色开关，默认开启。 |
| `void toggle_write_logging(bool)` | 文件写入开关，默认开启。 |
| `void set_logfile_maxsize(int64_t)` | 单文件大小上限，`<= 0` 表示按小时滚动；小于 10 MiB 的正值会被忽略。 |

例如：

```cpp
xlogger::set_log_level(xlogger::_logger_info_id__);
XLOG_DBG << "不会输出";                 // 被丢弃
XLOG_INFO << "会输出";
```

## 2. 日志宏

### 2.1 流式

```cpp
XLOG_DBG << "value = " << 42;           // debug
XLOG_INFO << "value = " << 42;          // info
XLOG_WARN << "value = " << 42;          // warn
XLOG_ERR  << "value = " << 42;          // error
XLOG_FILE << "value = " << 42;          // 只写文件
```

`operator<<` 支持内建类型、`std::string`/`string_view`、`std::pmr::string`、
宽字符串、`std::chrono` 各时长与日历类型、`std::thread::id`、
`std::filesystem::path`，以及在可用时的 `boost::string_view`/
`boost::filesystem::path`/`boost::posix_time::ptime`/
`boost::asio::ip::{tcp,udp}::endpoint`。

### 2.2 `fmt` 风格

```cpp
XLOG_FDBG("{} {}", "hello", 1);
XLOG_FINFO("{} {}", "hello", 2);
XLOG_FWARN("{} {}", "hello", 3);
XLOG_FERR("{} {}", "hello", 4);
XLOG_FFILE("{} {}", "hello", 5);
```

两种写法可以混用：

```cpp
XLOG_FINFO("{} + {} = {}", 1, 2, 3) << " (stream appended)";
```

### 2.3 异步

`AXLOG_*` / `AXLOG_F*` 与同步版本一一对应，但把消息投递到后台线程写盘，
适合热路径：

```cpp
AXLOG_INFO << "async " << 1;
AXLOG_FWARN("async {}", 2);
```

### 2.4 带位置信息的宏

`VXLOG_*` 在消息前追加 `(文件:行号): `，`AXVLOG_*` 是异步版本，
`AXVLOG_FMT`/`AXVLOG_IFMT`/`AXVLOG_WFMT`/`AXVLOG_EFMT`/`AXVLOG_FFMT`
为异步 + 位置信息的 `fmt` 版本：

```cpp
VXLOG_ERR << "something wrong";
AXVLOG_EFMT("{} failed: {}", "connect", ec.message());
```

> 注意：`-DDISABLE_XLOGGER` 时 `VXLOG_*` 只提供 `VXLOG_F*` 形式，流式
> 用法在禁用配置下会编译失败（见「已知问题」）。

## 3. 初始化与关闭

| 接口 | 说明 |
| :--- | :--- |
| `void init_logging(const std::string& path = "")` | 指定日志目录（默认 `./logs`），文件名为 `<LOG_APPNAME>.log`。 |
| `std::string log_path()` | 返回当前日志文件路径。 |
| `void shutdown_logging()` | 停止异步线程并排空队列。 |
| `struct auto_init_async_logger` | RAII：构造时 `init_logging()`，析构时 `shutdown_logging()`。 |

```cpp
int main()
{
    xlogger::init_logging("/var/log/myapp");
    xlogger::set_logfile_maxsize(64 * 1024 * 1024);

    XLOG_INFO << "written to " << xlogger::log_path();

    xlogger::shutdown_logging();
}
```

可以在函数内使用 `INIT_ASYNC_LOGGING()` 声明一个脚手架对象：

```cpp
int main()
{
    INIT_ASYNC_LOGGING();       // 退出时自动排空异步队列
    AXLOG_INFO << "hello";
}
```

> `init_logging()` 只对进程内第一次调用生效：日志写入器是单例。

## 4. 用户钩子

`tag_invoke` 可以把日志转接到自己的日志系统。勾子返回 `true` 表示该条
日志已被接管，不再写入文件/控制台：

```cpp
namespace xlogger {
    bool tag_invoke(logger_tag, int64_t time,
        const int& level, const std::string& message)
    {
        my_sink(time, level, message);
        return true;
    }
}
```

另有中断勾子，返回 `true` 时丢弃当前及后续日志：

```cpp
namespace xlogger {
    bool tag_invoke(logger_abort_tag)
    {
        return my_shutdown_flag;
    }
}
```

完整的可运行示例见 `examples/hook.cpp`。

## 5. 编译期开关

| 宏 | 作用 |
| :--- | :--- |
| `DISABLE_XLOGGER` | 所有日志宏退化为空操作。 |
| `FORCE_USE_FMT_FORMAT` | 强制使用 `{fmt}` 而不是 `std::format`。 |
| `DISABLE_WRITE_LOGGING` | 关闭文件写入（仍可输出控制台）。 |
| `LOGGING_DISABLE_COMPRESS_LOGS` | 关闭旋转日志的 gzip 压缩（默认在检测到 `zlib.h` 时开启）。 |
| `LOGGING_DISABLE_BOOST_*` | `FILESYSTEM`/`ASIO_ENDPOINT`/`POSIX_TIME`/`STRING_VIEW`，跳过对应的可选重载。 |
| `LOGGING_ENABLE_AUTO_UTF8` | Windows 下把非 UTF-8 字符串自动转换后再输出（默认开启）。 |
| `USE_SYSTEMD_LOGGING` | 同时输出到 systemd journal。 |
| `ENABLE_ANDROID_LOG` | 同时输出到 Android logcat（Android 默认开启）。 |
| `LOG_APPNAME` | 应用名，用于日志文件名与 Android tag，默认 `application`。 |
| `DEFAULT_LOG_MAXFILE_SIZE` | 默认单文件大小上限，默认 `-1`（按小时滚动）。 |

## 6. 集成方式

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
链接时按需提供 `-lfmt`、`-lboost_filesystem -lboost_system`、`-lz`。

## 7. 已知问题

- **非 Windows 平台的宽字符转换**：`logger_aux__::utf16_utf8` /
  `utf8_utf16` 按 UTF-16 处理输入，而 Linux/macOS 的 `wchar_t` 是 32 位，
  因此 `std::wstring`/`std::u16string`/`std::filesystem::path` 重载在
  这些平台上会产生内嵌 `\0` 或截断（Windows 下正常）。
  如需在 Linux 上输出路径，请先自行转换为 `std::string`。
- **`-DDISABLE_XLOGGER` 下的 `VXLOG_*`**：禁用配置只定义了函数式宏
  `VXLOG_DBG(...)`，因此 `VXLOG_DBG << "x"` 无法编译，请改用
  `VXLOG_FDBG("x")` 或非禁用构建。
- **`string_wide()` 依赖系统 locale**：非 UTF-8 locale 下非 ASCII 输入会
  转换失败并返回 `std::nullopt`。
- **MSVC 下非 ASCII 窄字符串字面量**：MSVC 默认按系统 ANSI 代码页转换窄
  字符串字面量，中文等字符会退化成 `?`（日志里表现为乱码问号）。请在
  编译选项中加入 `/utf-8`，或改用 `u8"..."` 字面量。
