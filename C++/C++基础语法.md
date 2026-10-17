# 1. C++ 简介

C++ 是当前最常用的：

```text
通用、多范式编程语言
```

由 Bjarne Stroustrup 于 1985 年在 C 语言基础上开发，主要用于：

* 游戏开发（Unreal Engine）
* 高性能计算
* 操作系统与驱动
* 嵌入式系统
* 桌面应用（Qt）
* 金融交易系统
* 编译器与工具链

与 C 语言不同：

```text
C  ：面向过程，无类，无 STL
C++：面向对象 + 泛型编程，支持类、模板、STL
```

例如：

C：

```c
#include <stdio.h>

int main() {
    printf("Hello\n");
    return 0;
}
```

C++：

```cpp
#include <iostream>

int main() {
    std::cout << "Hello" << std::endl;
    return 0;
}
```

---

# 2. 编译与运行

## 2.1 安装编译器

### Linux

```bash
sudo apt install g++   # Ubuntu/Debian
sudo yum install gcc-c++   # CentOS/RHEL
```

### macOS

```bash
xcode-select --install
```

### Windows

```text
安装 MinGW-w64 或 Visual Studio
或使用 WSL
```

---

## 2.2 验证安装

```bash
g++ --version
```

输出：

```text
g++ (Ubuntu 11.4.0) 11.4.0
```

---

## 2.3 编写第一个程序

创建 `hello.cpp`：

```cpp
#include <iostream>

int main() {
    std::cout << "Hello, World!" << std::endl;
    return 0;
}
```

编译：

```bash
g++ hello.cpp -o hello
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
源文件 (.cpp)
   ↓ 预处理
预处理文件 (.ii)
   ↓ 编译
汇编文件 (.s)
   ↓ 汇编
目标文件 (.o)
   ↓ 链接
可执行文件
```

---

## 2.5 常用编译选项

```bash
g++ main.cpp -o main              # 基本编译
g++ -Wall -Wextra main.cpp -o main # 开启警告
g++ -g main.cpp -o main           # 带调试信息
g++ -O2 main.cpp -o main          # 优化
g++ -std=c++17 main.cpp -o main   # 指定标准
g++ -std=c++20 main.cpp -o main   # C++20
```

---

# 3. 程序结构

```cpp
#include <iostream>       // 头文件

#define PI 3.14159        // 宏定义

using namespace std;      // 使用命名空间

int global_var = 10;      // 全局变量

// 函数声明
int add(int a, int b);

int main() {
    int result = add(3, 5);
    cout << result << endl;
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
• using namespace：使用命名空间（简化代码）
• main 函数：程序入口，返回 int
• 函数声明：告诉编译器函数存在
```

---

# 4. 命名空间

## 4.1 什么是命名空间

```text
命名空间用于解决命名冲突
把不同库的同名标识符隔离开
```

---

## 4.2 定义与使用

```cpp
namespace MySpace {
    int value = 10;
    void func() {}
}

int main() {
    std::cout << MySpace::value << std::endl;   // 访问方式：命名空间::成员
    MySpace::func();
    return 0;
}
```

---

## 4.3 using 声明

```cpp
using namespace std;        // 引入整个命名空间（不推荐在头文件中使用）
using std::cout;            // 引入单个成员
```

---

## 4.4 匿名命名空间

```cpp
namespace {
    int internal_var = 10;   // 仅本文件可见
}
```

---

# 5. 输入输出

## 5.1 cout 输出

```cpp
#include <iostream>
using namespace std;

cout << "Hello" << endl;
cout << "数字：" << 123 << endl;
cout << "浮点：" << 3.14 << endl;

int a = 10;
cout << "a = " << a << endl;
```

`endl` 与 `\n` 区别：

```text
endl ：换行 + 刷新缓冲区
\n   ：仅换行
```

---

## 5.2 cin 输入

```cpp
int age;
cout << "请输入年龄：";
cin >> age;

double height;
cin >> height;

string name;
cin >> name;   // 遇空格停止

getline(cin, name);   // 读整行（含空格）
```

---

## 5.3 格式化输出（iomanip）

