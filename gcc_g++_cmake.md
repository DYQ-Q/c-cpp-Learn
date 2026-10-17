# 1. GCC 与 G++ 简介

GCC 与 G++ 是 GNU 编译器集合中最常用的：

```text
C / C++ 编译器驱动
```

```text
gcc ：C 语言编译器驱动
g++ ：C++ 语言编译器驱动
```

两者核心区别：

```text
• 编译 C 文件：gcc 和 g++ 均可，但 gcc 按 C 语法处理 .c
• 编译 C++ 文件：推荐 g++，它会自动链接 C++ 标准库
• 链接阶段：g++ 自动链接 libstdc++，gcc 不会
```

关键点：如果仅用 gcc 编译 C++ 代码并链接，可能出现：

```text
undefined reference to `std::cout'
undefined reference to `operator new(unsigned long)'
```

因此 **C++ 程序统一使用 g++** 编译和链接是最省心的做法。

---

# 2. 编译流程

C/C++ 从源码到可执行文件经过四个阶段：

```text
源文件 (.c / .cpp)
   ↓ 预处理 (Preprocessing)      gcc -E
预处理文件 (.i)
   ↓ 编译 (Compilation)          gcc -S
汇编文件 (.s)
   ↓ 汇编 (Assembly)             gcc -c
目标文件 (.o)
   ↓ 链接 (Linking)              gcc
可执行文件
```

对应命令：

```bash
gcc -E main.c -o main.i    # 只做预处理
gcc -S main.c -o main.s    # 生成汇编
gcc -c main.c -o main.o    # 生成目标文件
gcc main.o -o main         # 链接
```

---

# 3. 常用编译选项

## 3.1 基础选项

| 选项           | 说明                   |
| -------------- | ---------------------- |
| `-o file`    | 指定输出文件名         |
| `-c`         | 只编译不链接，生成 .o  |
| `-E`         | 只预处理               |
| `-S`         | 只生成汇编代码         |
| `-Wall`      | 开启常见警告           |
| `-Wextra`    | 开启额外警告           |
| `-Werror`    | 把警告当错误           |
| `-g`         | 生成调试信息（GDB 用） |
| `-std=c11`   | 指定 C 标准            |
| `-std=c++17` | 指定 C++ 标准          |
| `-v`         | 显示编译详细过程       |

```bash
gcc -Wall -g -o main main.c
g++ -std=c++17 -Wall -o main main.cpp
```

## 3.2 优化选项

```text
-O0 ：无优化（默认，便于调试）
-O1 ：基本优化
-O2 ：进一步优化（推荐发布）
-O3 ：最高级别优化（体积可能增大）
-Os ：优化代码体积
-Ofast：激进优化（可能违反标准）
```

```bash
gcc -O2 -o main main.c
```

## 3.3 目录与链接选项

```text
-I<dir>  ：指定头文件搜索路径
-L<dir>  ：指定库文件搜索路径
-l<name> ：链接名为 lib<name>.so 或 lib<name>.a 的库
```

```bash
gcc main.c -I/usr/local/include -L/usr/local/lib -lmylib -o main
```

## 3.4 其他常用选项

```text
-D<macro>        ：定义宏（如 -DDEBUG）
-pthread         ：启用 POSIX 线程
-fPIC            ：生成位置无关代码（动态库必须）
-shared          ：生成动态库
-static          ：静态链接
```

---

# 4. 编译 C 工程

## 4.1 单文件

```bash
gcc hello.c -o hello
./hello
```

指定生成的.exe文件的路径,bash需要手动新建目录

```bash
mkdir output
gcc hello.c -o ./output/hello.exe
./output/hello.exe
```

## 4.2 多文件直接编译

```bash
gcc main.c foo.c bar.c -o myprogram
```

## 4.3 分步编译（推荐）

```bash
gcc -c main.c -o main.o
gcc -c foo.c -o foo.o
gcc -c bar.c -o bar.o

gcc main.o foo.o bar.o -o myprogram
```

分步编译的好处：修改某个文件时只需重新编译该文件的目标文件。

## 4.4 指定头文件目录

```bash
gcc -Iinclude src/main.c src/foo.c -o app
```

---

# 5. 编译 C++ 工程

C++ 工程只需把 `gcc` 替换为 `g++`：

```bash
g++ main.cpp foo.cpp -o myprogram
```

分步编译：

```bash
g++ -c main.cpp -o main.o
g++ -c foo.cpp -o foo.o
g++ main.o foo.o -o myprogram
```

指定 C++ 标准：

```bash
g++ -std=c++17 -Wall -o myprogram main.cpp foo.cpp
```

---

# 6. 静态库与动态库

## 6.1 静态库（.a）

编译为目标文件：

```bash
gcc -c file1.c -o file1.o
gcc -c file2.c -o file2.o
```

