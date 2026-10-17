# GCC / G++ / Make / CMake —— Windows(MinGW-w64) 工具链参考

> 本机已核实：`gcc/g++ 15.2.0 (Rev11, MSYS2 project)`、`GNU Make 4.4.1 (Built for x86_64-w64-mingw32)`、`CMake 4.2.3`、`Ninja 1.13.2`、`GDB 17.1`、`binutils 2.45.1`（`ar/objdump/nm/size/strip`），全部位于 `D:\software_tech\MSYS2\mingw64\bin`。
> `cppcheck`、`clang`、`valgrind` **未安装**。
> 标 `待实测` 的条目是我按标准与工具链常识写的，需要你在终端跑一遍确认后改成 `已验证`——我这边的执行环境无法调用 `cc1`，编译不了。

---

## 1. gcc 与 g++：两个"驱动"，不是一个编译器

```text
gcc  ：编译器驱动（driver）。看到 .c 调 cc1，看到 .cpp 也**按 C++ 处理**，
       但链接时**不自动链 C++ 标准库**。
g++  ：同一个驱动，但默认多链 libstdc++ / libgcc，并把 .cpp 当 C++。
```

用 `gcc` 编译 C++ 的典型后果（这就是"为什么必须用 g++"）：

```text
undefined reference to `std::cout'
undefined reference to `operator new(unsigned long long)'
```

等价的手工补救（理解用，不要这么写）：

```bat
g++ -c main.cpp -o output\main.o
gcc output\main.o -o output\main.exe -lstdc++ -lm
```

`.c` 和 `.cpp` 的后缀**决定按哪种语言解析**，和用 gcc 还是 g++ 无关：

| 后缀 | 语言 | 说明 |
|---|---|---|
| `.c` | C | `gcc a.cpp` 会按 C++ 编；`g++ a.c` 也按 C++ 编（这是常见困惑源） |
| `.cc .cpp .cxx` | C++ | |
| `.i` | C 预处理产物 | 直接喂给编译器会跳过预处理 |
| `.ii .s .o .a .lib .dll` | C++预处理 / 汇编 / 目标 / 库 / 动态库 | |

强制指定语言：`gcc -x c file.txt`、`gcc -x c++ -` （`-` 表示 stdin）。

---

## 2. 编译四阶段与"亲眼看到每一段"

```text
main.c ──预处理──▶ main.i ──编译──▶ main.s ──汇编──▶ main.o ──链接──▶ main.exe
        gcc -E           gcc -S          gcc -c          gcc
```

在 Windows 上（`output` 目录必须先存在，否则 `gcc` 报 `No such file or directory`——**报错指的是输出目录，不是源文件**，这是新手最常踩的第一个坑）：

```bat
mkdir output
gcc -E main.c -o output\main.i        :: 只有宏展开和 #include 结果，无 C 代码逻辑
gcc -S -O1 main.c -o output\main.s    :: AT&T 语法汇编，看代码生成
gcc -c -g main.c -o output\main.o     :: 目标文件，含重定位表和未定义符号表
gcc output\main.o -o output\main.exe  :: 这一步才调用 ld
```

观察每一段的正确姿势：

```bat
:: 预处理：看宏定义 + 真正进了哪些头文件
gcc -E -dD main.c -o output\main.i
gcc -MM -MT main.o main.c              :: 打印该源文件的头文件依赖（Makefile 用）

:: 目标文件里有哪些符号：U = 未定义（等链接器解决）
nm output\main.o
objdump -t output\main.o               :: 符号表
objdump -h output\main.o               :: 段表（.text/.data/.bss/.rdata）

:: 整链接过程都调用了谁（排查库找不到最有用的一条）
gcc -v -o output\main.exe main.c
```

**`Hello World` 到底有多大**：`gcc -static -o output\hello.exe hello.c` 之后 `size output\hello.exe`，`.text` 会有几百 KB——你的 `main` 只占其中几十条指令，剩下全是 CRT 启动代码和静态进去的 libc。这条比任何解释都能说明"链接"是什么。`待实测`。

---

## 3. 标准与警告：这两件事必须每天做

