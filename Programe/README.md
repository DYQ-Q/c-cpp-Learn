# Programe —— C / C++ 能干什么（15 个可运行演示）

你之前的练习基本都是「终端输入 → 处理 → 终端输出」。这一套程序的目标是让你看到
C/C++ 真正被使用的场景：**自己管内存、直接写二进制文件、开窗口动画、起线程、
收发网络包、写上层的库和工具**。

环境：MSYS2 MinGW gcc / g++ 15.2（支持 `-std=c17` / `-std=c++23`）。

## 准备

```powershell
cd D:\st_documents\VS_Doc\C_C++\C_C++_Learn\Programe
mkdir out            # 放生成的 exe
mkdir img            # 03 号程序会把图片输出到这里
chcp 65001           # 让控制台用 UTF-8，中文不乱码
```

## c/ —— C 语言（贴近硬件与操作系统）

| 文件 | 一句话 | 编译 |
|---|---|---|
| `01_memory_and_pointers.c` | 栈/堆、指针算术、malloc-realloc、结构体内存布局、函数指针回调 | `gcc -std=c17 -Wall -Wextra -g c\01_memory_and_pointers.c -o out\01.exe` |
| `02_cli_tools.c` | 自己写一套 Unix 小工具：hexdump / wc / grep / crc32 / base64 | `gcc -std=c17 -Wall -Wextra c\02_cli_tools.c -o out\02.exe` |
| `03_ppm_image.c` | **不装任何库**，纯算术生成图片文件：渐变、Mandelbrot 分形、光线追踪球 | `gcc -std=c17 -Wall -Wextra -O2 c\03_ppm_image.c -o out\03.exe` |
| `04_console_snake.c` | 完整贪吃蛇游戏：光标定位、非阻塞按键、游戏循环、增速 | `gcc -std=c17 -O2 c\04_console_snake.c -o out\04.exe` |
| `05_win32_threads.c` | 直接调 Windows API 开线程：CreateThread、临界区、原子加法、高精度计时 | `gcc -std=c17 -O2 c\05_win32_threads.c -o out\05.exe` |
| `06_tcp_chat.c` | TCP 网络编程：多人聊天室服务器 + 客户端（select 多路复用） | `gcc -O2 c\06_tcp_chat.c -o out\06.exe -lws2_32` |
| `07_data_structures.c` | 用 `void*` 手写泛型动态数组 + 哈希表 + LRU，并做性能对比 | `gcc -std=c17 -O2 c\07_data_structures.c -o out\07.exe` |

## cpp/ —— C++（抽象能力 + 仍然零开销）

| 文件 | 一句话 | 编译 |
|---|---|---|
| `01_modern_stl.cpp` | 容器/算法/lambda/string_view/optional/variant/结构化绑定/ranges/std::format | `g++ -std=c++20 -Wall -Wextra cpp\01_modern_stl.cpp -o out\cpp01.exe` |
| `02_raii_smartptr.cpp` | RAII、unique_ptr 自定义删除器、shared/weak_ptr 循环引用、内存池 | `g++ -std=c++20 -Wall -Wextra cpp\02_raii_smartptr.cpp -o out\cpp02.exe` |
| `03_template_vector.cpp` | **自己实现一个 std::vector**：模板、移动语义、迭代器、完美转发、异常安全 | `g++ -std=c++20 -Wall -Wextra cpp\03_template_vector.cpp -o out\cpp03.exe` |
| `04_thread_pool.cpp` | 线程池 + future/packaged_task + 条件变量生产者消费者 | `g++ -std=c++20 -O2 cpp\04_thread_pool.cpp -o out\cpp04.exe` |
| `05_game_of_life.cpp` | ANSI 转义码做全屏彩色动画：双缓冲、差分刷新、稀疏优化 | `g++ -std=c++20 -O2 cpp\05_game_of_life.cpp -o out\cpp05.exe` |
| `06_http_server.cpp` | **手写 HTTP 服务器**，浏览器打开 http://localhost:8080 就能看到页面 | `g++ -std=c++20 -O2 cpp\06_http_server.cpp -o out\cpp06.exe -lws2_32` |
| `07_json_parser.cpp` | 递归下降 JSON 解析器 + 异常处理 + std::filesystem 读配置 | `g++ -std=c++20 -Wall -Wextra cpp\07_json_parser.cpp -o out\cpp07.exe` |
| `08_inventory_game.cpp` | 小型 RPG：虚函数多态、unique_ptr 容器、存档读写、随机战斗模拟 | `g++ -std=c++20 -Wall -Wextra cpp\08_inventory_game.cpp -o out\cpp08.exe` |

网络类的（`06_tcp_chat.c`、`cpp06_http_server`）第一次运行时 Windows 可能弹防火墙提示，
点「允许」或直接关掉防火墙弹窗即可，只是本地回环测试。

几个小项目的玩法提示：

- `cpp\06_http_server.cpp`：先 `out\cpp06.exe 8080` 起服务，浏览器打开 <http://127.0.0.1:8080/>；
  再开一个窗口 `out\cpp06.exe 8080 bench 20000`，能测出平均 / P50 / P90 / P99 延迟和 req/s。
- `cpp\07_json_parser.cpp`、`cpp\08_inventory_game.cpp` 会在当前目录自动创建 `data\`，
  分别写 `config.json` 和 `savegame.txt`，可以直接打开看内容。
- `cpp\08_inventory_game.cpp` 加参数 `interactive` 进入控制台：`inv / sort / equip <名字> / drink / fight / shop / save / load / stats / quit`。

## 建议顺序

1. `c/01` → `c/02` → `c/03`：先把「内存 + 二进制文件」这条线打通，这是 C 的根。
2. `c/04`、`c/05`：程序不再是一行行往下跑，而是**有循环、有并发**。
3. `c/06`：程序开始和**别的程序对话**。
4. `cpp/01` → `cpp/02` → `cpp/03`：从手动 malloc 升级到「用对象生命周期管资源」。
5. `cpp/04` → `cpp/08`：把前面的东西组合成真正的小项目。

每个文件头部都写清楚了这个程序「展示了什么能力」，直接看注释就能当讲义用。
