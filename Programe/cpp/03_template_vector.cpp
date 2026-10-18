/* ==========================================================================
 * 03_template_vector.cpp —— 自己实现一个 std::vector
 *
 * 写完它你会同时搞懂：模板 / 原始内存 vs 对象 / placement new / 移动语义 /
 * 异常安全 / 迭代器到底是什么 / 为什么 push_back 是「摊销 O(1)」/
 * 为什么扩容后手里的指针和引用会全部失效。
 *
 * 最有意思的两点：
 *   1. 因为我们的迭代器就是 T*，std::sort、std::accumulate 可以「直接」用在
 *      MyVector 上 —— 泛型算法只要求接口，不要求继承，这是 C++ 模板的精髓。
 *   2. 同一个模板函数既能吃 MyVector 又能吃 std::vector，没有虚函数、没有开销。
 *
 * 编译：g++ -std=c++20 -Wall -Wextra cpp\03_template_vector.cpp -o out\cpp03.exe
 * 运行：out\cpp03.exe
 * ========================================================================== */
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <initializer_list>
#include <iostream>
#include <new>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

static void sec(std::string_view t) { std::cout << "\n===== " << t << " =====\n"; }

/* ---------- 全局计数器：让「构造/移动/析构」这些平时看不见的事曝光 ---------- */
struct Counters {
    static long ctor, copy_ctor, move_ctor, move_assign, dtor;
    static void reset() { ctor = copy_ctor = move_ctor = move_assign = dtor = 0; }
    static void report(std::string_view tag) {
        std::cout << std::format("  {}: 构造{} 拷贝构造{} 移动构造{} 移动赋值{} 析构{}\n",
                                 tag, ctor, copy_ctor, move_ctor, move_assign, dtor);
    }
};
long Counters::ctor = 0, Counters::copy_ctor = 0, Counters::move_ctor = 0;
long Counters::move_assign = 0, Counters::dtor = 0;

struct Hero {
    std::string name;
    int         power = 0;

    Hero() : name("无名") { ++Counters::ctor; }
    Hero(std::string n, int p) : name(std::move(n)), power(p) { ++Counters::ctor; }
    Hero(const Hero &o) : name(o.name), power(o.power) { ++Counters::copy_ctor; }
    Hero(Hero &&o) noexcept : name(std::move(o.name)), power(o.power)
    {
        o.power = 0;
        ++Counters::move_ctor;
    }
    Hero &operator=(const Hero &o) {
        if (this != &o) { name = o.name; power = o.power; ++Counters::copy_ctor; }
        return *this;
    }
    Hero &operator=(Hero &&o) noexcept {
        if (this != &o) { name = std::move(o.name); power = o.power; o.power = 0; ++Counters::move_assign; }
        return *this;
    }
    ~Hero() { ++Counters::dtor; }
};
/* ---------------- MyVector<T>：下面这一百来行就是 std::vector 的骨架 ---------------- */
template <class T>
class MyVector {
    static_assert(!std::is_reference_v<T>, "MyVector 不能存引用");
    static_assert(std::is_move_constructible_v<T>, "扩容要移动元素，T 必须可移动");

    T     *data_ = nullptr;     /* 指向「原始内存」；里面的对象是我们手动构造出来的 */
    size_t size_ = 0;           /* 已经构造了几个对象 */
    size_t cap_  = 0;           /* 内存能装几个 */

    static void destroy_range(T *p, size_t n)
    {
        for (size_t i = n; i-- > 0;) p[i].~T();          /* 逆序析构，和构造顺序相反 */
    }

    void reallocate(size_t nc)
    {
        T *nd = static_cast<T *>(::operator new(nc * sizeof(T)));  /* 只要内存，不要对象 */
        size_t i = 0;
        try {
            for (; i < size_; i++) new (&nd[i]) T(std::move(data_[i]));   /* 搬家 */
        } catch (...) {
            destroy_range(nd, i);         /* 搬到一半失败：清理新内存，旧数组完好无损 */
            ::operator delete(nd);
            throw;                        /* 强异常安全：要么成功，要么像没发生过 */
        }
        destroy_range(data_, size_);
        ::operator delete(data_);
        data_ = nd;
        cap_  = nc;
    }
public:
    using value_type       = T;
    using iterator         = T *;          /* 迭代器就是裸指针 —— 这点决定了后面能用 std::sort */
    using const_iterator   = const T *;

    MyVector() = default;

    explicit MyVector(size_t n)
        : data_(n ? static_cast<T *>(::operator new(n * sizeof(T))) : nullptr), size_(n), cap_(n)
    {
        for (size_t i = 0; i < n; i++) new (&data_[i]) T();
    }

    MyVector(std::initializer_list<T> init)
        : data_(init.size() ? static_cast<T *>(::operator new(init.size() * sizeof(T))) : nullptr),
          size_(init.size()), cap_(init.size())
    {
        size_t i = 0;
        for (const T &x : init) new (&data_[i++]) T(x);
    }