```bat
gcc -std=c11 -Wall -Wextra -g -o output\main.exe main.c
```

| 选项 | 作用 |
|---|---|
| `-std=c11` / `c17` / `c23` / `gnu11` | 语言标准。`gnuXX` 版本额外开 GCC 扩展（如 `__int128`、案例区间） |
| `-Wall` | 一组常见警告，**不等于"所有警告"** |
| `-Wextra` | 再加一批（如比较有符号/无符号、未使用参数） |
| `-Werror` | 警告即报错。学习期建议开，逼自己读懂每条警告 |
| `-g` | 生成 DWARF 调试信息，`gdb` 需要它 |
| `-pedantic` | 拒绝非标准扩展，学"标准 C"时有用 |
| `-fsyntax-only` | **只做语法检查，不产出文件**。快速验证头文件写错没 |
| `-Wshadow -Wconversion -Wsign-conversion` | 强烈建议加进日常组合，能提前抓出隐式窄化 |

GCC 15 的默认 C 标准已经切到 **GNU23**（`__STDC_VERSION__` 为 `202311L`）。不写 `-std=` 你会碰到"教程代码在我这报错"。**别信文档里的默认值，自己查**：

```bat
echo int main(void){return 0;} > _std_probe.c
gcc -dM -E _std_probe.c | findstr __STDC_VERSION__
del _std_probe.c
```

C++ 同理：`g++ -dM -E -x c++ _std_probe.c | findstr __cplusplus`（GCC 14 起默认 `gnu++17`）。

一条建议的固定组合（写进你的 muscle memory）：

```bat
gcc -std=c11 -Wall -Wextra -Wshadow -Wconversion -g -o output\ex.exe ex.c
```

---

## 4. 优化，以及"优化之后为什么没法调"

```text
-O0  不优化（默认）        -g 配套调试用这个或 -Og
-Og  只保留可调试性的优化  ← 学习期最推荐
-O1  基本优化
-O2  工程默认，推荐发布
-O3  更激进，可能变慢、体积变大
-Os  体积优先
-Ofast  关闭严格标准合规（浮点），学习期不要用
```

为什么 `-O2` 下 `gdb` 会"胡说八道"：变量被消进寄存器或直接消失、循环被展开、函数被内联导致行号跳来跳去、`printf` 被改写成 `puts`。所以：

```bat
gcc -std=c11 -Og -g -o output\ex.exe ex.c      :: 调试
gcc -std=c11 -O2 -o output\ex.exe ex.c         :: 测性能
```

**测性能必须用 `-O2`**。用 `-O0` 测出来的排序耗时对比毫无意义（未优化时函数调用开销会淹没真实差异）。

其它高频：`-funroll-loops`、`-march=native`（本机指令集，注意换机可能非法指令）、`-flto`（链接期优化，跨 TU 内联）、`-fno-omit-frame-pointer`（崩溃时栈回溯更完整）。

---

## 5. 多文件项目：一条铁律 + `-I` 的顺序

铁律：**声明可以出现多次，定义必须只出现一次**（ODR）。这条在 C 里的具体形态是"头文件放 `extern 声明`，某个 `.c` 放定义"，详见 `C/03_Scope_Storage_Linkage/`。

```text
mylib/
├── include/
│   └── vector.h        :: 只放 extern 声明、类型、static inline
├── src/
│   ├── vector.c        :: 定义
│   └── pool.c
└── app/main.c
```

```bat
gcc -std=c11 -Wall -Wextra -Iinclude -c src\vector.c -o output\vector.o
gcc -std=c11 -Wall -Wextra -Iinclude -c src\pool.c   -o output\pool.o
gcc -std=c11 -Wall -Wextra -Iinclude -c app\main.c   -o output\main.o
gcc output\main.o output\vector.o output\pool.o -o output\app.exe
```

`-I` 的三条要点：

1. **顺序有意义**：同名头文件被多个 `-I` 目录覆盖时，先出现的胜出。
2. `#include "x.h"` 先从**当前文件所在目录**找，再从 `-I` 找，最后系统目录；`#include <x.h>` 跳过当前目录。所以头文件之间用 `<>`/`""` 不是风格问题，是**搜索路径问题**。
3. 别在头文件里写 `-I.` 能解决的 hack（如 `#include "../src/x.h"`），那是目录结构出了问题。

