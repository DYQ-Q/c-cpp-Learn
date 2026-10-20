# 1. C语言内存管理简介

C 语言的内存管理是：

```text
程序员手动控制内存分配与释放
```

与 Python/Java 不同：

```text
Python/Java：自动垃圾回收（GC）
C          ：手动 malloc / free，完全由程序员负责
```

C 程序运行时内存布局：

```text
高地址
┌─────────────────┐
│     内核空间     │  （用户程序不可访问）
├─────────────────┤
│     栈 (Stack)   │  局部变量、函数调用帧，向下增长
├─────────────────┤
│                 │
│       ↓         │
│                 │
│       ↑         │
│                 │
├─────────────────┤
│    堆 (Heap)    │  malloc/free 分配，向上增长
├─────────────────┤
│BSS 段 (未初始化)│  全局/静态变量（未初始化）
├─────────────────┤
│Data段 (已初始化)│  全局/静态变量（已初始化）
├─────────────────┤
│ 代码段 (Text)   │  程序指令（只读）
└─────────────────┘
低地址
```

内存区域说明：

```text
• 栈（Stack）   ：自动管理，速度快，容量小（通常 1~8 MB）
• 堆（Heap）    ：手动管理，灵活，容量大（受物理内存限制）
• BSS 段        ：未初始化的全局/静态变量，自动清零
• Data 段       ：已初始化的全局/静态变量
• 代码段        ：程序指令，只读
```

---

# 2. 栈内存

## 2.1 栈的特点

```text
• 由编译器自动分配和释放
• 存放局部变量、函数参数、返回地址
• 后进先出（LIFO）
• 分配速度快（仅移动栈指针）
• 容量有限，溢出会导致栈溢出（Stack Overflow）
```

---

## 2.2 栈上的局部变量

```c
void func() {
    int a = 10;        // 栈上分配
    char buf[100];     // 栈上分配
    // 函数返回时自动释放
}
```

---

## 2.3 栈溢出

```c
void recursive() {
    int arr[10000];    // 每次递归分配大量栈空间
    recursive();       // 无限递归 → 栈溢出
}

// 结果：Segmentation Fault
```

常见原因：

```text
• 无限递归
• 局部数组过大（如 int arr[1000000]）
• 深层函数调用
```

---

## 2.4 栈的优缺点

| 优点 | 缺点 |
| ---- | ---- |
| 自动管理，无需手动释放 | 容量小 |
| 分配速度快 | 生命周期受限于作用域 |
| 不会产生内存碎片 | 不能跨函数返回 |

---

# 3. 堆内存

## 3.1 堆的特点

```text
• 由程序员手动分配和释放（malloc/free）
• 生命周期由程序员控制
• 容量大（受物理内存和虚拟内存限制）
• 分配速度较慢
• 可能产生内存碎片
```

---

## 3.2 malloc / free

```c
#include <stdlib.h>

// 分配内存
int *p = (int*)malloc(5 * sizeof(int));

if (p == NULL) {
    printf("内存分配失败\n");
    return 1;
}

// 使用
for (int i = 0; i < 5; i++) {
    p[i] = i * 10;
}

// 释放
free(p);
p = NULL;   // 防止野指针
```

---

## 3.3 calloc

分配并清零：

```c
int *p = (int*)calloc(5, sizeof(int));
// 等价于 malloc(5 * sizeof(int)) + memset(p, 0, ...)
```

区别：

```text
malloc ：分配内存，内容未初始化（随机值）
calloc ：分配内存，内容全部清零
```

---

## 3.4 realloc

调整已分配内存的大小：

```c
int *p = (int*)malloc(5 * sizeof(int));

// 扩大到 10 个 int
int *new_p = (int*)realloc(p, 10 * sizeof(int));

if (new_p == NULL) {
    printf("realloc 失败\n");
    free(p);   // 原内存仍然有效
    return 1;
}

p = new_p;
```

注意：

```text
• realloc 可能移动内存块（返回新地址）
• 必须用临时变量接收返回值
• 失败时返回 NULL，原内存保持不变
```

---

## 3.5 堆的优缺点

| 优点 | 缺点 |
| ---- | ---- |
| 容量大 | 需手动管理 |
| 生命周期可控 | 容易内存泄漏 |
| 可跨函数使用 | 分配速度较慢 |
| 可动态调整大小 | 可能产生内存碎片 |

---

# 4. 全局变量与静态变量

## 4.1 全局变量

```c
int global_var = 10;       // Data 段（已初始化）
int global_uninit;         // BSS 段（未初始化，自动为 0）

void func() {
    global_var = 20;       // 所有函数可访问
}
```

---

## 4.2 静态变量

```c
void counter() {
    static int count = 0;   // 只初始化一次，生命周期贯穿程序
    count++;
    printf("%d\n", count);
}

// 多次调用输出：1 2 3 4 ...
```

静态变量特性：

