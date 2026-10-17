# 1. C语言简介

C 语言是当前最常用的：

```text
通用、过程式编程语言
```

由 Dennis Ritchie 于 1972 年在贝尔实验室开发，主要用于：

* 操作系统开发（Linux、Windows 内核）
* 嵌入式系统
* 编译器与解释器
* 数据库系统
* 高性能计算
* 底层驱动

与 Python 不同：

```text
Python：解释型，动态类型，语法简洁
C     ：编译型，静态类型，贴近硬件
```

例如：

Python：

```python
print("Hello")
```

C：

```c
#include <stdio.h>

int main() {
    printf("Hello\n");
    return 0;
}
```

---

# 2. 编译与运行

## 2.1 安装编译器

### Linux

```bash
sudo apt install gcc   # Ubuntu/Debian
sudo yum install gcc   # CentOS/RHEL
```

### macOS

```bash
xcode-select --install
```

### Windows

```text
安装 MinGW 或使用 Visual Studio
或使用 WSL
```

---

## 2.2 验证安装

```bash
gcc --version
```

输出：

```text
gcc (Ubuntu 11.4.0) 11.4.0
```

---

## 2.3 编写第一个程序

创建 `hello.c`：

```c
#include <stdio.h>

int main() {
    printf("Hello, World!\n");
    return 0;
}
```

编译：

```bash
gcc hello.c -o hello
```

运行：

```bash
./hello
```

输出：

```text
Hello, World!
```

---

## 2.4 编译流程

```text
源文件 (.c)
   ↓ 预处理 (gcc -E)
预处理文件 (.i)
   ↓ 编译 (gcc -S)
汇编文件 (.s)
   ↓ 汇编 (gcc -c)
目标文件 (.o)
   ↓ 链接 (gcc)
可执行文件
```

---

# 3. 程序结构

```c
#include <stdio.h>      // 头文件

#define PI 3.14159      // 宏定义

int global_var = 10;    // 全局变量

// 函数声明
int add(int a, int b);

int main() {
    int result = add(3, 5);
    printf("%d\n", result);
    return 0;
}

// 函数定义
int add(int a, int b) {
    return a + b;
}
```

结构说明：

```text
• #include：引入头文件
• #define：宏定义
• 全局变量：所有函数可访问
• 函数声明：告诉编译器函数存在
• main 函数：程序入口
```

---

# 4. 变量与数据类型

## 4.1 基本数据类型

| 类型 | 说明 | 大小（典型） | 范围 |
| ---- | ---- | ------------ | ---- |
| char | 字符 | 1 字节 | -128 ~ 127 |
| int | 整数 | 4 字节 | -2^31 ~ 2^31-1 |
| float | 单精度浮点 | 4 字节 | ±3.4e38 |
| double | 双精度浮点 | 8 字节 | ±1.7e308 |
| void | 无类型 | - | - |

修饰符：

```text
short       ：短整型（2 字节）
long        ：长整型（4 或 8 字节）
long long   ：超长整型（8 字节）
unsigned    ：无符号（只能表示非负数）
signed      ：有符号（默认）
```

示例：

```c
unsigned int a = 100;
long long b = 1234567890123LL;
```

---

## 4.2 变量声明与初始化

```c
int a;              // 声明
a = 10;             // 赋值

int b = 20;         // 声明并初始化
float pi = 3.14f;   // f 表示 float
double d = 3.14159;
char c = 'A';
```

---

## 4.3 常量

```c
const int MAX = 100;      // const 常量
#define PI 3.14159        // 宏常量
```

区别：

```text
const：有类型，编译时检查
#define：预处理替换，无类型
```

---

## 4.4 sizeof 运算符

```c
printf("%zu\n", sizeof(int));      // 4
printf("%zu\n", sizeof(double));   // 8
printf("%zu\n", sizeof(char));     // 1
```

---

# 5. 运算符

## 5.1 算术运算符

```c
int a = 10, b = 3;

printf("%d\n", a + b);   // 13
printf("%d\n", a - b);   // 7
printf("%d\n", a * b);   // 30
printf("%d\n", a / b);   // 3（整数除法）
printf("%d\n", a % b);   // 1（取余）
```