**改了头文件为什么没重编**：手写 `.o` 依赖不会跟踪头文件。让 GCC 自己生成依赖：

```bat
gcc -std=c11 -MMD -MP -Iinclude -c app\main.c -o output\main.o
```

- `-MMD`：生成 `main.d`（依赖列表），但只含用户头文件（不含系统头）
- `-MP`：为每个头文件加一个空规则，**防止删掉某个头文件后 make 报 "No rule to make target"**

这两个标志是 Makefile 自动依赖的核心，见第 8 节。

---

## 6. 静态库：一个归档文件 + "按符号抽取"

```bat
gcc -std=c11 -Wall -c src\add.c -o output\add.o
gcc -std=c11 -Wall -c src\sub.c -o output\sub.o
ar rcs output\libmathy.a output\add.o output\sub.o

gcc -std=c11 app.c -o output\app.exe -Loutput -lmathy
```

`ar` 四个字母：`r` 插入/替换，`c` 不存在则创建（**不加会警告**），`s` 写索引（不加则链接器找不到成员，`-l` 会失败）。查看内容：

```bat
ar t output\libmathy.a        :: 列出成员
ar x output\libmathy.a        :: 解出 .o
nm output\libmathy.a          :: 每个成员提供的符号
```

**链接器对静态库的态度是"按需抽取"**：它从左到右扫命令行，只有当某个 `.a` 里的成员能解决**当前已经悬空的符号**时，才把这个成员拽进来。由此产生三个必须记住的结论：

```bat
gcc app.c -o app.exe -Loutput -lmathy        :: ✅ 正确：先有未定义符号，再给库
gcc -Loutput -lmathy app.c -o app.exe        :: ❌ 链接时 mathy 里没有待解决的符号 → 被整个跳过
```

1. **库要放在使用它的目标文件后面**（这是 Linux/MinGW 共通的规则）。
2. 循环依赖（A 需要 B、B 又需要 A）用 `-Wl,--start-group ... -Wl,--end-group` 包住，代价是反复扫描。
3. 静态库的体积会**永久进入你的 exe**，且库更新后你必须重新链接 exe（`.a` 换了不重链，跑的还是老代码）。

---

## 7. 动态库（Windows 主战场）：DLL、导入库、搜索顺序

MinGW 生成 DLL：

```bat
:: 方法 A：源码里标 dllexport
gcc -shared -o output\mylib.dll src\add.c src\sub.c ^
    -Wl,--out-implib,output\libmylib.dll.a

:: 方法 B：不改源码，用 .def 导出文件
gcc -shared -o output\mylib.dll src\add.c -Wl,output\mylib.def
```

导出宏的**正确写法**（一份头文件同时服务"编译库"和"用库"两侧）：

```c
/* mylib_api.h */
/* 顺序很重要：静态库必须最先判断，否则同时定义两个宏时会错误导出 */
#if defined(MYLIB_STATIC)
#  define MYLIB_API
#elif defined(MYLIB_BUILD)        /* 只有编译 DLL 本体时才 dllexport */
#  define MYLIB_API __declspec(dllexport)
#else                              /* 默认：别人在用我的库 → dllimport */
#  define MYLIB_API __declspec(dllimport)
#endif

MYLIB_API int add(int, int);
```

调用侧：

```bat
gcc -std=c11 app.c -o output\app.exe -Iinclude -Loutput -lmylib
output\app.exe
```

`-lmylib` 在 MinGW 下会**优先找 `libmylib.dll.a`（导入库），其次找 `libmylib.a`（静态库）**——所以同目录下两个都放会导致"我以为在链动态库其实链了静态库"。要么改名区分，要么用 `-l:精确文件名` 语法（`-l:libmylib.dll.a`）。

**`libmylib.dll.a` 里没有代码**，它只有"符号在哪"的跳转桩。真正的代码在 `mylib.dll`。所以：编译链接全成功、一运行就报"找不到 mylib.dll"——这是 Windows 上最典型的动态库错误。