```text
• 存储在 Data/BSS 段（不在栈上）
• 只初始化一次
• 作用域限于所在文件或函数
• 生命周期贯穿整个程序
```

---

## 4.3 static 的三种用法

```c
// 1. 修饰局部变量：延长生命周期
void func() {
    static int x = 0;
}

// 2. 修饰全局变量：限制为文件内可见
static int file_var = 10;

// 3. 修饰函数：限制为文件内可见
static void helper() { }
```

---

# 5. 内存对齐

## 5.1 什么是内存对齐

```text
CPU 访问内存时按固定块大小读取
数据按特定边界对齐可提高访问效率
```

结构体对齐规则：

```text
• 每个成员的偏移量必须是该成员大小的整数倍
• 结构体总大小必须是最大成员大小的整数倍
```

---

## 5.2 对齐示例

```c
struct A {
    char c;     // 1 字节，偏移 0
    int i;      // 4 字节，偏移 4（填充 3 字节）
    short s;    // 2 字节，偏移 8
};              // 总大小 12（最大成员 int 的整数倍）

struct B {
    char c;     // 偏移 0
    short s;    // 偏移 2
    int i;      // 偏移 4
};              // 总大小 8（无填充）
```

---

## 5.3 查看与调整对齐

```c
#include <stddef.h>

printf("%zu\n", offsetof(struct A, i));   // 偏移量
printf("%zu\n", sizeof(struct A));        // 总大小
```

调整对齐：

```c
#pragma pack(push, 1)   // 按 1 字节对齐（取消填充）
struct Packed {
    char c;
    int i;
};
#pragma pack(pop)
```

或使用属性：

```c
struct __attribute__((packed)) Packed {
    char c;
    int i;
};
```

---

# 6. 常见内存错误

## 6.1 内存泄漏

```c
void leak() {
    int *p = (int*)malloc(100);
    // 忘记 free(p) → 内存泄漏
}

// 循环中泄漏更严重
while (1) {
    int *p = (int*)malloc(100);
    // 每次循环泄漏 100 字节
}
```

检测工具：

```text
• Valgrind（Linux）
• AddressSanitizer（gcc -fsanitize=address）
• Visual Studio 内存诊断（Windows）
```

---

## 6.2 悬空指针（Use After Free）

```c
int *p = (int*)malloc(sizeof(int));
*p = 10;
free(p);

printf("%d\n", *p);   // 未定义行为：访问已释放内存
```

解决：

```c
free(p);
p = NULL;    // 释放后置空
```

---

## 6.3 重复释放（Double Free）

```c
int *p = (int*)malloc(sizeof(int));
free(p);
free(p);     // 未定义行为
```

解决：

```c
free(p);
p = NULL;
if (p) free(p);   // 安全
```

---

## 6.4 越界访问

```c
int *p = (int*)malloc(5 * sizeof(int));
p[5] = 10;    // 越界，未定义行为
free(p);
```

---

## 6.5 未初始化内存

```c
int *p = (int*)malloc(sizeof(int));
printf("%d\n", *p);   // 随机值
```

解决：

```c
int *p = (int*)calloc(1, sizeof(int));   // 清零
// 或
memset(p, 0, sizeof(int));
```

---

## 6.6 返回栈内存地址

```c
int* bad() {
    int a = 10;
    return &a;      // 危险：a 在函数返回后销毁
}

int* good() {
    int *p = (int*)malloc(sizeof(int));
    *p = 10;
    return p;       // 正确：堆内存，需调用者 free
}
```

---

# 7. 内存管理最佳实践

## 7.1 分配后检查

```c
int *p = (int*)malloc(sizeof(int) * n);
if (p == NULL) {
    // 处理失败
    return -1;
}
```

---

## 7.2 释放后置空

```c
free(p);
p = NULL;
```

---

## 7.3 谁分配谁释放

```text
• 函数内部分配的内存在函数内部释放
• 若需返回给调用者，需明确约定调用者负责 free
```

---

## 7.4 避免在循环中频繁分配

```c
// 不好：每次循环都 malloc/free
for (int i = 0; i < 1000; i++) {
    int *p = (int*)malloc(sizeof(int));
    // ...
    free(p);
}

// 好：一次性分配
int *p = (int*)malloc(1000 * sizeof(int));
for (int i = 0; i < 1000; i++) {
    // 使用 p[i]
}
free(p);
```

---

## 7.5 使用 memset 初始化

```c
#include <string.h>

int *p = (int*)malloc(5 * sizeof(int));
memset(p, 0, 5 * sizeof(int));   // 清零
```

---

## 7.6 结构体初始化

```c
typedef struct {
    int id;
    char name[20];
} Person;

Person *p = (Person*)malloc(sizeof(Person));
memset(p, 0, sizeof(Person));
```

---

# 8. 动态数据结构

## 8.1 动态数组