```cpp
#include <iomanip>

cout << fixed << setprecision(2) << 3.14159 << endl;   // 3.14
cout << setw(10) << 123 << endl;                        // 右对齐宽度 10
cout << hex << 255 << endl;                             // ff（十六进制）
cout << dec << 255 << endl;                             // 255（十进制）
```

---

# 6. 变量与数据类型

## 6.1 基本数据类型

| 类型 | 说明 | 大小（典型） |
| ---- | ---- | ------------ |
| bool | 布尔 | 1 字节 |
| char | 字符 | 1 字节 |
| int | 整数 | 4 字节 |
| float | 单精度浮点 | 4 字节 |
| double | 双精度浮点 | 8 字节 |
| void | 无类型 | - |

修饰符：

```text
short       ：短整型（2 字节）
long        ：长整型
long long   ：超长整型（8 字节）
unsigned    ：无符号
signed      ：有符号（默认）
```

---

## 6.2 变量声明与初始化

```cpp
int a = 10;             // C 风格
int b(20);              // 构造风格
int c{30};              // 列表初始化（C++11，推荐）

auto d = 3.14;          // 自动类型推导
auto e = 100;
```

---

## 6.3 常量

```cpp
const int MAX = 100;        // 运行时常量
constexpr int SIZE = 50;    // 编译时常量（C++11）

#define PI 3.14159          // 宏常量（不推荐）
```

区别：

```text
const    ：有类型，运行时常量
constexpr：编译时求值，性能更优
#define  ：预处理替换，无类型
```

---

## 6.4 sizeof

```cpp
cout << sizeof(int) << endl;      // 4
cout << sizeof(double) << endl;   // 8
```

---

# 7. 运算符

## 7.1 算术运算符

```cpp
int a = 10, b = 3;

cout << a + b << endl;    // 13
cout << a - b << endl;    // 7
cout << a * b << endl;    // 30
cout << a / b << endl;    // 3（整数除法）
cout << a % b << endl;    // 1
```

浮点除法：

```cpp
double x = 10.0 / 3;   // 3.333333
```

---

## 7.2 关系运算符

```cpp
cout << (3 > 2) << endl;    // 1
cout << (3 < 2) << endl;    // 0
cout << (3 == 3) << endl;   // 1
cout << (3 != 2) << endl;   // 1
```

---

## 7.3 逻辑运算符

```cpp
cout << (true && false) << endl;   // 0
cout << (true || false) << endl;   // 1
cout << (!true) << endl;           // 0
```

---

## 7.4 位运算符

```cpp
int a = 5;   // 0101
int b = 3;   // 0011

cout << (a & b) << endl;    // 1
cout << (a | b) << endl;    // 7
cout << (a ^ b) << endl;    // 6
cout << (~a) << endl;       // -6
cout << (a << 1) << endl;   // 10
cout << (a >> 1) << endl;   // 2
```

---

## 7.5 自增自减

```cpp
int a = 5;
cout << a++ << endl;   // 5（先输出再加）
cout << a << endl;     // 6

int b = 5;
cout << ++b << endl;   // 6（先加再输出）
```

---

## 7.6 三目运算符

```cpp
int a = 10, b = 20;
int max = (a > b) ? a : b;
cout << max << endl;   // 20
```

---

## 7.7 范围 for（C++11）

```cpp
int arr[] = {1, 2, 3, 4, 5};

for (int x : arr) {
    cout << x << " ";
}
```

---

# 8. 条件语句

## 8.1 if / else if / else

```cpp
int score = 85;

if (score >= 90) {
    cout << "优秀" << endl;
} else if (score >= 60) {
    cout << "及格" << endl;
} else {
    cout << "不及格" << endl;
}
```

---

## 8.2 switch

```cpp
int day = 3;

switch (day) {
    case 1:
        cout << "周一" << endl;
        break;
    case 2:
        cout << "周二" << endl;
        break;
    default:
        cout << "其他" << endl;
}
```

**注意**：每个 case 后要加 `break`，否则会“穿透”。

---

# 9. 循环

## 9.1 for 循环

```cpp
for (int i = 0; i < 5; i++) {
    cout << i << endl;
}
```

---

## 9.2 范围 for

```cpp
int arr[] = {1, 2, 3, 4, 5};

for (int x : arr) {
    cout << x << " ";
}

// 引用修改原值
for (int &x : arr) {
    x *= 2;
}
```