浮点除法：

```c
float x = 10.0 / 3;   // 3.333333
```

---

## 5.2 关系运算符

```c
printf("%d\n", 3 > 2);    // 1（真）
printf("%d\n", 3 < 2);    // 0（假）
printf("%d\n", 3 == 3);   // 1
printf("%d\n", 3 != 2);   // 1
printf("%d\n", 3 >= 3);   // 1
printf("%d\n", 3 <= 2);   // 0
```

---

## 5.3 逻辑运算符

```c
printf("%d\n", 1 && 0);   // 0（与）
printf("%d\n", 1 || 0);   // 1（或）
printf("%d\n", !1);       // 0（非）
```

短路特性：

```text
&& ：左边为假，右边不计算
|| ：左边为真，右边不计算
```

---

## 5.4 位运算符

```c
int a = 5;   // 0101
int b = 3;   // 0011

printf("%d\n", a & b);    // 1  （按位与,两位都为1才为1）
printf("%d\n", a | b);    // 7  （按位或,两位有一个为1就为1）
printf("%d\n", a ^ b);    // 6  （按位异或,两位不同为1）
printf("%d\n", ~a);       // -6 （按位取反）
printf("%d\n", a << 1);   // 10 （左移）
printf("%d\n", a >> 1);   // 2  （右移）
```

---

## 5.5 赋值运算符

```c
int x = 10;
x += 5;    // x = x + 5  → 15
x -= 3;    // x = x - 3  → 12
x *= 2;    // x = x * 2  → 24
x /= 4;    // x = x / 4  → 6
x %= 4;    // x = x % 4  → 2
```

---

## 5.6 自增自减

```c
int a = 5;
printf("%d\n", a++);   // 5（先输出再加）
printf("%d\n", a);     // 6

int b = 5;
printf("%d\n", ++b);   // 6（先加再输出）
```

---

## 5.7 三目运算符

```c
int a = 10, b = 20;
int max = (a > b) ? a : b;
printf("%d\n", max);   // 20
```

---

## 5.8 运算符优先级

```text
1. () [] -> .
2. ! ~ ++ -- + - * & (类型) sizeof
3. * / %
4. + -
5. << >>
6. < <= > >=
7. == !=
8. & ^ |
9. && ||
10. ?:
11. = += -= 等
12. ,
```

记忆建议：

```text
不确定时加括号
```

---

# 6. 输入输出

## 6.1 printf 格式化输出

```c
int a = 10;
float f = 3.14;
char c = 'A';
char s[] = "hello";

printf("%d\n", a);       // 整数
printf("%f\n", f);       // 浮点数
printf("%.2f\n", f);     // 保留 2 位小数
printf("%c\n", c);       // 字符
printf("%s\n", s);       // 字符串
printf("%p\n", &a);      // 指针
```

常用格式符：

| 格式符 | 类型 |
| ------ | ---- |
| %d | int |
| %ld | long |
| %lld | long long |
| %u | unsigned int |
| %f | float/double |
| %lf | double（scanf） |
| %c | char |
| %s | 字符串 |
| %p | 指针 |
| %x | 十六进制 |
| %% | 百分号 |

---

## 6.2 scanf 输入

```c
int age;
float height;
char name[20];

printf("请输入年龄：");
scanf("%d", &age);

printf("请输入身高：");
scanf("%f", &height);

printf("请输入姓名：");
scanf("%s", name);   // 数组名本身就是地址，无需 &
```

**注意**：`scanf` 需要传变量的地址（`&`），字符串数组例外。

---

## 6.3 字符输入输出

```c
char c;

c = getchar();     // 读一个字符
putchar(c);        // 输出一个字符

putchar('\n');
```

---

# 7. 条件语句

## 7.1 if / else if / else

```c
int score = 85;

if (score >= 90) {
    printf("优秀\n");
} else if (score >= 60) {
    printf("及格\n");
} else {
    printf("不及格\n");
}
```

---

## 7.2 单行 if

```c
if (x > 0) printf("正数\n");
```