```c
typedef struct {
    int *data;
    int size;
    int capacity;
} DynamicArray;

void init(DynamicArray *arr, int cap) {
    arr->data = (int*)malloc(cap * sizeof(int));
    arr->size = 0;
    arr->capacity = cap;
}

void push(DynamicArray *arr, int value) {
    if (arr->size >= arr->capacity) {
        arr->capacity *= 2;
        arr->data = (int*)realloc(arr->data, arr->capacity * sizeof(int));
    }
    arr->data[arr->size++] = value;
}

void destroy(DynamicArray *arr) {
    free(arr->data);
    arr->data = NULL;
}
```

---

## 8.2 链表节点

```c
typedef struct Node {
    int data;
    struct Node *next;
} Node;

Node *create_node(int value) {
    Node *node = (Node*)malloc(sizeof(Node));
    if (node == NULL) return NULL;
    node->data = value;
    node->next = NULL;
    return node;
}

void free_list(Node *head) {
    while (head != NULL) {
        Node *temp = head;
        head = head->next;
        free(temp);
    }
}
```

---

## 8.3 动态二维数组

```c
int **create_matrix(int rows, int cols) {
    int **matrix = (int**)malloc(rows * sizeof(int*));
    for (int i = 0; i < rows; i++) {
        matrix[i] = (int*)malloc(cols * sizeof(int));
    }
    return matrix;
}

void free_matrix(int **matrix, int rows) {
    for (int i = 0; i < rows; i++) {
        free(matrix[i]);
    }
    free(matrix);
}
```

---

# 9. 内存调试工具

## 9.1 Valgrind

```bash
valgrind --leak-check=full ./program
```

输出示例：

```text
==12345== HEAP SUMMARY:
==12345==   in use at exit: 100 bytes in 1 blocks
==12345==   total heap usage: 1 allocs, 0 frees, 100 bytes allocated
==12345== LEAK SUMMARY:
==12345==   definitely lost: 100 bytes in 1 blocks
```

---

## 9.2 AddressSanitizer

编译时启用：

```bash
gcc -fsanitize=address -g main.c -o main
./main
```

检测内容：

```text
• 内存泄漏
• 越界访问
• 悬空指针
• 重复释放
```

---

## 9.3 GDB 调试

```bash
gcc -g main.c -o main
gdb ./main
```

GDB 内存相关命令：

```text
x/10x p      ：以十六进制查看内存
p *p         ：打印指针指向的值
info locals  ：查看局部变量
```

---

# 10. 常见问题

## 1. malloc 返回 NULL 未检查

```c
int *p = (int*)malloc(sizeof(int));
*p = 10;    // 若 p 为 NULL，崩溃
```

---

## 2. free 后继续使用

```c
free(p);
p[0] = 10;    // 未定义行为
```

---

## 3. 指针类型不匹配

```c
int *p = (int*)malloc(sizeof(double));   // 分配 double 大小
```

---

## 4. sizeof 指针与数组混淆

```c
int arr[10];
int *p = arr;

printf("%zu\n", sizeof(arr));   // 40
printf("%zu\n", sizeof(p));     // 8（指针大小）
```

---

## 5. 忘记释放导致内存泄漏

```text
• 检查每个 malloc/calloc/realloc 是否有对应 free
• 使用工具检测
```

---

# 11. 推荐学习路线

```text
内存布局（栈、堆、全局区）
↓
栈内存与作用域
↓
malloc / free / calloc / realloc
↓
全局变量与 static 变量
↓
内存对齐
↓
常见内存错误（泄漏、悬空指针、越界）
↓
内存管理最佳实践
↓
动态数据结构（动态数组、链表）
↓
内存调试工具（Valgrind、ASan）
↓
综合项目实践
```

---

# 12. 推荐练习项目

## 初级

* 动态分配数组并释放
* 实现 `memset` 的简单版本

---

## 中级

* 动态数组（支持扩容）
* 单向链表（增删改查 + 释放）
* 动态二维数组

---

## 高级

* 简单内存池实现
* 检测内存泄漏的工具
* 带引用计数的智能指针（C 实现）

---

# 13. 核心速查

```c
#include <stdlib.h>
#include <string.h>

// 分配
void *malloc(size_t size);              // 未初始化
void *calloc(size_t n, size_t size);    // 清零
void *realloc(void *ptr, size_t size);  // 调整大小

// 释放
void free(void *ptr);

// 初始化
memset(ptr, 0, size);

// 常见模式
int *p = (int*)malloc(5 * sizeof(int));
if (p == NULL) { /* 处理失败 */ }
// ... 使用 ...
free(p);
p = NULL;

// 动态二维数组
int **m = (int**)malloc(rows * sizeof(int*));
for (int i = 0; i < rows; i++)
    m[i] = (int*)malloc(cols * sizeof(int));
for (int i = 0; i < rows; i++) free(m[i]);
free(m);
```

## 核心原则

```text
谁分配，谁释放
释放后置 NULL
分配必检查
越界不可取
泄漏用工具查
```