打包为静态库：

```bash
ar rcs libmylib.a file1.o file2.o
```

使用静态库：

```bash
gcc main.c -L/path/to/library -lmylib -o myprogram
```

## 6.2 动态库（.so）

编译为位置无关代码：

```bash
gcc -fPIC -c file1.c -o file1.o
gcc -fPIC -c file2.c -o file2.o
```

打包为动态库：

```bash
gcc -shared -o libmylib.so file1.o file2.o
```

使用动态库：

```bash
gcc main.c -L/path/to/library -lmylib -o myprogram
```

运行前设置库搜索路径：

```bash
export LD_LIBRARY_PATH=/path/to/library:$LD_LIBRARY_PATH
./myprogram
```

**命名规则**：

```text
静态库：lib<名称>.a
动态库：lib<名称>.so

-L 指定路径，-l 指定名称（去掉 lib 前缀和后缀）
```

---

# 7. Makefile 构建

Makefile 是 C/C++ 工程最常用的构建工具。

## 7.1 C 项目 Makefile

```makefile
CC = gcc
CFLAGS = -Wall -g -Iinclude

OBJS = main.o foo.o bar.o

myprogram: $(OBJS)
	$(CC) $(OBJS) -o myprogram

main.o: main.c foo.h
	$(CC) $(CFLAGS) -c main.c

foo.o: foo.c foo.h
	$(CC) $(CFLAGS) -c foo.c

bar.o: bar.c bar.h
	$(CC) $(CFLAGS) -c bar.c

clean:
	rm -f *.o myprogram
```

## 7.2 C++ 项目 Makefile

```makefile
CXX = g++
CXXFLAGS = -std=c++17 -Wall -g -Iinclude

OBJS = main.o foo.o bar.o

myprogram: $(OBJS)
	$(CXX) $(OBJS) -o myprogram

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o myprogram
```

使用：

```bash
make          # 编译
make clean    # 清理
```

---

# 8. CMake 构建

CMake 是跨平台的现代 C/C++ 构建系统。

## 8.1 项目结构

```text
myproject/
├── CMakeLists.txt
├── main.cpp
└── build/          # 构建目录（Out-of-Source）
```

## 8.2 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.10)
project(MyProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(myprogram
    main.cpp
    foo.cpp
)
```

## 8.3 标准构建流程

```bash
mkdir build
cd build

cmake ..
cmake --build .

./myprogram
```

使用指定编译器：

```bash
cmake -DCMAKE_CXX_COMPILER=g++ ..
```

使用 Ninja 加速构建：

```bash
cmake -G "Ninja" ..
cmake --build .
```

---

# 9. 常见错误与解决

## 9.1 未定义引用（undefined reference）

```text
原因：
• 忘记编译某个源文件
• 链接时缺少目标文件或库
• C++ 代码用 gcc 链接（缺少 libstdc++）
```

解决：

```bash
# 确保所有源文件都参与编译
gcc main.c foo.c bar.c -o app

# C++ 代码用 g++
g++ main.cpp foo.cpp -o app
```

## 9.2 找不到头文件

```text
fatal error: myheader.h: No such file or directory
```

解决：

```bash
gcc -I/path/to/headers main.c -o app
```

## 9.3 找不到库

```text
cannot find -lmylib
```

解决：

```bash
gcc main.c -L/path/to/library -lmylib -o app
```

## 9.4 运行时找不到动态库

```text
error while loading shared libraries: libmylib.so: cannot open shared object file
```

解决：

```bash
export LD_LIBRARY_PATH=/path/to/library:$LD_LIBRARY_PATH
```

或将库路径写入 `/etc/ld.so.conf.d/` 并执行 `ldconfig`。

---

# 10. 推荐学习路线

```text
gcc / g++ 基本编译命令
↓
编译选项（-c -o -Wall -g -O2 -std=）
↓
单文件编译
↓
多文件分步编译
↓
静态库与动态库
↓
Makefile 编写
↓
CMake 构建系统
↓
调试（GDB）与优化
```

---

# 11. 核心命令速查

```bash
# 编译 C
gcc main.c -o main
gcc -Wall -g -o main main.c

# 编译 C++
g++ main.cpp -o main
g++ -std=c++17 -Wall -o main main.cpp

# 分步编译
gcc -c main.c -o main.o
gcc main.o foo.o -o app

# 头文件与库路径
gcc main.c -Iinclude -Llib -lmylib -o app

# 静态库
ar rcs libmylib.a *.o

# 动态库
gcc -fPIC -c *.c
gcc -shared -o libmylib.so *.o
export LD_LIBRARY_PATH=$PWD:$LD_LIBRARY_PATH

# Makefile
make
make clean

# CMake
mkdir build && cd build
cmake ..
cmake --build .
```