---

## 7.3 switch

```c
int day = 3;

switch (day) {
    case 1:
        printf("周一\n");
        break;
    case 2:
        printf("周二\n");
        break;
    case 3:
        printf("周三\n");
        break;
    default:
        printf("其他\n");
}
```

**注意**：每个 case 后要加 `break`，否则会“穿透”。

---

# 8. 循环

## 8.1 for 循环

```c
for (int i = 0; i < 5; i++) {
    printf("%d\n", i);
}
```

结构：

```text
for (初始化; 条件; 更新) {
    循环体
}
```

---

## 8.2 while 循环

```c
int i = 0;
while (i < 5) {
    printf("%d\n", i);
    i++;
}
```

---

## 8.3 do-while 循环

```c
int i = 0;
do {
    printf("%d\n", i);
    i++;
} while (i < 5);
```

区别：

```text
while    ：先判断条件，可能一次都不执行
do-while ：先执行一次，再判断条件
```

---

## 8.4 break 与 continue

```c
for (int i = 0; i < 10; i++) {
    if (i == 5) break;      // 跳出循环
    if (i % 2 == 0) continue; // 跳过本次
    printf("%d\n", i);
}
```

---

## 8.5 嵌套循环

```c
for (int i = 1; i <= 3; i++) {
    for (int j = 1; j <= 3; j++) {
        printf("%d ", i * j);
    }
    printf("\n");
}
```

---

## 8.6 goto（慎用）

```c
int i = 0;
loop:
    if (i < 5) {
        printf("%d\n", i);
        i++;
        goto loop;
    }
```

---

# 9. 数组

## 9.1 一维数组

```c
int arr[5];                     // 声明
int arr2[5] = {1, 2, 3, 4, 5};  // 初始化
int arr3[] = {1, 2, 3};         // 自动推断长度

// 访问
arr[0] = 10;
printf("%d\n", arr2[2]);        // 3
```

---

## 9.2 遍历数组

```c
int arr[5] = {1, 2, 3, 4, 5};

for (int i = 0; i < 5; i++) {
    printf("%d ", arr[i]);
}
```

---

## 9.3 二维数组

```c
int matrix[3][4] = {
    {1, 2, 3, 4},
    {5, 6, 7, 8},
    {9, 10, 11, 12}
};

// 访问
printf("%d\n", matrix[1][2]);   // 7

// 遍历
for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 4; j++) {
        printf("%d ", matrix[i][j]);
    }
    printf("\n");
}
```

---

## 9.4 字符数组与字符串

```c
char str1[] = "hello";        // 数组，可修改
char str2[10] = "world";      // 指定长度
char str3[] = {'h','i','\0'}; // 手动添加结束符

printf("%s\n", str1);
```

**注意**：C 字符串以 `'\0'` 结尾。

---

# 10. 函数

## 10.1 定义与调用

```c
// 声明
int add(int a, int b);

int main() {
    int result = add(3, 5);
    printf("%d\n", result);
    return 0;
}

// 定义
int add(int a, int b) {
    return a + b;
}
```

---

## 10.2 返回类型

```c
void print_hello() {       // 无返回值
    printf("Hello\n");
}

int square(int x) {        // 有返回值
    return x * x;
}
```

---

## 10.3 参数传递

```text
C 语言只有值传递
传入的是变量的副本
```

示例：

```c
void change(int x) {
    x = 100;    // 只修改副本，不影响外部
}

int main() {
    int a = 10;
    change(a);
    printf("%d\n", a);   // 10
}
```

通过指针修改外部变量：

```c
void change(int *x) {
    *x = 100;
}

int main() {
    int a = 10;
    change(&a);
    printf("%d\n", a);   // 100
}
```

---

## 10.4 数组作为参数

```c
void print_array(int arr[], int size) {
// 等价于 void print_array(int *arr, int size)
    for (int i = 0; i < size; i++) {
        printf("%d ", arr[i]);
    }
}
```

**注意**：数组传参退化为指针，无法通过 sizeof 获取长度。

---

## 10.5 递归

```c
int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

int main() {
    printf("%d\n", factorial(5));   // 120
}
```