DLL 运行时搜索顺序（简化）：

```text
1. 加载者(exe)所在目录            ← 最省事：把 dll 和 exe 放一起
2. System32
3. PATH 里的目录（顺序敏感）
```

注意：**当前工作目录不算**（SafeDllSearchMode 默认开启）。在 MSYS2 里能跑、双击 exe 报缺 `libwinpthread-1.dll` / `libgcc_s_seh-1.dll` / `libstdc++-6.dll`，就是这个原因——你的 PATH 里有 `mingw64\bin`，别人的没有。

**要发给别人一个不依赖 MSYS2 的 exe**：

```bat
gcc -std=c11 -static -o output\app.exe app.c -Loutput -lmylib
```

`-static` 会把 libgcc/libstdc++/winpthreads 全静态进去（C++ 体积涨几 MB 很正常）。想只静态 C++ 相关而保留 msvcrt：`-static-libgcc -static-libstdc++`。验证是否还有外部 DLL 依赖：

```bat
objdump -p output\app.exe | findstr "DLL Size"
```

Linux/WSL 对应写法（**Windows 上无效，别混用**）：

```bash
gcc -fPIC -c add.c -o add.o && gcc -shared -o libadd.so add.o
LD_LIBRARY_PATH=$PWD ./app        # Windows 没有 LD_LIBRARY_PATH，Windows 用 PATH
```

x86-64 Windows 上 `-fPIC` **不是必需**（该 ABI 本身就位置无关），旧笔记里"动态库必须 -fPIC"在 MinGW 下是误导；跨平台项目加上无害，但别把它当成功能开关。

---

## 8. Makefile（本机 `make` 就是 GNU Make 4.4.1，不是只有 mingw32-make）

最小可用、且**自动跟踪头文件依赖**的 C 版 Makefile：

```makefile
CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -Wshadow -g -Iinclude -MMD -MP
SRCDIR  := src
OBJDIR  := output
BIN     := $(OBJDIR)/app.exe

SRCS    := $(wildcard $(SRCDIR)/*.c)
OBJS    := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))
DEPS    := $(OBJS:.o=.d)

.PHONY: all clean run

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	-@mkdir -p $(OBJDIR)

-include $(DEPS)

clean:
	-@rm -f $(OBJDIR)/*.o $(OBJDIR)/*.d $(BIN)

run: all
	./$(BIN)
```

必须知道的四件事：

1. ** recipe 行开头必须是 TAB**，不能是空格。这是 Makefile 最古老也最致命的语法陷阱（VS Code 里请对该文件关掉"缩进转空格"）。
2. 自动变量：`$<` 第一个依赖、`$@` 目标、`$^` 所有依赖（去重）、`$*` 模式匹配部分。
3. `| $(OBJDIR)` 是**顺序先决条件**：只保证目录先建好，目录时间戳变化不触发重编。
4. `-include $(DEPS)` 让 `.d` 里的头文件依赖生效；前面加 `-` 是因为首次构建时 `.d` 还不存在。

在纯 cmd（没有 `rm`/`mkdir -p`）下跑 make 会失败。两个选择：

```makefile
clean:
	-if exist $(OBJDIR)\*.o del /q $(OBJDIR)\*.o $(OBJDIR)\*.d
```

或者**统一在 MSYS2 shell 里跑 `make`**（推荐，能用 `-rm`、`mkdir -p`、`./app`）。若坚持在 cmd 里构建，用 `mingw32-make`，它专为 Windows shell 而生（`mingw32-make -f Makefile.win`）。

并行构建与排查：

```bat
make -j8                 :: 并行，注意 -j 会打乱错误输出顺序
make -n                  :: 只打印将要执行的命令，不执行（调试规则写得对不对）
make --debug=b           :: 为什么目标没重建
```

---

## 9. CMake：它是"生成器"，不是构建器

心智模型：`CMakeLists.txt` → CMake 生成 → `Makefile` 或 `build.ninja` → 由 **make/ninja** 真正编译。所以 CMake 报错和编译器报错是两个完全不同的阶段。

