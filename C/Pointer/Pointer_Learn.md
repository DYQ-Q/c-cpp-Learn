# 1. C语言指针简介

指针是 C 语言的核心特性：

```text
存储变量地址的变量
通过地址间接访问数据
```

指针的作用：

```text
• 动态内存管理
• 数组与字符串操作
• 函数间传递大量数据（避免复制）
• 实现数据结构（链表、树等）
• 函数指针实现回调与多态
• 硬件寄存器访问
```

与普通变量对比：

```c
int a = 10;    // 普通变量，存储值
int *p = &a;   // 指针变量，存储 a 的地址
```



---

# 2. 指针基础语法

## 2.1 声明与初始化

```c
类型 *指针变量名;

int *p;      // p 是指向 int 的指针
double *q;   // 指向 double
char *r;     // 指向 char
```

初始化：

```c
int a = 5;
int *p = &a;   // p 指向 a

int *q = NULL; // 空指针，不指向任何有效地址
```

---

## 2.2 取地址 & 与解引用 *

```c
int a = 10;
int *p = &a;   // &a 取 a 的地址

printf("%d\n", *p); // *p 解引用，输出 10
*p = 20;            // 通过指针修改 a
printf("%d\n", a);  // 输出 20
```

---

## 2.3 指针的大小与类型

```c
printf("%zu\n", sizeof(int *));    // 通常 4（32位）或 8（64位）
printf("%zu\n", sizeof(double *)); // 同指针大小，与指向类型无关
```

指针类型决定：

```text
• 解引用时读取多少字节
• 指针加减时的步长
```

---

## 2.4 指针运算

```c
int arr[5] = {10, 20, 30, 40, 50};
int *p = arr;

p++;        // 指向下一个 int（地址增加 sizeof(int)）
p += 2;     // 向后移动 2 个元素
p--;        // 向前移动 1 个元素

int *q = p + 3;   // 偏移 3 个元素
```

两个指针相减：

```c
int *start = arr;
int *end = arr + 5;
printf("%td\n", end - start);   // 5（元素个数）
```

---

## 2.5 空指针与野指针

```c
int *p = NULL;     // 空指针，安全

if (p != NULL) {
    *p = 10;
}

int *q;            // 野指针，危险
// *q = 10;        // 未定义行为
```

---

## 2.6 指针与 const

```c
const int *p1;          // 指向常量的指针（不能通过 p1 修改内容）
int * const p2 = &a;    // 常量指针（p2 不能指向别处）
const int * const p3 = &a;  // 两者都不可变
```

示例：

```c
int a = 5, b = 8;
const int *p = &a;
// *p = 10;  // 错误
p = &b;       // 可以

int *const q = &a;
*q = 10;      // 可以
// q = &b;    // 错误
```

---

# 3. 指针与数组

## 3.1 数组名是指针常量

```c
int arr[5] = {1, 2, 3, 4, 5};
int *p = arr;   // arr 等价于 &arr[0]

printf("%d\n", arr[0]);   // 1
printf("%d\n", *p);       // 1
printf("%d\n", *(arr+2)); // 3
```

**重要**：`arr` 是常量指针，不能赋值：`arr = p;` 非法。

例外情况：数组名在 `sizeof` 和 `&` 运算中不退化为指针：

```c
printf("%zu\n", sizeof(arr));   // 20（5 * sizeof(int)）
printf("%p\n", (void*)&arr);    // 整个数组的地址
```

---

## 3.2 指针运算遍历数组

```c
int arr[5] = {10, 20, 30, 40, 50};
int *p = arr;

for (int i = 0; i < 5; i++) {
    printf("%d ", *(p + i));   // 等价于 p[i] 或 arr[i]
}
```

---

## 3.3 指针与数组下标等价

```c
arr[i] == *(arr + i)
p[i]   == *(p + i)
```

---

## 3.4 指向数组的指针（数组指针）

指向整个数组的指针，而不是指向第一个元素。

```c
int arr[5] = {1, 2, 3, 4, 5};
int (*p)[5] = &arr;   // p 指向包含 5 个 int 的数组

printf("%d\n", (*p)[0]);   // 1
printf("%d\n", (*p)[2]);   // 3
printf("%zu\n", sizeof(*p)); // 20
```

步长区别：

```text
int *p1 = arr;       // 指向 int，步长 sizeof(int)
int (*p2)[5] = &arr; // 指向 int[5]，步长 5*sizeof(int)
```

二维数组中的应用：

```c
int matrix[3][4];
int (*row)[4] = matrix;   // row 指向第 0 行（int[4]）
row++;                    // 跳转到下一行
printf("%d\n", row[1][2]); // 访问第 1 行第 2 列
```

---

## 3.5 指针数组

数组元素是指针。

```c
int a = 1, b = 2, c = 3;
int *arr[3] = {&a, &b, &c};   // 存储三个 int 指针

for (int i = 0; i < 3; i++) {
    printf("%d ", *arr[i]);   // 输出 1 2 3
}
```