---

## 10.6 局部与全局变量

```c
int global = 10;    // 全局变量

void func() {
    int local = 20; // 局部变量
    printf("%d %d\n", global, local);
}
```

作用域：

```text
全局：整个程序可见
局部：仅所在函数或代码块可见
```

---

## 10.7 static 关键字

```c
void counter() {
    static int count = 0;   // 只初始化一次
    count++;
    printf("%d\n", count);
}

// 多次调用输出：1 2 3 4 ...
```

---

# 11. 指针

## 11.1 基本概念

```c
int a = 10;
int *p = &a;    // p 存储 a 的地址

printf("%p\n", &a);   // a 的地址
printf("%p\n", p);    // p 的值（即 a 的地址）
printf("%d\n", *p);   // 解引用，输出 10

*p = 20;              // 通过指针修改 a
printf("%d\n", a);    // 20
```

---

## 11.2 指针与数组

```c
int arr[5] = {1, 2, 3, 4, 5};
int *p = arr;    // 等价于 &arr[0]

printf("%d\n", *p);       // 1
printf("%d\n", *(p+2));   // 3
printf("%d\n", p[3]);     // 4
```

遍历：

```c
for (int i = 0; i < 5; i++) {
    printf("%d ", *(p + i));
}
```

---

## 11.3 指针运算

```c
int arr[5] = {1, 2, 3, 4, 5};
int *p = arr;

p++;         // 指向下一个元素（地址增加 sizeof(int)）
p += 2;      // 向后移动 2 个元素
```

---

## 11.4 指针与函数

```c
void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

int main() {
    int x = 5, y = 10;
    swap(&x, &y);
    printf("%d %d\n", x, y);   // 10 5
}
```

---

## 11.5 空指针与野指针

```c
int *p = NULL;     // 空指针，安全

if (p != NULL) {
    *p = 10;
}

int *q;            // 野指针，危险
// *q = 10;        // 未定义行为
```

---

## 11.6 指针与 const

```c
const int *p1;       // 指向常量的指针（不能修改内容）
int * const p2;      // 常量指针（不能指向别处）
const int * const p3; // 两者都不可变
```

---

## 11.7 多级指针

```c
int a = 10;
int *p = &a;
int **pp = &p;

printf("%d\n", **pp);   // 10
```

---

## 11.8 函数指针

```c
int add(int a, int b) {
    return a + b;
}

int main() {
    int (*fp)(int, int) = add;
    printf("%d\n", fp(3, 5));   // 8
}
```

---

# 12. 结构体

## 12.1 定义与使用

```c
struct Point {
    int x;
    int y;
};

int main() {
    struct Point p1;
    p1.x = 3;
    p1.y = 4;

    struct Point p2 = {5, 6};

    printf("%d %d\n", p1.x, p1.y);
    printf("%d %d\n", p2.x, p2.y);
}
```

---

## 12.2 typedef 简化

```c
typedef struct {
    int x;
    int y;
} Point;

Point p = {3, 4};
```

---

## 12.3 结构体指针

```c
Point p = {3, 4};
Point *ptr = &p;

printf("%d\n", ptr->x);   // 3，等价于 (*ptr).x
ptr->y = 10;
```

---

## 12.4 结构体数组

```c
Point points[3] = {
    {1, 2},
    {3, 4},
    {5, 6}
};

for (int i = 0; i < 3; i++) {
    printf("%d %d\n", points[i].x, points[i].y);
}
```

---

## 12.5 嵌套结构体

```c
struct Date {
    int year;
    int month;
    int day;
};

struct Person {
    char name[20];
    struct Date birthday;
};

struct Person p = {"Alice", {2000, 1, 1}};
printf("%d\n", p.birthday.year);   // 2000
```

---

## 12.6 结构体作为函数参数

```c
void print_point(Point p) {       // 值传递（复制）
    printf("%d %d\n", p.x, p.y);
}

void move(Point *p, int dx) {     // 指针传递（修改原值）
    p->x += dx;
}
```

---

# 13. 动态内存分配

## 13.1 malloc / free

