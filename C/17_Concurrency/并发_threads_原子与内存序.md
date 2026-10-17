# 并发：threads、原子与内存序

> Stage 5 ｜ `C/17_Concurrency/` ｜ 计划：**第 6 批**（骨架，能力待实测）

## 一句话定位
并发 bug 不遵守"跑十次都对"的直觉。核心是**内存模型**：没有同步，编译器与 CPU 都可以合法重排你的代码。

## 待覆盖
- C11 `<threads.h>` vs POSIX `pthread` vs Win32 `CreateThread`（**MinGW 支持度待实测**）
- `<stdatomic.h>`：`atomic_int`、memory_order 六种、release/acquire 配对
- 互斥/条件变量的丢失唤醒(spurious wakeup)、死锁四条件、读写锁、自旋锁
- data race ≠ race condition，各给可复现例子
- 线程局部存储、生产者消费者、线程池
- Windows 侧：CRITICAL_SECTION、SRWLOCK、InterlockedXxx
