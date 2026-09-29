# 设计说明

本文说明 `logging.hpp` 的内部结构、线程模型、日志滚动与各后端的行为，
以及当前实现中的已知限制。

## 1. 总体结构

```
XLOG_INFO << ...            logger___(前端, 负责格式化与级别门控)
        |
        v
logger_writer__()           后端: 加锁 -> 用户钩子 -> 写文件/后端输出
        |
        +-- auto_logger_file__   (文件写入, 滚动, gzip 压缩)
        +-- logger_output_console__  (ANSI 彩色控制台 / OutputDebugString)
        +-- logger_output_systemd__  (USE_SYSTEMD_LOGGING)
        +-- logger_output_android__  (__ANDROID__)

AXLOG_*  --------------> async_logger___(后台线程 + 无界队列) --> logger_writer__
```

- **前端 `logger___`**：临时对象，`operator<<` 与 `format_to()` 把内容写进
  内部 `std::string out_`，析构时交给后端。
- **后端 `logger_writer__()`**：进程级单例写入点。同步日志在调用线程执行；
  异步日志在后台线程执行。两者共用同一把锁，因此输出顺序与加锁顺序一致。
- **异步队列**：`std::deque<internal_message>` + `std::condition_variable`，
  无界、不丢消息；线程在 `stop()`/析构时排空后退出。

## 2. 级别门控

`logger___` 构造时判断：

```cpp
if (disabled() || level_filtered(level_))
    ignore_ = true;
```

- `level_filtered()`：`level < global_logging_level___`（`std::atomic`，
  `memory_order_relaxed`）。
- `disabled()`：总开关关闭或本条已被标记忽略。

被忽略的对象后续所有 `operator<<`/`format_to()` 立即返回，**不产生任何
格式化开销**——这是把热路径日志留在代码里的前提。

## 3. 文件写入与滚动

`auto_logger_file__` 直接用 OS 级文件描述符（`open`/`write`）追加写入，
避免 `std::ofstream` 的缓冲与异常：

- 打开：`O_WRONLY | O_CREAT | O_APPEND`（跨进程/多次打开均安全）。
- 滚动条件：
  - `set_logfile_maxsize(n)` 且 `n > 0`：累计写入超过 `n` 字节时滚动；
  - `n <= 0`（默认）：跨小时滚动一次。
- 滚动流程：关闭句柄 → 把 `application.log` 原子 `rename` 为
  `YYYYMMDD-<时间戳>.log` → 重新打开 `application.log` → 后台线程把旧文件
  压缩为 `.gz` 并删除原文件（`LOGGING_ENABLE_COMPRESS_LOGS` 时）。
- 压缩受全局互斥保护（`compress_lock()`），避免多个滚动文件同时压缩。

## 4. UTF 处理

`logger_aux__::utf` 提供三套实现，按平台选择：

| 平台 | `string_wide` | `utf8_utf16` / `utf16_utf8` |
| :--- | :--- | :--- |
| Windows | `MultiByteToWideChar(CP_ACP)` | `WideCharToMultiByte(CP_UTF8)` |
| Android | 逐字节拷贝 | 手写 UTF-8 ↔ UTF-32 转换（bionic 不导出 `codecvt` 特化符号） |
| 其它 | 系统 locale + `codecvt<wchar_t, char>` | `std::codecvt<char16_t, char8_t>` |

其它平台（Linux/macOS）的 `wchar_t` 是 32 位，而 `utf8_utf16` 按 UTF-16
处理输入，因此宽字符串/路径重载在这些平台上结果不正确（见
`docs/usage.md` 的「已知问题」）。

## 5. 崩溃与信号

`async_logger___` 构造时注册信号处理（`SIGTERM`/`SIGABRT`/`SIGFPE`/
`SIGSEGV`/`SIGILL`），Windows 下再挂 `SetUnhandledExceptionFilter`。
处理方式是释放全局异步对象，析构会排空队列，从而尽量避免崩溃时丢日志。

## 6. 线程安全

- 各级别开关是 `inline` 全局变量；日志级别用 `std::atomic`。
- `logger_writer__()` 全程持锁（`DISABLE_XLOGGER_THREAD_SAFE` 可关闭），
  用户钩子同样在锁内执行。
- 异步队列由 `internal_mutex` 保护。

## 7. 后续工作

- 统一非 Windows 平台的宽字符转换（按 `sizeof(wchar_t)` 选择 UTF-16/
  UTF-32 实现），并把 `wstring`/`u16string`/`path` 重载纳入测试。
- `DISABLE_XLOGGER` 下把 `VXLOG_*` 改为对象式宏，保持与启用配置一致的
  用法。
- 异步队列可考虑加上限与丢弃策略，避免极端情况下内存无界增长。