```c
#include <stdlib.h>

int *p = (int *)malloc(sizeof(int) * 5);   // 分配 5 个 int

if (p == NULL) {
    printf("分配失败\n");
    return 1;
}

for (int i = 0; i < 5; i++) {
    p[i] = i * 10;
}

free(p);    // 释放
p = NULL;   // 防止野指针
```

---

## 13.2 calloc

```c
int *p = (int *)calloc(5, sizeof(int));   // 分配并清零
```

---

## 13.3 realloc

```c
int *p = (int *)malloc(sizeof(int) * 5);
p = (int *)realloc(p, sizeof(int) * 10);  // 扩大到 10
```

---

## 13.4 常见错误

```text
• 忘记 free → 内存泄漏
• free 后继续使用 → 悬空指针
• 重复 free → 未定义行为
• 越界访问 → 未定义行为
```

---

# 14. 字符串函数

需要包含：

```c
#include <string.h>
```

## 14.1 常用函数

```c
char s1[20] = "hello";
char s2[] = "world";

strlen(s1);              // 5（长度，不含 '\0'）
strcpy(s1, s2);          // 复制 s2 到 s1
strcat(s1, s2);          // 拼接
strcmp(s1, s2);          // 比较（0 表示相等）
strchr(s1, 'l');         // 查找字符
strstr(s1, "ll");        // 查找子串
```

---

## 14.2 安全版本（推荐）

```c
strncpy(s1, s2, sizeof(s1) - 1);
strncat(s1, s2, sizeof(s1) - strlen(s1) - 1);
```

---

## 14.3 字符串与数字转换

```c
#include <stdlib.h>

int num = atoi("123");           // 字符串转 int
double d = atof("3.14");         // 字符串转 double
long l = atol("123456");         // 字符串转 long

char buf[20];
sprintf(buf, "%d", 123);         // 数字转字符串
```

---

# 15. 预处理指令

## 15.1 #include

```c
#include <stdio.h>    // 系统头文件
#include "myheader.h" // 自定义头文件
```

---

## 15.2 #define

```c
#define PI 3.14159
#define MAX(a, b) ((a) > (b) ? (a) : (b))

printf("%f\n", PI);
printf("%d\n", MAX(3, 5));
```

**注意**：宏参数要加括号。

---

## 15.3 条件编译

```c
#ifdef DEBUG
    printf("调试信息\n");
#endif

#ifndef HEADER_H
#define HEADER_H
// 头文件内容
#endif
```

---

## 15.4 预定义宏

```c
printf("%s\n", __FILE__);    // 当前文件名
printf("%d\n", __LINE__);    // 当前行号
printf("%s\n", __DATE__);    // 编译日期
printf("%s\n", __TIME__);    // 编译时间
```

---

# 16. 文件操作

## 16.1 打开与关闭

```c
#include <stdio.h>

FILE *fp = fopen("data.txt", "r");

if (fp == NULL) {
    printf("打开失败\n");
    return 1;
}

// 操作文件...

fclose(fp);
```

---

## 16.2 文件模式

| 模式 | 说明 |
| ---- | ---- |
| "r" | 只读 |
| "w" | 写入（覆盖） |
| "a" | 追加 |
| "r+" | 读写 |
| "w+" | 读写（覆盖） |
| "rb" | 二进制只读 |
| "wb" | 二进制写入 |

---

## 16.3 读写文件

```c
// 写入
FILE *fp = fopen("out.txt", "w");
fprintf(fp, "Hello %d\n", 123);
fputs("World\n", fp);
fputc('A', fp);
fclose(fp);

// 读取
fp = fopen("out.txt", "r");
char buf[100];
while (fgets(buf, sizeof(buf), fp) != NULL) {
    printf("%s", buf);
}
fclose(fp);
```

---

## 16.4 字符级读写

```c
int c;
while ((c = fgetc(fp)) != EOF) {
    putchar(c);
}
```

---

## 16.5 二进制读写

```c
// 写入
int data[3] = {1, 2, 3};
fwrite(data, sizeof(int), 3, fp);

// 读取
int buf[3];
fread(buf, sizeof(int), 3, fp);
```