标准 C 项目写法（含静态库、动态库、可执行三段）：

```cmake
cmake_minimum_required(VERSION 3.16)
project(mathy LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)          # 等价于 -std=c11 而非 gnu11

add_compile_options(-Wall -Wextra -Wshadow)

# 目标名必须唯一：STATIC 和 SHARED 用两个不同的名字
add_library(mathy_static STATIC src/add.c src/sub.c)
add_library(mathy_shared SHARED src/add.c src/sub.c)
set_target_properties(mathy_shared PROPERTIES OUTPUT_NAME "mylib")

# 谁用这个库，就自动带上 include/ —— 这是 PUBLIC 的含义
target_include_directories(mathy_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(mathy_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 只有编译 DLL 本体时定义 MYLIB_BUILD；静态版自己和使用它的人都要看到 MYLIB_STATIC
target_compile_definitions(mathy_static PUBLIC  MYLIB_STATIC)
target_compile_definitions(mathy_shared PRIVATE MYLIB_BUILD)

add_executable(app app/main.c)
target_link_libraries(app PRIVATE mathy_static)   # 想改用动态库：只换这一行
```

`PUBLIC/PRIVATE/INTERFACE` 三个关键词是 CMake 最难也最有价值的部分，一句话记住：**`PUBLIC` = 我自己要用，用了我的人也要用；`PRIVATE` = 只有我自己用；`INTERFACE` = 我不需要，但用了我的人需要**（典型：头文件-only 库）。

Windows/MinGW 的生成器选择——**这是 Windows 上和 Linux 差别最大的一步**。Linux 下 `cmake ..` 直接默认 Unix Makefiles；Windows 下 CMake 会优先找 Visual Studio，没装 VS 又没指定生成器就会失败或挑错：

```bat
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build -- -j 8

:: 或者用 Ninja（更快，你本机 1.13.2 可用）
cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
cmake --build build

:: 常用辅助
cmake --build build --target clean
cmake --build build --verbose          :: 打印真实编译器命令行（排错第一件武器）
cmake -S . -B build --fresh            :: 彻底重来，清掉缓存里的旧编译器探测结果
```

改了编译器却"没生效"，几乎一定是 `build/CMakeCache.txt` 里缓存了旧的编译器路径 → `--fresh` 或删掉整个 `build` 目录。

三个高频坑：

1. **CMake 4.x 不再兼容 `cmake_minimum_required(VERSION 3.10)` 以下的旧项目**，报 `Compatibility with CMake < 3.5 has been removed`。网上大量教程的 `3.10` 在 CMake 4 下会给 deprecation 警告，建议一律写 `3.16`+。
2. 生成器名写错：Windows 上是 `"MinGW Makefiles"`（**有空格、要引号、区分大小写**），不是 `MinGW`。用 `cmake -S . -B build --help` 可列出本机可用生成器。
3. `build` 目录里产物路径随生成器变化（Ninja 直接在 `build/`，MinGW Makefiles 也在 `build/`，VS 会多一层 `Debug/`）。写脚本时别硬编码，用 `$<TARGET_FILE:app>` 或 `--build` 后 `cmake -LAH` 查看。

---

## 10. GDB：`-g` 是前提，`run` 之前先设断点

```bat
gcc -std=c11 -Wall -Wextra -Og -g -o output\bug.exe bug.c
gdb output\bug.exe
```

```gdb
(gdb) set pagination off          # 关掉 --Type <RET>-- 打断，强烈推荐第一行就设
(gdb) break 23                    # 行号断点；或 break parse_line
(gdb) run arg1 arg2               # 带参数启动
(gdb) next                        # 单步（不进函数）
(gdb) step                        # 进入函数
(gdb) finish                      # 跑到当前函数返回
(gdb) continue                    # 放行
(gdb) print/x count              # 十六进制；print/t 二进制；print/x &buf 地址
(gdb) print arr                   # 数组；print *arr@10 显示前 10 个元素
(gdb) x/16gx $rsp                 # 看内存：16 个 unit，格式 g(8字节)，x=hex
(gdb) display count              # 每次停下自动打印
(gdb) info locals / info args / info registers / info frame
(gdb) backtrace                   # bt；bt full 带局部变量；frame 3 切栈帧
(gdb) watch count                 # 数据断点：谁改了它（找"值哪来的"神器）
(gdb) break main if i==42         # 条件断点
(gdb) commands / silent / continue  # 断点上自动执行 = 断点式打印
(gdb) attach <pid> / detach
(gdb) catch throw                 # C++ 异常抛出点
```