    MyVector(const MyVector &o)
        : data_(o.size_ ? static_cast<T *>(::operator new(o.size_ * sizeof(T))) : nullptr),
          size_(o.size_), cap_(o.size_)
    {
        for (size_t i = 0; i < size_; i++) new (&data_[i]) T(o.data_[i]);
    }

    MyVector(MyVector &&o) noexcept : data_(o.data_), size_(o.size_), cap_(o.cap_)
    {
        o.data_ = nullptr; o.size_ = 0; o.cap_ = 0;      /* 偷完要留个安全的空壳 */
    }

    MyVector &operator=(MyVector o) noexcept             /* copy-and-swap：一个函数同时实现两种赋值 */
    {
        std::swap(data_, o.data_);
        std::swap(size_, o.size_);
        std::swap(cap_,  o.cap_);
        return *this;                                    /* o 带着旧内存离开作用域被释放 */
    }

    ~MyVector() { destroy_range(data_, size_); ::operator delete(data_); }

    size_t size()     const { return size_; }
    size_t capacity() const { return cap_; }
    bool   empty()    const { return size_ == 0; }

    T       &operator[](size_t i)       { return data_[i]; }        /* 快，不检查 */
    const T &operator[](size_t i) const { return data_[i]; }
    T       &at(size_t i) {
        if (i >= size_) throw std::out_of_range("MyVector::at 下标越界");
        return data_[i];
    }
    const T &at(size_t i) const {
        if (i >= size_) throw std::out_of_range("MyVector::at 下标越界");
        return data_[i];
    }
    T       &front() { return data_[0]; }
    T       &back()  { return data_[size_ - 1]; }

    iterator       begin()       { return data_; }
    iterator       end()         { return data_ + size_; }
    const_iterator begin() const { return data_; }
    const_iterator end()   const { return data_ + size_; }
    void push_back(const T &x) {                      /* 拷贝版 */
        if (size_ == cap_) reallocate(cap_ ? cap_ * 2 : 4);
        new (&data_[size_]) T(x);
        ++size_;
    }
    void push_back(T &&x) {                           /* 移动版：省掉一次深拷贝 */
        if (size_ == cap_) reallocate(cap_ ? cap_ * 2 : 4);
        new (&data_[size_] ) T(std::move(x));
        ++size_;
    }
    template <class... Args>
    void emplace_back(Args &&...args)                 /* 原地构造，连临时对象都不产生 */
    {
        if (size_ == cap_) reallocate(cap_ ? cap_ * 2 : 4);
        new (&data_[size_]) T(std::forward<Args>(args)...);
        ++size_;
    }
    void pop_back()
    {
        if (size_) { data_[size_ - 1].~T(); --size_; }
    }
    iterator insert(const_iterator pos, const T &x)
    {
        size_t idx = (size_t)(pos - data_);
        if (idx >= size_) { push_back(x); return data_ + size_ - 1; }
        if (size_ == cap_) reallocate(cap_ ? cap_ * 2 : 4);   /* 注意：扩容后 pos 已失效，所以只用 idx */
        new (&data_[size_]) T(std::move(data_[size_ - 1]));   /* 尾巴先腾出一个已构造的位置 */
        for (size_t i = size_ - 1; i > idx; i--) data_[i] = std::move(data_[i - 1]);
        data_[idx] = x;
        ++size_;
        return data_ + idx;
    }
    iterator erase(const_iterator pos)
    {
        size_t idx = (size_t)(pos - data_);
        for (size_t i = idx; i + 1 < size_; i++) data_[i] = std::move(data_[i + 1]);
        data_[size_ - 1].~T();
        --size_;
        return data_ + idx;
    }
    void clear() { destroy_range(data_, size_); size_ = 0; }
    void reserve(size_t n) { if (n > cap_) reallocate(n); }
    void resize(size_t n)
    {
        if (n < size_) { destroy_range(data_ + n, size_ - n); size_ = n; }
        else if (n > size_) {
            if (n > cap_) reallocate(n);
            for (size_t i = size_; i < n; i++) new (&data_[i]) T();
            size_ = n;
        }
    }
};
/* ---------------- 一个模板函数，两种容器都能吃 ---------------- */
template <class Container>
static double average(const Container &c)
{
    if (c.empty()) return 0.0;
    using T = typename Container::value_type;
    return (double)std::accumulate(c.begin(), c.end(), T{0}) / (double)c.size();
}

static std::string join_ints(const int *p, size_t n, std::string_view sep)
{
    std::string s;
    for (size_t i = 0; i < n; i++) { if (i) s += sep; s += std::to_string(p[i]); }
    return s;
}