---

## 16.6 文件定位

```c
fseek(fp, 0, SEEK_SET);   // 移到开头
fseek(fp, 0, SEEK_END);   // 移到末尾
long pos = ftell(fp);     // 当前位置
rewind(fp);               // 回到开头
```

---

# 17. 常见问题

## 1. 段错误（Segmentation Fault）

```text
原因：
• 访问空指针
• 数组越界
• 使用未初始化指针
• 释放已释放的内存
```

---

## 2. 未定义行为

```text
• 有符号整数溢出
• 除以零
• 访问已释放内存
• 修改字符串字面量
```

---

## 3. 忘记初始化

```c
int a;              // 未初始化，值是随机的
printf("%d\n", a);  // 危险

int a = 0;          // 应显式初始化
```

---

## 4. 数组越界

```c
int arr[5];
arr[10] = 1;        // 越界，未定义行为
```

---

## 5. 忘记加 &（scanf）

```c
int a;
scanf("%d", a);     // 错误
scanf("%d", &a);    // 正确
```

---

## 6. 字符串未以 '\0' 结尾

```c
char s[3] = {'a', 'b', 'c'};   // 不是合法字符串
printf("%s\n", s);             // 可能乱码
```

---

# 18. 编译与调试

## 18.1 常用编译选项

```bash
gcc main.c -o main              # 基本编译
gcc -Wall -Wextra main.c -o main # 开启警告
gcc -g main.c -o main           # 带调试信息
gcc -O2 main.c -o main          # 优化
gcc -std=c11 main.c -o main     # 指定标准
```

---

## 18.2 多文件编译

```bash
gcc main.c utils.c -o main
```

或分步：

```bash
gcc -c main.c -o main.o
gcc -c utils.c -o utils.o
gcc main.o utils.o -o main
```

---

## 18.3 使用 GDB 调试

```bash
gcc -g main.c -o main
gdb ./main
```

GDB 常用命令：

```text
run / r            ：运行
break / b 行号     ：设置断点
next / n           ：单步跳过
step / s           ：单步进入
print / p 变量     ：打印变量
continue / c       ：继续
quit / q           ：退出
```

---

# 19. 推荐学习路线

```text
安装编译器与第一个程序
↓
变量与数据类型
↓
运算符
↓
输入输出（printf / scanf）
↓
条件语句
↓
循环
↓
数组
↓
函数
↓
指针
↓
字符串
↓
结构体
↓
动态内存分配
↓
文件操作
↓
预处理与多文件编程
↓
综合项目练习
```

---

# 20. 推荐练习项目

## 初级

* 计算器（四则运算）
* 猜数字游戏
* 九九乘法表

---

## 中级

* 学生成绩管理系统（结构体 + 数组）
* 字符串处理工具（strlen / strcpy 实现）
* 简易通讯录（文件持久化）

---

## 高级

* 链表实现（单链表、双向链表）
* 简易编译器（词法分析器）
* 内存池管理
* 简单的 Shell

---

# 21. 核心语法速查

```c
// 头文件
#include <stdio.h>

// 变量
int a = 10;
float f = 3.14f;
char c = 'A';

// 输入输出
printf("%d\n", a);
scanf("%d", &a);

// 条件
if (a > 5) {
    // ...
} else {
    // ...
}

switch (a) {
    case 1: break;
    default: break;
}

// 循环
for (int i = 0; i < 10; i++) { }
while (a > 0) { a--; }
do { } while (a > 0);

// 数组
int arr[5] = {1, 2, 3, 4, 5};
int matrix[3][3];

// 函数
int add(int a, int b) {
    return a + b;
}

// 指针
int *p = &a;
printf("%d\n", *p);

// 结构体
struct Point {
    int x, y;
};

typedef struct {
    int x, y;
} Point;

// 动态内存
int *p = (int *)malloc(sizeof(int) * 10);
free(p);

// 字符串
#include <string.h>
strlen(s);
strcpy(dest, src);

// 文件
FILE *fp = fopen("file.txt", "r");
fclose(fp);
```