C 里最常做的一件事：程序 **段错误** 了。

```gdb
(gdb) run
Program received signal SIGSEGV, Segmentation fault.
(gdb) bt                # 直接告诉你死在哪个函数的哪一行
(gdb) frame 2
(gdb) print p           # p = 0x0 → 空指针；p = 0xfeeefeee / 0xdddddddd → 用了已释放内存
```

**GDB 里没有源文件行**（显示 `??` 或只有汇编）：三件事挨个查——编译有没有 `-g`、是不是在 `-O2` 下（换 `-Og`）、`.exe` 是不是重新编过而 GDB 还挂着旧符号。

VS Code 里用 gdb 需要 `launch.json` 指定 `miDebuggerPath`：

```json
{
  "name": "C: gdb launch",
  "type": "cppdbg",
  "request": "launch",
  "program": "${workspaceFolder}/output/ex.exe",
  "args": [],
  "stopAtEntry": false,
  "cwd": "${workspaceFolder}",
  "MIMode": "gdb",
  "miDebuggerPath": "D:/software_tech/MSYS2/mingw64/bin/gdb.exe",
  "setupCommands": [
    { "description": "关分页", "text": "-gdb-set pagination off", "execute": true }
  ],
  "preLaunchTask": "build-ex"
}
```

`preLaunchTask` 要在 `tasks.json` 里定义，否则 VS Code 会调试**上一次**的旧 exe——这种"我明明改了代码"的错觉能浪费你一整个下午。

---

## 11. 用 binutils 读产物：把黑盒打开

```bat
size output\ex.exe                 :: 各段大小：text/data/bss。bss 不占文件体积
objdump -h output\ex.exe           :: 段表 + 文件偏移
objdump -t output\ex.exe           :: 符号表（本地符号）
nm -C output\ex.exe                :: 符号；T=text, D=data, B=bss, U=undefined, t=local
objdump -d -M intel output\ex.exe > output\ex.asm    :: 反汇编（Intel 语法好读）
objdump -p output\ex.exe           :: PE 头：DLL 依赖表就在这里
strings? 未核实 → 用 findstr 代替：findstr /i "error" output\ex.asm
strip -s output\ex.exe -o output\ex_min.exe          :: 去符号，体积对比体会 -g 有多大代价
```

`nm` 的字母大小写含义：**大写=外部链接，小写=静态/局部**。`U` 就是"等链接器给"，它出现在 `.o` 里正常，出现在最终 exe 里就是错误。

**读 `main` 的反汇编**（`16_Debug_Tools` 的验收项）：

```bat
objdump -d -M intel output\ex.exe | findstr /A:20 "<main>"
```

看到的关键模式：`push rbp / mov rbp,rsp`（栈帧建立）、`call puts`（printf 被优化成 puts）、`mov eax,[rbp-4]`（局部变量在负偏移处）、`leave / ret`。

---

## 12. 常见报错：按"发生在哪一层"分类

看报错第一件事：**判断它死在哪个阶段**。`fatal error: ...: No such file or directory` 是预处理；`undefined reference` 是链接；`error:` 带行列号是编译；启动就崩是运行时/加载。

### 12.1 预处理/编译期

```text
fatal error: myheader.h: No such file or directory
```
`-I` 加路径；确认相对路径是相对**当前工作目录**还是源文件目录（`""` 的搜索以源文件目录为先）。

```text
error: 'for' loop initial declarations are only allowed in C99 or C11 mode
```
GCC 15 默认已是 C23，这条只在老 GCC 上出现。写 `-std=c11` 是好习惯的根本原因。