---

## 9.3 while 循环

```cpp
int i = 0;
while (i < 5) {
    cout << i << endl;
    i++;
}
```

---

## 9.4 do-while 循环

```cpp
int i = 0;
do {
    cout << i << endl;
    i++;
} while (i < 5);
```

---

## 9.5 break 与 continue

```cpp
for (int i = 0; i < 10; i++) {
    if (i == 5) break;        // 跳出循环
    if (i % 2 == 0) continue; // 跳过本次
    cout << i << endl;
}
```

---

# 10. 数组

## 10.1 一维数组

```cpp
int arr[5];                     // 未初始化
int arr2[5] = {1, 2, 3, 4, 5};  // 初始化
int arr3[] = {1, 2, 3};         // 自动推断长度

arr[0] = 10;
cout << arr2[2] << endl;        // 3
```

---

## 10.2 遍历数组

```cpp
int arr[5] = {1, 2, 3, 4, 5};

for (int i = 0; i < 5; i++) {
    cout << arr[i] << " ";
}

for (int x : arr) {
    cout << x << " ";
}
```

---

## 10.3 二维数组

```cpp
int matrix[3][4] = {
    {1, 2, 3, 4},
    {5, 6, 7, 8},
    {9, 10, 11, 12}
};

cout << matrix[1][2] << endl;   // 7

for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 4; j++) {
        cout << matrix[i][j] << " ";
    }
    cout << endl;
}
```

---

## 10.4 std::array（C++11）

```cpp
#include <array>

std::array<int, 5> arr = {1, 2, 3, 4, 5};

cout << arr.size() << endl;    // 5
cout << arr[0] << endl;        // 1
cout << arr.at(1) << endl;     // 2（带边界检查）
```

---

# 11. 字符串

## 11.1 C 风格字符串

```cpp
char str1[] = "hello";
char str2[20] = "world";

cout << strlen(str1) << endl;   // 5
```

需要 `#include <cstring>`。

---

## 11.2 std::string（推荐）

```cpp
#include <string>

string s1 = "hello";
string s2 = "world";

string s3 = s1 + " " + s2;      // 拼接
cout << s3 << endl;             // hello world
cout << s1.length() << endl;    // 5
cout << s1[0] << endl;          // h
cout << s1.substr(1, 3) << endl; // ell

s1 += "!";                       // 追加
s1.append("!!");                 // 追加

if (s1 == s2) { }                // 比较
if (s1.find("ell") != string::npos) { }  // 查找
```

---

## 11.3 字符串转换

```cpp
string s = "123";
int n = stoi(s);              // 字符串转 int
double d = stod("3.14");      // 字符串转 double

string s2 = to_string(100);   // 数字转字符串
```

---

# 12. 函数

## 12.1 定义与调用

```cpp
// 声明
int add(int a, int b);

int main() {
    int result = add(3, 5);
    cout << result << endl;
    return 0;
}

// 定义
int add(int a, int b) {
    return a + b;
}
```

---

## 12.2 默认参数

```cpp
void greet(string name = "World") {
    cout << "Hello, " << name << endl;
}

greet();           // Hello, World
greet("Alice");    // Hello, Alice
```

---

## 12.3 函数重载

```cpp
int add(int a, int b) {
    return a + b;
}

double add(double a, double b) {
    return a + b;
}

cout << add(3, 5) << endl;         // 8
cout << add(3.14, 2.86) << endl;   // 6.0
```

---

## 12.4 引用传递

```cpp
void swap(int &a, int &b) {    // 引用参数，直接修改原值
    int temp = a;
    a = b;
    b = temp;
}

int x = 5, y = 10;
swap(x, y);    // x=10, y=5
```

---

## 12.5 const 引用

```cpp
void print(const string &s) {   // 避免复制，且不修改
    cout << s << endl;
}
```

---

## 12.6 内联函数

```cpp
inline int square(int x) {
    return x * x;
}
```

---

## 12.7 递归

```cpp
int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

cout << factorial(5) << endl;   // 120
```

---

# 13. 指针与引用

## 13.1 指针

