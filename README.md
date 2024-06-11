# xlog
一个基于 c++20 的简单且功能齐全的日志库, 提供日志压缩, 支持异步日志写入, 对接 android log, systemd journal, windows debugview 等日志系统.


使用语法:

```c++
// 流式日志输出:
XLOG_DBG << "hello, " << "world";
XLOG_WARN << "hello, " << "world";
XLOG_ERR << "hello, " << "world";

// 支持 FMT 语法:
XLOG_FMT("{}, {}", "hello", "world");

// 甚至还可以:
XLOG_FMT("{}, {}", "hello", "world") << ", 你好, " << "世界!";
```