常见应用：字符串数组

```c
char *strs[3] = {"hello", "world", "c"};
printf("%s\n", strs[0]);   // hello
```

---

## 3.6 二维数组与指针的关系

```c
int a[3][4] = {0};
int (*p)[4] = a;      // p 指向第一行（包含 4 个 int 的数组）

// a[i][j] 等价于 *(*(a+i)+j) 等价于 p[i][j]
```

按一维方式遍历二维数组：

```c
int a[3][4];
int *p = &a[0][0];
for (int i = 0; i < 12; i++) {
    p[i] = i;
}
```

行指针方式：

```c
int (*row)[4] = a;
row[1][2] = 100;   // 第二行第三列
```

---

## 3.7 数组指针 vs 指针数组 vs 指针的指针

| 定义 | 含义 | 示例 |
| ---- | ---- | ---- |
| `int *p[5]` | 指针数组：5 个 int 指针 | `p[0] = &a;` |
| `int (*p)[5]` | 数组指针：指向 int[5] 的指针 | `p = &arr;` |
| `int **p` | 指针的指针：指向 int 指针 | `p = &q;`（q 是 int*） |

二维数组传参时常用数组指针：

```c
void func(int (*matrix)[4], int rows) {
    // matrix 指向一行
}
```

---

# 4. 指针与字符串

## 4.1 字符指针与字符数组

```c
char str1[] = "hello";   // 数组，可修改
char *str2 = "world";    // 指针指向常量字符串，不可修改

str1[0] = 'H';           // 合法
// str2[0] = 'W';        // 未定义行为（通常段错误）
```

---

## 4.2 字符串数组（两种方式）

```c
// 方式1：二维字符数组（可修改）
char strs1[3][10] = {"abc", "def", "ghi"};

// 方式2：指针数组（指向常量字符串）
char *strs2[3] = {"abc", "def", "ghi"};
```

---

# 5. 指针与函数

## 5.1 指针作为函数参数（传址调用）

```c
void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

int x = 5, y = 10;
swap(&x, &y);   // x=10, y=5
```

---

## 5.2 数组参数的本质是指针

```c
void printArray(int arr[], int size) {
// 等价于 void printArray(int *arr, int size)
    for (int i = 0; i < size; i++) {
        printf("%d ", arr[i]);
    }
}
```

传递多维数组：

```c
void printMatrix(int mat[][4], int rows) {
// 等价于 void printMatrix(int (*mat)[4], int rows)
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < 4; j++) {
            printf("%d ", mat[i][j]);
        }
    }
}
```

**注意**：数组传参退化为指针，无法通过 `sizeof` 获取长度，必须额外传 `size`。

---

## 5.3 指针作为返回值

```c
int* max(int *a, int *b) {
    return (*a > *b) ? a : b;
}

int x = 5, y = 8;
int *p = max(&x, &y);
printf("%d\n", *p);   // 8
```

**注意**：不要返回局部变量的地址。

```c
int* error() {
    int a = 10;
    return &a;   // 危险：a 在函数结束时销毁
}
```

---

## 5.4 函数指针

指向函数的指针，用于回调、动态调用。

```c
// 声明：返回类型 (*指针名)(参数列表)
int add(int a, int b) {
    return a + b;
}

int (*fp)(int, int) = add;   // fp 指向 add
int result = fp(3, 4);       // 调用，result=7
```

函数指针作为参数：

```c
void apply(int *arr, int size, int (*f)(int)) {
    for (int i = 0; i < size; i++) {
        arr[i] = f(arr[i]);
    }
}

int square(int x) { return x * x; }

int data[] = {1, 2, 3};
apply(data, 3, square);   // data 变为 {1, 4, 9}
```

函数指针数组：

```c
int (*ops[4])(int, int) = {add, sub, mul, div};
int r = ops[0](10, 5);   // 10 + 5
```

---

# 6. 指针与结构体

## 6.1 指向结构体的指针

```c
struct Point {
    int x;
    int y;
};

struct Point p1 = {3, 4};
struct Point *ptr = &p1;

// 访问成员：使用 -> 运算符
printf("%d %d\n", ptr->x, ptr->y);   // 3 4
// 等价于 (*ptr).x
```

---

## 6.2 结构体指针作为函数参数

传递结构体指针可避免复制，效率高，且能修改原值。

```c
void move(struct Point *p, int dx, int dy) {
    p->x += dx;
    p->y += dy;
}

struct Point p = {0, 0};
move(&p, 5, -2);
```

const 保护：

```c
void print(const struct Point *p) {   // 只读访问
    printf("%d %d\n", p->x, p->y);
}
```

---

## 6.3 结构体中的指针成员（链表基础）

```c
struct Node {
    int value;
    struct Node *next;   // 指向自身类型的指针，用于链表
};
```

动态分配结构体：