```text
warning: implicit declaration of function 'foo' [-Wimplicit-function-declaration]
```
**这条警告必须当错误看**：C 里没声明就调用，编译器假设它返回 `int`，于是链接时可能报 `undefined reference`，或者返回 `int64_t`/指针时被静默截断。99% 是忘了 `#include` 或函数名拼错。

```text
warning: initialization of 'int' from 'char *' makes integer from pointer without a cast
```
典型：`char *s = malloc(10); int n = s;` 之类的类型串线。

### 12.2 链接期（MinGW 上最耗时的一类）

```text
undefined reference to `add'
```
四选一：① 忘把 `.c`/`.o` 放进命令；② 库写在使用者**前面**（见 6 节）；③ `-L` 路径不对（`gcc -v` 看它去哪找了）；④ C++ 里函数名被 mangle，需要用 `extern "C"` 包住声明。

```text
multiple definition of `counter'; output/a.o: first defined here
```
**在头文件里写了定义**：`int counter = 0;` 放进 `.h`，每个包含它的 `.c` 都生成一份。改成头文件 `extern int counter;` + 某一个 `.c` 里 `int counter = 0;`。C99 `inline` 也是重灾区，见 `C/03_Scope_Storage_Linkage/`。

```text
undefined reference to `WSAStartup@8'
```
Windows 特有：网络 API 在 `ws2_32`。加 `-lws2_32`。同类：`-lwinmm`、`-lgdi32`、`-lole32`。

```text
cannot find -lmylib
```
MinGW 找的是 `libmylib.a` / `libmylib.dll.a` ——**`lib` 前缀必须存在**。文件名不对，或 `-L` 没给对（相对路径相对当前目录）。

```text
collect2.exe: error: ld returned 1 exit status
```
不是独立错误，它只是"上面那条 ld 错误"的收尾。**向上翻找第一条真正的错误**。同理 `cannot open output file ... : No such file or directory` 通常是 `output\` 目录不存在。

```text
undefined reference to `WinMain@16'
```
链接器把你当成了 GUI 程序。原因：没有 `main` 函数（常见于 `.c` 文件其实没被加进来，或文件名后缀是 `.cpp` 而你当 C 处理）。

### 12.3 运行期

```text
由于找不到 XXX.dll，无法继续执行代码
```
见第 7 节搜索顺序。开发期最省事：把 dll 拷到 exe 同目录。

```text
The code execution cannot proceed because LIBWINPTHREAD-1.dll was not found
```
你用了 winpthreads（`-pthread`）却静态分发失败 → 重编加 `-static`，或连同这几个 dll 一起发。

### 12.4 内存与未定义行为（学习期最该重视）

GCC/MinGW 上 `-fsanitize=address` 与 `-fsanitize=undefined` **理论上可用（MSYS2 mingw64 打包了 sanitizer 库），但我在本机没能实测**——请你自己跑一遍确认，命令：

```bat
gcc -std=c11 -g -fsanitize=address bug.c -o output\bug_ansi.exe 2>&1 && output\bug_ansi.exe
```

若报 `unable to find libasan`，退而求其次用我将在 `C/14_Memory_Management/` 里手写的**调试分配器**（红色区域 + 影子值 + 泄漏表），它不依赖任何 sanitizer，学习价值反而更高。`gdb` 的 `watch` 断点也是抓越界写的好工具。

---

## 13. 编码：你的中文注释和乱码到底怎么回事

三个不同的字符集，**任何一个搞混就乱码**：

```text
源文件字符集  --(-finput-charset)-->  编译器内部(UTF-8/宽字符)
              --(-fexec-charset)-->   执行字符集（字面量在 .exe 里的字节）
              --控制台代码页-->        屏幕上你看到的
```

推荐组合（**本机实测配置，写死进你的习惯**）：

1. 源文件存为 **UTF-8（无 BOM）**。VS Code 右下角确认；GCC 对 BOM 容忍但 C23 前不保证。
2. 告诉编译器源文件编码，防止它按 GBK 猜：

```bat
gcc -std=c11 -finput-charset=UTF-8 -fexec-charset=UTF-8 -Wall -g main.c -o output\main.exe
```