```cpp
int a = 10;
int *p = &a;

cout << *p << endl;    // 10
*p = 20;
cout << a << endl;     // 20

int *q = nullptr;      // 空指针（C++11）
```

---

## 13.2 引用

```cpp
int a = 10;
int &ref = a;          // ref 是 a 的别名

ref = 20;
cout << a << endl;     // 20
```

引用特点：

```text
• 必须初始化
• 不能重新绑定
• 无空引用
• 语法上像普通变量
```

---

## 13.3 指针 vs 引用

| 特性 | 指针 | 引用 |
| ---- | ---- | ---- |
| 可为空 | ✅ | ❌ |
| 可重新赋值 | ✅ | ❌ |
| 需要解引用 | ✅ | ❌ |
| 内存地址 | 独立变量 | 别名 |

---

## 13.4 const 与指针

```cpp
const int *p1;         // 指向常量的指针（不能改内容）
int * const p2 = &a;   // 常量指针（不能改指向）
const int * const p3 = &a;  // 两者都不可变
```

---

## 13.5 动态内存（new / delete）

```cpp
int *p = new int(10);         // 分配单个
cout << *p << endl;           // 10
delete p;

int *arr = new int[5];        // 分配数组
for (int i = 0; i < 5; i++) arr[i] = i;
delete[] arr;                 // 释放数组
```

---

## 13.6 智能指针（C++11）

```cpp
#include <memory>

// unique_ptr：独占所有权
unique_ptr<int> p1 = make_unique<int>(10);

// shared_ptr：共享所有权（引用计数）
shared_ptr<int> p2 = make_shared<int>(20);
shared_ptr<int> p3 = p2;   // 引用计数 +1

// weak_ptr：不增加引用计数
weak_ptr<int> wp = p2;
```

---

# 14. 结构体与类

## 14.1 结构体

```cpp
struct Point {
    int x;
    int y;
};

Point p1 = {3, 4};
cout << p1.x << endl;    // 3

Point *ptr = &p1;
cout << ptr->x << endl;  // 3
```

---

## 14.2 类

```cpp
class Person {
private:
    string name;
    int age;

public:
    // 构造函数
    Person(string n, int a) : name(n), age(a) {}

    // 成员函数
    void greet() {
        cout << "我是 " << name << "，今年 " << age << " 岁" << endl;
    }

    // getter / setter
    string getName() const { return name; }
    void setAge(int a) { age = a; }
};

int main() {
    Person p("Alice", 20);
    p.greet();
    return 0;
}
```

---

## 14.3 访问修饰符

```text
public    ：任意位置可访问
private   ：仅类内部可访问
protected ：类内部及派生类可访问
```

---

## 14.4 构造函数与析构函数

```cpp
class MyClass {
public:
    MyClass() {                    // 构造函数
        cout << "构造" << endl;
    }

    ~MyClass() {                   // 析构函数
        cout << "析构" << endl;
    }
};
```

初始化列表：

```cpp
class Point {
    int x, y;
public:
    Point(int a, int b) : x(a), y(b) {}   // 初始化列表
};
```

---

## 14.5 继承

```cpp
class Animal {
public:
    void eat() { cout << "eating" << endl; }
    virtual void speak() { cout << "..." << endl; }
    virtual ~Animal() {}    // 虚析构
};

class Dog : public Animal {
public:
    void speak() override { cout << "Woof" << endl; }  // 重写
};

Dog d;
d.eat();      // 继承的方法
d.speak();    // Woof
```

---

## 14.6 多态

```cpp
Animal *a = new Dog();
a->speak();   // Woof（虚函数动态绑定）
delete a;
```

---

## 14.7 运算符重载

```cpp
class Complex {
public:
    double real, imag;

    Complex(double r, double i) : real(r), imag(i) {}

    Complex operator+(const Complex &other) {
        return Complex(real + other.real, imag + other.imag);
    }
};

Complex c1(1, 2), c2(3, 4);
Complex c3 = c1 + c2;
```

---

## 14.8 this 指针

```cpp
class MyClass {
    int value;
public:
    void setValue(int value) {
        this->value = value;    // this 指向当前对象
    }
};
```

---

## 14.9 static 成员

```cpp
class Counter {
public:
    static int count;    // 静态成员变量（所有对象共享）

    Counter() { count++; }
};

int Counter::count = 0;   // 类外定义
```