```c
#include <stdlib.h>

struct Node *head = (struct Node*)malloc(sizeof(struct Node));
if (head == NULL) {
    printf("malloc failed\n");
    exit(1);
}
head->value = 10;
head->next = NULL;
```

typedef 简化：

```c
typedef struct Node {
    int data;
    struct Node *next;
} Node;

Node *create_node(int value) {
    Node *new_node = (Node*)malloc(sizeof(Node));
    if (new_node == NULL) {
        return NULL;
    }
    new_node->data = value;
    new_node->next = NULL;
    return new_node;
}
```

---

## 6.4 指向结构体数组的指针

```c
struct Point pts[3] = {{1,2}, {3,4}, {5,6}};
struct Point *p = pts;   // 指向第一个元素

p++;                     // 指向第二个元素
printf("%d %d\n", p->x, p->y);   // 3 4
```

---

# 7. 指针的高级主题

## 7.1 多级指针（指针的指针）

```c
int a = 10;
int *p = &a;
int **pp = &p;   // pp 指向 p

printf("%d\n", **pp);   // 10
**pp = 20;
printf("%d\n", a);      // 20
```

应用一：动态分配二维数组

```c
int **matrix = (int**)malloc(rows * sizeof(int*));
for (int i = 0; i < rows; i++) {
    matrix[i] = (int*)malloc(cols * sizeof(int));
}
// 访问 matrix[i][j]
```

应用二：修改指针本身

```c
void allocateInt(int **p) {
    *p = (int*)malloc(sizeof(int));
    **p = 100;
}

int *ptr = NULL;
allocateInt(&ptr);
printf("%d\n", *ptr);   // 100
free(ptr);
```

---

## 7.2 void 指针（泛型指针）

```c
void *ptr;   // 可指向任何类型

int a = 10;
double d = 3.14;

ptr = &a;
ptr = &d;    // 合法

// 使用时必须强制转换
int *p = (int*)ptr;
```

常用于通用函数（如 `memcpy`、`qsort`）。

---

## 7.3 指针与动态内存

```c
#include <stdlib.h>

// 分配
int *p = (int*)malloc(5 * sizeof(int));
if (p == NULL) {
    printf("分配失败\n");
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

# 8. 常见错误与调试

## 8.1 未初始化指针

```c
int *p;
*p = 5;    // 错误：p 指向未知地址
```

---

## 8.2 解引用空指针

```c
int *p = NULL;
printf("%d", *p);   // 段错误
```

---

## 8.3 内存泄漏

```c
int *p = malloc(10);
p = malloc(20);     // 丢失第一个块的地址，无法 free
```

---

## 8.4 返回局部变量地址

```c
int* f() {
    int a = 10;
    return &a;      // 危险
}
```

---

## 8.5 数组越界

```c
int arr[5];
int *p = arr;
*(p + 5) = 10;      // 越界
```

---

# 9. 指针与数组总结对比表

| 概念 | 声明 | 含义 | 步长 |
| ---- | ---- | ---- | ---- |
| 指向 int 的指针 | `int *p` | p 可指向 int | sizeof(int) |
| 指针数组 | `int *p[5]` | p 是含 5 个 int 指针的数组 | 数组元素是指针 |
| 数组指针（指向 int[5]） | `int (*p)[5]` | p 指向 int[5] 类型 | 5*sizeof(int) |
| 指针的指针 | `int **p` | p 指向 int* | sizeof(int*) |
| 二维数组的行指针 | `int (*p)[C]` | p 可指向 int[C]（行） | C*sizeof(int) |
| 函数指针 | `int (*p)(int, int)` | p 指向函数 | — |
| 返回指针的函数 | `int *f(int)` | f 是函数，返回 int* | — |

---

# 10. 推荐学习路线

```text
指针基础（取址、解引用）
↓
指针与一维数组
↓
指针运算与遍历
↓
指针数组与数组指针
↓
字符串指针
↓
函数参数传递（传址）
↓
函数指针与回调
↓
结构体指针与链表
↓
动态内存分配
↓
多级指针与 const
↓
综合项目实践
```

---

# 11. 推荐练习项目

## 初级

* 使用指针实现数组逆序
* 字符串拷贝函数（模拟 `strcpy`）

---

## 中级

* 动态数组（支持扩容）
* 单向链表的增删改查
* 函数指针实现排序策略（升序/降序）

---

## 高级

* 简单的内存池管理
* 泛型交换函数（利用 `void*`）
* 矩阵运算库（动态二维数组）

---

# 12. 核心速查

```c
&a    // 取地址
*p    // 解引用
ptr+1 // 指针加法（移动一个元素）
arr   // 数组名退化为指向首元素的指针（sizeof 和 & 除外）

int *p[5]             // 指针数组
int (*p)[5]           // 数组指针
int **p               // 二级指针
int (*fp)(int, int)   // 函数指针
int *f(int)           // 返回指针的函数
ptr->member           // 结构体指针访问成员
```