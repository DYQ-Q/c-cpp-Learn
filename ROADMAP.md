# C 语言深度学习路线图

> 环境：Windows + MSYS2 MinGW-w64（`gcc/g++ 15.2.0`、`cmake 4.2.3`、`ninja 1.13.2`、`gdb 17.1`、`GNU Make 4.4.1`、`binutils 2.45.1`）。
> 标准：以 **C11** 为主线，标注 C17/C23 差异。目标不是"会写 C"，而是**知道每一行编译成什么、运行时在内存里是什么样**。

---

## 0. 三条硬约定

1. **只用命令行**。VS Code 只当编辑器 + gdb 前端，不用 MSVC、不用 IDE 一键运行。理由你要亲眼看到：
   `Hello World` 的 `.exe` 里，你的 `main` 只占极小一段，其余是 CRT 启动代码 + 静态/动态链接的库——这比任何文字都能说明"编译链接"是什么。
2. **每个示例必须实际编译运行过**。所有产物统一落到该主题目录**上一级**的 `output/`（例如 `C\01_Pointer_And_Memory\*.c` → `C\output\*.exe`，已被 `.gitignore` 忽略）。
   每篇笔记顶部给出标准编译命令，每个主题目录另有 `build.cmd` 一键编译该目录全部示例。
3. **未实测的结论一律标 `待实测`**。我这边的执行环境能调起 `gcc --version`，但**无法完成编译**（`cc1` 那层被沙箱挡住），所以代码是按标准写的，需要你在终端里跑一遍确认。跑通后把 `待实测` 改成 `已验证`。

---

## 1. 六阶段主线

### Stage 1 语言地基（第 2 批：正文 + 示例已交付，**待本地实测**）

| 目录 | 内容 | 第 2 批示例 |
|---|---|---|
| `C/01_Pointer_And_Memory` | 声明阅读法、指针运算与合法边界、数组退化、多级/函数指针与调用约定、`offsetof`/`container_of`、柔性数组、对象表示、三类内存错误与无 ASan 抓法 | `pointer_arithmetic.c` `dynamic_matrix.c` `container_of.c` |
| `C/02_Type_System` | 整型全景、整数提升、Usual Arithmetic Conversions 完整算法、溢出与除法、IEEE-754 与容差、字符类型家族、`const`/`volatile`、枚举、严格别名、UB 分级与"编译器如何利用 UB"、警告分层 | `promotion_and_overflow.c` `float_compare.c` |
| `C/03_Scope_Storage_Linkage` | 作用域/存储期/链接三维度、四种名字空间、tentative definition 与 `-fno-common`、`extern` 数组写法、C99 与 GNU `inline` 语义差异、全局状态四种写法、`nm` 符号表、链接错误排查 | `static_counter.c` `linkage_pair_main.c` `linkage_pair_a.c` `linkage_pair_b.c` |
| `C/04_Preprocessor_Macro` | 翻译八阶段、include 搜索与守卫、`-M/-MD` 依赖生成、`#if`/`__has_include`、跨平台宏判定、括号与 `do{}while(0)`、`#`/`##`/两级展开、变参与日志宏、`_Generic`、X-macro、Windows 宏（`NOMINMAX`/`UNICODE`/A-W 分派） | `xmacro_demo.c` `macro_traps.c` `generic_typing.c` |

**地基顺序为什么这样排**：预处理 → 编译(词法/语法/AST) → 优化 → 汇编 → 链接 → 加载 → 执行，
这条链上每一段出错的样子都不一样。先学会**看错误发生的位置**，再谈语法。

### Stage 2 数据结构（第 3、4 批）

线性：`05_String_Encoding` `06_File_IO` `07_DS_List` `08_DS_StackQueue`
非线性：`09_DS_Tree`(BST/AVL/堆/Trie) `10_DS_Hash` `11_DS_Graph`
抽象：`12_Generic_Container`——用 `qsort` 的函数指针 + `void*` 做通用容器，这是 C 唯一合法的"多态"

### Stage 3 算法（第 5 批）：`13_Algorithm_SortSearch`

### Stage 4 内存与工程（第 5 批）

`14_Memory_Management`（手写块式 malloc + 内存池 + 调试分配器）
`15_Build_Library`（多 TU、静态库/导入库、DLL 导出、Makefile、CMake）
`16_Debug_Tools`（gdb、objdump/nm/size、反汇编读法）

### Stage 5 系统与并发（第 6 批）：`17_Concurrency` `18_System_Programming`

### Stage 6 综合项目（第 6 批）：`19_Projects`

---

## 2. 里程碑项目

| 项目 | 依赖阶段 | 学到什么 |
|---|---|---|
| **mini-redis**（RESP 协议 + 事件循环） | 1+2+4+5 | socket、`epoll`/IOCP 差异、哈希表、序列化 |
| **memcached-lite**（slab 分配器） | 1+2+4 | 内存池、LRU、引用计数 |
| **cjson 重写** | 1+2 | 递归下降解析、柔性数组、内存所有权 |
| **极简 HTTP 静态服务器** | 全部 | 线程池、缓冲区、文件 IO、信号 |

---

## 3. 书单与用法

| 书 | 怎么用 |
|---|---|
| C Primer Plus / C 和指针 | 语法速查，别通读 |
| **C 专家编程** | 声明语法、数组指针、链接——**和 Stage 1 完全重合，优先** |
| 现代 C | 深入机制的入口 |
| C 陷阱与规范 | 每写完一篇笔记读一遍对应章节 |
| **C 语言参考（ISO 17.1 工作草案）** | 遇到争议直接查原文 |
| C 与面向对象系统设计 | Stage 2 的 `12_Generic_Container` 参考 |
| 算法：C语言模式 | Stage 3 主教材 |
| C 语言接口与调试 | Stage 4 的调试章 |
| UNIX 环境高级编程 | Stage 5 系统编程（POSIX 部分需 WSL） |
| C++ 并发编程实战 | Stage 5 并发（内存模型部分 C 通用） |
| 数据处理：缓存之道 | Stage 2/3 的复杂度直觉 |
| 编译系统：C 语言视角 | Stage 1 编译链接章 |
| 数据结构与算法分析 | Stage 2 补充 |

---

## 4. 阶段验收（能不看书写出来才算过）

- [ ] Stage 1：给出 `int (*a[5])(int)` 与 `int (*b)(int)` 的区别，能立刻说出各自类型并写出可运行用例
- [ ] Stage 1：解释为什么 `extern int n;` 在头文件里、`int n;` 只能出现在一个 `.c` 里
- [ ] Stage 2：手写单链表逆置、循环检测，不留内存泄漏
- [ ] Stage 2：用 `void*` + 函数指针实现一个能被 `int`/`double`/`char*` 共用的动态数组
- [ ] Stage 4：`objdump -d` 找到自己 `main` 的起始地址并读懂前 20 条指令
- [ ] Stage 4：说出静态库链接"按符号抽取"与动态库"运行时加载"的差别及后果
- [ ] Stage 5：解释 data race 与 race condition 的区别，并各举一个能复现的例子