---

# 15. STL 标准模板库

## 15.1 vector（动态数组）

```cpp
#include <vector>

vector<int> v = {1, 2, 3};

v.push_back(4);              // 添加元素
v.pop_back();                // 删除最后一个
cout << v.size() << endl;    // 长度
cout << v[0] << endl;        // 访问
cout << v.at(1) << endl;     // 带边界检查

for (int x : v) {
    cout << x << " ";
}

v.clear();                   // 清空
```

---

## 15.2 map（键值对）

```cpp
#include <map>

map<string, int> m;

m["apple"] = 3;
m["banana"] = 5;

cout << m["apple"] << endl;   // 3

for (auto &pair : m) {
    cout << pair.first << ": " << pair.second << endl;
}

if (m.count("apple")) { }     // 检查键是否存在
m.erase("apple");             // 删除
```

---

## 15.3 set（集合）

```cpp
#include <set>

set<int> s = {3, 1, 4, 1, 5};

s.insert(6);
s.erase(1);

if (s.count(3)) { }           // 是否存在
cout << s.size() << endl;

for (int x : s) {
    cout << x << " ";         // 自动排序：3 4 5 6
}
```

---

## 15.4 pair 和 tuple

```cpp
#include <utility>

pair<int, string> p = {1, "apple"};
cout << p.first << " " << p.second << endl;

auto p2 = make_pair(2, "banana");
```

---

## 15.5 迭代器

```cpp
vector<int> v = {1, 2, 3, 4, 5};

for (auto it = v.begin(); it != v.end(); ++it) {
    cout << *it << " ";
}
```

---

## 15.6 算法库

```cpp
#include <algorithm>

vector<int> v = {3, 1, 4, 1, 5};

sort(v.begin(), v.end());              // 排序
reverse(v.begin(), v.end());           // 反转
auto it = find(v.begin(), v.end(), 4); // 查找
int cnt = count(v.begin(), v.end(), 1); // 计数

int mx = *max_element(v.begin(), v.end());
int mn = *min_element(v.begin(), v.end());
```

---

# 16. 模板

## 16.1 函数模板

```cpp
template <typename T>
T max_val(T a, T b) {
    return (a > b) ? a : b;
}

cout << max_val(3, 5) << endl;         // 5
cout << max_val(3.14, 2.71) << endl;   // 3.14
```

---

## 16.2 类模板

```cpp
template <typename T>
class Box {
    T value;
public:
    Box(T v) : value(v) {}
    T get() { return value; }
};

Box<int> b1(10);
Box<string> b2("hello");
```

---

# 17. 异常处理

## 17.1 try / catch

```cpp
try {
    throw runtime_error("出错了");
} catch (const runtime_error &e) {
    cout << "捕获异常：" << e.what() << endl;
} catch (...) {
    cout << "未知异常" << endl;
}
```

---

## 17.2 标准异常

```cpp
#include <stdexcept>

throw invalid_argument("参数无效");
throw out_of_range("越界");
throw runtime_error("运行时错误");
```

---

## 17.3 自定义异常

```cpp
class MyException : public exception {
public:
    const char* what() const noexcept override {
        return "自定义异常";
    }
};
```

---

# 18. 文件操作

## 18.1 写文件

```cpp
#include <fstream>

ofstream out("data.txt");
if (out.is_open()) {
    out << "Hello" << endl;
    out << 123 << endl;
    out.close();
}
```

---

## 18.2 读文件

```cpp
ifstream in("data.txt");
string line;

if (in.is_open()) {
    while (getline(in, line)) {
        cout << line << endl;
    }
    in.close();
}
```

---

## 18.3 文件模式

```cpp
ofstream out("data.txt", ios::app);    // 追加
ofstream out("data.bin", ios::binary); // 二进制
```

---

# 19. 现代 C++ 特性（C++11 及以上）

## 19.1 auto 类型推导

```cpp
auto x = 10;           // int
auto y = 3.14;         // double
auto s = string("hi"); // string

for (auto &x : vec) { }
```

---

## 19.2 lambda 表达式