static void demo_growth()
{
    sec("1. 扩容过程：看清 vector 的钱花在哪里");
    MyVector<Hero> v;
    Counters::reset();
    for (int i = 0; i < 8; i++) {
        v.emplace_back("H" + std::to_string(i), i * 10);
        std::cout << std::format("  push 第 {} 个：size={} capacity={}\n", i + 1, v.size(), v.capacity());
    }
    Counters::report("emplace_back 8 次");
    std::cout << "  capacity 用满就翻倍 -> 每个元素平均只被搬 O(1) 次，这就是「摊销常数」\n";

    Counters::reset();
    MyVector<Hero> a = v;                     /* 深拷贝：8 次拷贝构造 */
    Counters::report("把整个 vector 拷贝一份");
    Counters::reset();
    MyVector<Hero> b = std::move(a);          /* 移动：只是换指针 */
    Counters::report("把整个 vector 移动一份");
    std::cout << std::format("  移动后 a.size()={}  b.size()={}  -> 成本从 O(n) 变成 O(1)\n",
                             a.size(), b.size());
}

static void demo_algorithms()
{
    sec("2. std::sort / std::accumulate 直接作用在 MyVector 上");
    MyVector<int> nums = {5, 3, 9, 1, 7, 2, 8};
    std::sort(nums.begin(), nums.end());      /* 迭代器就是 int*，标准算法无需任何改动 */
    std::cout << "  排序后：" << join_ints(nums.begin(), nums.size(), " ") << "\n";
    std::cout << "  MyVector 的平均值 = " << average(nums) << "\n";
    std::vector<int> stdv{1, 2, 3, 4, 5, 6};
    std::cout << "  std::vector 的平均值 = " << average(stdv)
              << "  （同一个模板函数，没有虚函数、没有额外开销）\n";

    nums.insert(nums.begin() + 2, 999);
    std::cout << std::format("  在下标 2 插入 999 -> size={}，第 2 个 = {}\n", nums.size(), nums[2]);
    nums.erase(nums.begin());
    std::cout << std::format("  删除第 0 个       -> size={}，现在开头是 {}\n", nums.size(), nums[0]);

    try {
        nums.at(10000) = 1;
    } catch (const std::out_of_range &e) {
        std::cout << std::format("  at() 挡住了越界：{}\n", e.what());
    }
    std::cout << "  注意 insert 里做了「先腾位再后移」，用的是移动赋值而不是拷贝 —— 存 Hero 时省掉一整套字符串拷贝\n";
}

static void demo_dangling()
{
    sec("3. 最重要的一课：扩容会让指针 / 引用 / 迭代器全部失效");
    MyVector<int> v;
    v.push_back(1);
    const int *p = &v[0];                     /* 只记地址，绝不解引用（那会是未定义行为） */
    for (int i = 0; i < 64; i++) v.push_back(i);
    std::cout << std::format("  旧地址 0x{:X}  新首元素地址 0x{:X}  ->  {}\n",
                             (uintptr_t)p, (uintptr_t)&v[0],
                             p == &v[0] ? "居然相同" : "完全不同：旧指针指向的是已被释放的内存");
    std::cout << "  这就是「存了 vector 元素的指针，后来又 push_back」这类 C++ 经典事故的原理。\n";
    std::cout << "  三条出路：存下标 / 先 reserve 够再填 / 改用 std::deque、std::list（元素地址稳定）。\n";
}

static void demo_perf()
{
    sec("4. 和 std::vector 同规模对比（300 万次 push_back）");
    const int N = 3000000;
    using clk = std::chrono::steady_clock;

    auto t0 = clk::now();
    MyVector<int> mine;
    for (int i = 0; i < N; i++) mine.push_back(i);
    double ms_mine = std::chrono::duration<double, std::milli>(clk::now() - t0).count();

    auto t1 = clk::now();
    std::vector<int> stl;
    for (int i = 0; i < N; i++) stl.push_back(i);
    double ms_stl = std::chrono::duration<double, std::milli>(clk::now() - t1).count();

    auto t2 = clk::now();
    std::vector<int> stl2;
    stl2.reserve(N);
    for (int i = 0; i < N; i++) stl2.push_back(i);
    double ms_stl2 = std::chrono::duration<double, std::milli>(clk::now() - t2).count();

    std::cout << std::format("  MyVector    自动扩容 : {:8.1f} ms\n", ms_mine);
    std::cout << std::format("  std::vector 自动扩容 : {:8.1f} ms\n", ms_stl);
    std::cout << std::format("  std::vector reserve  : {:8.1f} ms  <-- 知道规模就先 reserve\n", ms_stl2);
    std::cout << std::format("  自写版本存了 {} 个元素，最后一个 = {}（内容正确）\n", mine.size(), mine[N - 1]);
}

int main()
{
    std::cout << "写完这 200 行，vector 就不再是黑盒了：\n";
    std::cout << "它 = 一块 T* 原始内存 + size + capacity + 特殊成员函数 + 一堆「搬对象」的技巧。\n";
    demo_growth();
    demo_algorithms();
    demo_dangling();
    demo_perf();
    std::cout << "\n程序结束，所有 MyVector 都在离开作用域时自动析构 —— 全程没写一句 delete。\n";
    std::cout << "把 Counters::reset()/report() 插到任何两行之间，就能看清每一步到底在拷贝还是在移动。\n";
    return 0;
}



