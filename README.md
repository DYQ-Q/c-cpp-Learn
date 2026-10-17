# C/C++ 深度学习仓库

不是"语法速查表"，而是**以机制为主线**的 C 学习工作区：每个主题一个目录，目录里有笔记 + 可运行示例。

- 学习路线、阶段划分、书单、验收标准 → **[ROADMAP.md](ROADMAP.md)**
- 编译链接工具链（GCC/Make/CMake/gdb）→ **[gcc_g++_cmake.md](gcc_g++_cmake.md)**

## 目录结构

```
C_C++_Learn/
├── ROADMAP.md              # C 学习总路线（先读这个）
├── gcc_g++_cmake.md        # 工具链权威参考
├── C/
│   ├── C语言基础语法.md     # 基础速查（深入部分已指向主题目录）
│   ├── Basic.c
│   ├── 01_Pointer_And_Memory/ ... 13_Algorithm_SortSearch/   # Stage 1-3
│   ├── 14_Memory_Management/ ... 16_Debug_Tools/             # Stage 4
│   ├── 17_Concurrency/ 18_System_Programming/                # Stage 5
│   └── 19_Projects/                                          # Stage 6
└── C++/
    ├── C++基础语法.md
    └── Basic.cpp
```

每个主题目录内：**一个 `.md` 主笔记 + 若干 `.c` 示例 + 一个 `build.cmd`**（一键把该目录所有示例编到上一级的 `C\output\`，并顺手 `nm`/`-E` 给你看符号与展开结果）。产物目录 `output/` 与 `*.o`、`*.exe` 都已被 `.gitignore` 忽略。

## 环境（已在本机核实）

| 工具 | 版本 | 路径 |
|---|---|---|
| gcc / g++ | 15.2.0 (Rev11, MSYS2) | `D:\software_tech\MSYS2\mingw64\bin` |
| GNU Make | 4.4.1 (x86_64-w64-mingw32) | 同上（另有 `mingw32-make`） |
| CMake / Ninja | 4.2.3 / 1.13.2 | 同上 |
| GDB | 17.1 | 同上 |
| binutils（`ar/objdump/nm/size/strip`） | 2.45.1 | 同上 |
| cppcheck / clang / valgrind | **未安装** | — |

## 标准命令约定

```bat
:: 单文件调试构建（永远带 -Wall -Wextra -g）
gcc -std=c11 -Wall -Wextra -g ex01.c -o output\ex01.exe

:: 四步拆解（理解编译流程用，见 gcc_g++_cmake.md）
gcc -E -dD ex01.c -o output\ex01.i
gcc -S -O1 -std=c11 ex01.c -o output\ex01.s
gcc -c -g -std=c11 ex01.c -o output\ex01.o
gcc output\ex01.o -o output\ex01.exe

:: C++
g++ -std=c++17 -Wall -Wextra -g basic.cpp -o output\basic.exe
```

**`-std=` 必须显式写**。GCC 15 默认走新版标准（GNU23/C23 附近），不写标准号你会遇到"教程里能编译的代码在我这报错"，或反过来。

## 约定

1. 产物一律进 `output/`，中间文件 `*.o` 已被 `.gitignore` 忽略，不要提交。
2. 笔记里每段代码顶部标注**验证状态**：`待实测` 或 `已验证`。
3. 中文源码文件请存为 **UTF-8**；控制台乱码见 `gcc_g++_cmake.md` 的编码章节。
4. `system("chcp 65001 > null")` 这类写法**已确认有 bug**：Windows 的空设备是 `nul` 不是 `null`，实测会在当前目录创建一个真文件 `null`（本仓库已复现：执行前 `null` 不存在，执行后存在）。正确写法 `>nul`，或直接用 `SetConsoleOutputCP(CP_UTF8)`。

## 交付进度

| 批次 | 内容 | 状态 |
|---|---|---|
| 1 | ROADMAP、README、工具链笔记重写、基础语法重写、19 主题目录骨架 | ✅ 已提交 |
| 2 | Stage 1：指针 / 类型系统 / 作用域存储期链接 / 预处理宏（正文 + 12 个 `.c` 示例 + 4 个 `build.cmd`） | 正文完成，**示例待本地实测** |
| 3 | 字符串编码、文件 IO、链表、栈与队列 | 待开始 |
| 4 | 树、哈希、图、泛型容器 | 待开始 |
| 5 | 排序查找、内存管理、构建与库、调试工具 | 待开始 |
| 6 | 并发、系统编程、综合项目 | 待开始 |

第 2 批交付清单（每目录先跑 `build.cmd`，再把笔记里的 `待实测` 改成 `已验证`）：

```bat
cd C\01_Pointer_And_Memory    && cmd /c build.cmd      :: 指针算术 / 三种二维矩阵 / container_of+侵入式链表
cd C\02_Type_System          && cmd /c build.cmd      :: 提升与转换实测 / IEEE-754 行为
cd C\03_Scope_Storage_Linkage && cmd /c build.cmd     :: static 语义 / 三个 TU 的链接实验（含 nm 输出）
cd C\04_Preprocessor_Macro   && cmd /c build.cmd      :: X-macro / 宏陷阱 / _Generic（含 *.i 展开转储）
```