```cpp
auto add = [](int a, int b) { return a + b; };
cout << add(3, 5) << endl;

// 捕获外部变量
int x = 10;
auto f = [x](int y) { return x + y; };

// 引用捕获
auto g = [&x](int y) { x += y; };
```

---

## 19.3 范围 for

```cpp
vector<int> v = {1, 2, 3, 4, 5};

for (int x : v) {
    cout << x << " ";
}
```

---

## 19.4 nullptr

```cpp
int *p = nullptr;    // 替代 NULL
```

---

## 19.5 结构化绑定（C++17）

```cpp
pair<int, string> p = {1, "apple"};
auto [id, name] = p;

cout << id << " " << name << endl;
```

---

## 19.6 if 初始化语句（C++17）

```cpp
if (int x = getValue(); x > 0) {
    cout << x << endl;
}
```

---

## 19.7 移动语义

```cpp
#include <utility>

vector<int> v1 = {1, 2, 3};
vector<int> v2 = std::move(v1);   // 转移所有权，v1 变为空
```

---

# 20. 常见问题

## 1. 段错误（Segmentation Fault）

```text
原因：
• 访问空指针
• 数组越界
• 使用未初始化指针
• 释放已释放的内存
```

---

## 2. 内存泄漏

```cpp
int *p = new int(10);
// 忘记 delete p; → 内存泄漏

// 推荐使用智能指针
auto p = make_unique<int>(10);
```

---

## 3. 未定义行为

```text
• 有符号整数溢出
• 除以零
• 访问已释放内存
• 迭代器失效
```

---

## 4. 头文件重复包含

```cpp
// myheader.h
#ifndef MYHEADER_H
#define MYHEADER_H

// 内容

#endif
```

或：

```cpp
#pragma once
```

---

## 5. 深拷贝 vs 浅拷贝

```cpp
class MyClass {
    int *data;
public:
    // 需要自定义拷贝构造函数和赋值运算符
    MyClass(const MyClass &other) {
        data = new int(*other.data);
    }
};
```

---

# 21. 推荐学习路线

```text
安装编译器与第一个程序
↓
命名空间与输入输出
↓
变量与数据类型
↓
运算符
↓
条件语句与循环
↓
数组与字符串
↓
函数与重载
↓
引用与指针
↓
结构体与类
↓
面向对象（继承、多态）
↓
STL（vector、map、set、算法）
↓
模板与泛型编程
↓
异常处理
↓
智能指针与现代 C++ 特性
↓
综合项目练习
```

---

# 22. 推荐练习项目

## 初级

* 计算器（四则运算）
* 猜数字游戏
* 学生成绩统计

---

## 中级

* 通讯录管理（类 + 文件）
* 矩阵运算库（运算符重载）
* 简易字符串类（实现 STL string）

---

## 高级

* 简易 STL 容器实现（vector、list）
* 多态图形库（继承 + 虚函数）
* 线程池（C++11 并发）
* 简易游戏引擎框架

---

# 23. 核心语法速查

```cpp
// 头文件
#include <iostream>
#include <vector>
#include <string>

// 命名空间
using namespace std;

// 变量
int a = 10;
auto b = 3.14;
const int MAX = 100;

// 输入输出
cout << "Hello" << endl;
cin >> a;

// 条件
if (a > 5) { }
else { }

switch (a) {
    case 1: break;
    default: break;
}

// 循环
for (int i = 0; i < 10; i++) { }
for (int x : vec) { }
while (a > 0) { a--; }

// 数组
int arr[5] = {1, 2, 3, 4, 5};

// 函数
int add(int a, int b) { return a + b; }

// 引用与指针
int &ref = a;
int *p = &a;

// 类
class Person {
private:
    string name;
public:
    Person(string n) : name(n) {}
    void greet() { cout << name << endl; }
};

// STL
vector<int> v = {1, 2, 3};
v.push_back(4);
map<string, int> m;
set<int> s;
sort(v.begin(), v.end());

// 智能指针
auto p = make_unique<int>(10);
auto q = make_shared<int>(20);

// lambda
auto f = [](int x) { return x * 2; };

// 异常
try { throw runtime_error("error"); }
catch (const exception &e) { cout << e.what(); }

// 文件
ofstream out("file.txt");
ifstream in("file.txt");
```