3. 让控制台按 UTF-8 解释（**只影响当前会话**，且需 TrueType 字体如 `Cascadia Mono`/`NSimSun`）：

```bat
chcp 65001
output\main.exe
```

4. **不要在 C 代码里 `system("chcp ...")`**。原因一是这条命令在你现有代码里有实锤 bug：

```c
system("chcp 65001 > null");   /* ❌ 实测会在当前目录创建一个真文件名为 null 的 0 字节文件 */
system("chcp 65001 >nul");     /* ✅ Windows 的空设备叫 nul（没有 l） */
```

我这边的执行结果（可自己复现）：执行 `cmd /c "chcp 65001 > null"` 前 `Test-Path .\null` 为 `False`，执行后为 `True`。这个文件被写进仓库过一次，就是你之前 `.gitignore` 里 `null` 那条的来历。

二是 `system()` 会**启动 cmd.exe**，慢、依赖 shell、还可能被 PATH 影响。正确做法是调 API：

```c
#include <windows.h>
int main(void) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);          /* 若还要读中文输入 */
    printf("中文正常输出\n");
    return 0;
}
```

纯标准 C 的替代方案：把中文输出交给 `fwrite` 原始字节 + 保证执行字符集是 UTF-8；或用 `wchar_t` + `wprintf`（**Windows 的 `wchar_t` 是 2 字节 UTF-16，POSIX 是 4 字节**，跨平台要小心，详见 `C/05_String_Encoding/`）。

`%s` 打印中文乱码但注释正常 → 执行字符集或代码页不一致；注释也乱 → 源文件本身被按错编码打开过（已损坏，需重存）。

---

## 14. 速查（Windows 版）

```bat
:: 编译运行一条龙
gcc -std=c11 -Wall -Wextra -g -o output\ex.exe ex.c && output\ex.exe

:: 四阶段
gcc -E -dD ex.c -o output\ex.i
gcc -S -O1 -std=c11 ex.c -o output\ex.s
gcc -c -g -std=c11 ex.c -o output\ex.o
gcc output\ex.o -o output\ex.exe

:: 查默认标准 / 查都链了什么
echo int main(void){return 0;}>_p.c && gcc -dM -E _p.c | findstr __STDC_VERSION__ && del _p.c
gcc -v -o output\ex.exe ex.c 2>&1 | findstr "-lgcc"

:: 静态库
ar rcs output\libmy.a a.o b.o
gcc app.c -o output\app.exe -Loutput -lmy

:: 动态库 + 导入库
gcc -shared -o output\my.dll a.c -Wl,--out-implib,output\libmy.dll.a
objdump -p output\app.exe | findstr "DLL"

:: Make / CMake
make -j8 ; make -n ; make clean
cmake -S . -B build -G "MinGW Makefiles" && cmake --build build
cmake -S . -B build -G Ninja && cmake --build build --verbose

:: GDB
gdb -q output\ex.exe
::   set pagination off / break main / run / bt / print/x / x/16gx / watch var / continue

:: 读产物
size output\ex.exe ; nm output\ex.o ; objdump -d -M intel output\ex.exe
```

---

## 15. 建议顺序

```text
第 2 节 四阶段（一定要亲手跑一遍 .i/.s/.o）
  ↓
第 3、4 节 警告与优化（此后每次编译都带 -Wall -Wextra -std=）
  ↓
第 5、12 节 多文件 + 报错分类（开始写真实规模项目）
  ↓
第 6、7 节 静态库/动态库（理解"符号"这个词才真正理解链接）
  ↓
第 8、9 节 Make → CMake（先手写 Makefile，否则 CMake 只是魔法）
  ↓
第 10、11 节 GDB + binutils（能看内存和汇编， debugging 才不靠猜）
  ↓
第 13 节 编码（写中文必然遇到，越早解决越省事）
```

配套主题目录：`C/15_Build_Library/`（构建与库的深入实践）、`C/16_Debug_Tools/`（GDB/反汇编实战）、`C/03_Scope_Storage_Linkage/`（链接错误的根源）、`C/04_Preprocessor_Macro/`（`-E -dD` 看宏真相）。



