/* ==========================================================================
 * 02_raii_smartptr.cpp —— C++ 管理资源的正统方式：RAII + 智能指针
 *
 * 对照 c/01_memory_and_pointers.c：那里每个 malloc 都要手写对应的 free。
 * C++ 的答案是「让析构函数替你 free」，于是：
 *   - 无论正常返回、提前 return、还是抛异常，资源都必然被释放
 *   - 代码里看不到一句 free/close/ReleaseMutex，却不会泄漏
 *
 * 演示能力：
 *   1. RAII 护栏：计时器、文件、Win32 句柄、mutex 锁（lock_guard 的原理）
 *   2. 构造/析构顺序：栈对象逆序销毁 —— RAII 能成立的根本原因
 *   3. unique_ptr：独占所有权、只能 move、自定义 deleter
 *   4. shared_ptr / weak_ptr：引用计数，以及循环引用怎么导致泄漏（+ 怎么修好）
 *   5. 内存池 + unique_ptr：把「分配策略」也变成类型系统的一部分
 *   6. Rule of Five：手写一个含裸资源的类，5 个特殊成员函数一个不能少
 *
 * 编译：g++ -std=c++20 -Wall -Wextra cpp\02_raii_smartptr.cpp -o out\cpp02.exe
 * 运行：out\cpp02.exe
 * ========================================================================== */
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

static void sec(std::string_view t) { std::cout << "\n===== " << t << " =====\n"; }

/* ---------------- 1. RAII：三个「离开作用域自动收尾」的护栏 ---------------- */
class ScopedTimer {                 /* 计时：析构时自动打印耗时 */
    const char      *name_;
    std::chrono::steady_clock::time_point t0_;
public:
    explicit ScopedTimer(const char *name) : name_(name), t0_(std::chrono::steady_clock::now()) {}
    ~ScopedTimer() {
        auto ms = std::chrono::duration<double, std::milli>(
                      std::chrono::steady_clock::now() - t0_).count();
        std::cout << std::format("  [计时] {} 用了 {:.2f} ms\n", name_, ms);
    }
    ScopedTimer(const ScopedTimer &) = delete;      /* 计时器不允许复制，语义上唯一 */
    ScopedTimer &operator=(const ScopedTimer &) = delete;
};

class FileGuard {                   /* 文件：构造打开，析构关闭 */
    std::FILE *fp_ = nullptr;
public:
    explicit FileGuard(const char *path, const char *mode) : fp_(std::fopen(path, mode)) {
        std::cout << std::format("  [文件] fopen({}) = {}\n", path, fp_ ? "OK" : "失败");
    }
    ~FileGuard() { if (fp_) { std::fclose(fp_); std::cout << "  [文件] 自动 fclose\n"; } }
    std::FILE *get() const { return fp_; }
    FileGuard(const FileGuard &) = delete;
    FileGuard &operator=(const FileGuard &) = delete;
};

class LockGuard {                   /* 这就是 std::lock_guard 的实现原理 */
    std::mutex &m_;
public:
    explicit LockGuard(std::mutex &m) : m_(m) { m_.lock(); std::cout << "  [锁] lock\n"; }
    ~LockGuard() { m_.unlock(); std::cout << "  [锁] unlock（即使中间抛异常也会执行）\n"; }
};

static std::mutex g_mtx;

static void demo_raii()
{
    sec("1. RAII：资源获取即初始化");
    {
        ScopedTimer t("一段故意慢的代码");
        std::vector<int> v;
        for (int i = 0; i < 3000000; i++) v.push_back(i % 977);
        std::sort(v.begin(), v.end());
    }                                        /* 这里没有任何手动调用，计时已打印 */

    {
        FileGuard f("cpp02_raii.txt", "w");
        if (f.get()) std::fprintf(f.get(), "written by RAII demo\n");
    }                                        /* 文件已关闭 */
    std::ifstream back("cpp02_raii.txt");
    std::string line;
    std::getline(back, line);
    std::cout << std::format("  复读刚才的文件：\"{}\"\n", line);

    {
        /* 抛异常也不会漏锁：栈展开时会调用局部对象的析构函数 —— RAII 的最大卖点 */
        LockGuard lk(g_mtx);
        std::cout << "  临界区里的输出\n";
        throw std::runtime_error("模拟中途出错");
    }
    std::cout << "  g_mtx 还可用吗？";
    std::lock_guard<std::mutex> relock(g_mtx);   /* 能再次加锁 = 上面的锁确实被释放了 */
    std::cout << " 能，说明异常时锁被自动释放了\n";
}

/* ---------------- 2. 构造 / 析构顺序 ---------------- */
struct Trace {
    std::string tag;
    explicit Trace(std::string t) : tag(std::move(t)) { std::cout << "  构造 " << tag << "\n"; }
    ~Trace() { std::cout << "  析构 " << tag << "\n"; }
};

static void demo_order()
{
    sec("2. 栈对象逆序销毁 —— RAII 的可靠性来自这个语言规则");
    Trace a("外层1");
    {
        Trace b("内层A");
        Trace c("内层B");
        std::cout << "  （离开内层作用域）\n";
    }
    Trace vec_holder("最后一个");
    std::cout << "  main 段结束后，顺序是：B, A, 外层1... 注意 vec_holder 在它们之前\n";
}

/* ---------------- 3. Rule of Five：手写一个持有裸资源的类 ---------------- */
class Buffer {
    int   *p_ = nullptr;
    size_t n_ = 0;
    static int n_ctor, n_dtor, n_copy, n_move;
public:
    Buffer() = default;
    explicit Buffer(size_t n) : p_(n ? new int[n] : nullptr), n_(n) { ++n_ctor; }

    /* 析构 */
    ~Buffer() { delete[] p_; ++n_dtor; }

    /* 拷贝构造：深拷贝，否则两份指针共享 -> double free */
    Buffer(const Buffer &o) : p_(o.n_ ? new int[o.n_] : nullptr), n_(o.n_) {
        std::copy_n(o.p_, n_, p_);
        ++n_copy;
    }
    /* 拷贝赋值（copy-and-swap 惯用法：自赋值安全和异常安全一起解决） */
    Buffer &operator=(Buffer o) {            /* 注意：按值传参 = 已经复制好 */
        std::swap(p_, o.p_);
        std::swap(n_, o.n_);
        return *this;
    }
    /* 移动构造：偷走资源，不复制 */
    Buffer(Buffer &&o) noexcept : p_(o.p_), n_(o.n_) { o.p_ = nullptr; o.n_ = 0; ++n_move; }
    /* 移动赋值 */
    Buffer &operator=(Buffer &&o) noexcept {
        if (this != &o) { delete[] p_; p_ = o.p_; n_ = o.n_; o.p_ = nullptr; o.n_ = 0; ++n_move; }
        return *this;
    }

    size_t size() const { return n_; }
    static void report() {
        std::cout << std::format("  Buffer: ctor={} dtor={} copy={} move={}  (未释放数={})\n",
                                 n_ctor, n_dtor, n_copy, n_move, n_ctor - n_dtor);
    }
};
int Buffer::n_ctor = 0, Buffer::n_dtor = 0, Buffer::n_copy = 0, Buffer::n_move = 0;

static void demo_rule_of_five()
{
    sec("3. Rule of Five：一次复制/移动的开销对比");
    Buffer a(1000000);               /* 4MB 的堆数组 */
    Buffer b = a;                    /* 拷贝：又分配 4MB 再 memcpy */
    Buffer c = std::move(a);         /* 移动：只是偷指针，几乎零成本 */
    Buffer d(10);
    d = c;                           /* 拷贝赋值 */
    d = std::move(b);                /* 移动赋值 */
    std::cout << std::format("  a.size()={}（被移空）  c.size()={}（被移走）  d.size()={}\n",
                             a.size(), c.size(), d.size());
    Buffer::report();
    std::cout << "  这就是为什么容器里放「大对象」时，移动语义能让速度差一个数量级\n";
}
/* ---------------- 4. unique_ptr：唯一所有权 ---------------- */
struct Widget {
    std::string name;
    explicit Widget(std::string n) : name(std::move(n)) { std::cout << "  + Widget " << name << "\n"; }
    ~Widget() { std::cout << "  - Widget " << name << "\n"; }
    void use() const { std::cout << "  使用 " << name << "\n"; }
};

static std::unique_ptr<Widget> create_widget(std::string n)
{
    return std::make_unique<Widget>(std::move(n));
}

static void demo_unique()
{
    sec("4. unique_ptr：一行 delete 都不用写，也绝不会漏");
    auto w = create_widget("独占者");
    w->use();

    auto p = std::make_unique<Widget>("移交者");
    /* auto q = p; */          /* <-- 取消注释就编译失败：所有权不许有两个主人 */
    auto q = std::move(p);      /*  OK：责任转移，p 变空 */
    std::cout << std::format("  移交之后 p == nullptr ? {}\n", p == nullptr);

    /* 容器装 unique_ptr：删元素 = 立刻析构，出作用域 = 全部自动销毁 */
    std::vector<std::unique_ptr<Widget>> team;
    team.push_back(create_widget("队员1"));
    team.push_back(create_widget("队员2"));
    team.push_back(create_widget("队员3"));
    std::cout << "  现在删掉队首：\n";
    team.erase(team.begin());
    std::cout << std::format("  队里还剩 {} 人\n", team.size());

    /* 自定义 deleter：管 FILE* 这种「不是 new 出来的」资源 */
    auto closer = [](std::FILE *f) {
        if (f) { std::fclose(f); std::cout << "  自定义 deleter 关闭了 FILE*\n"; }
    };
    std::unique_ptr<std::FILE, decltype(closer)> fp(std::fopen("cpp02_file.txt", "w"), closer);
    if (fp) std::fprintf(fp.get(), "managed by unique_ptr\n");
}

/* ---------------- 5. shared_ptr / weak_ptr ---------------- */
struct Tracer {
    std::string id;
    explicit Tracer(std::string i) : id(std::move(i)) { std::cout << "  + " << id << "\n"; }
    ~Tracer() { std::cout << "  - " << id << "\n"; }
};
struct BadNode {
    std::string id;
    std::shared_ptr<BadNode> parent, child;
    explicit BadNode(std::string i) : id(std::move(i)) { std::cout << "  + bad:" << id << "\n"; }
    ~BadNode() { std::cout << "  - bad:" << id << "\n"; }
};
struct GoodNode {
    std::string id;
    std::shared_ptr<GoodNode> child;
    std::weak_ptr<GoodNode>   parent;        /* 反向引用一律用 weak_ptr */
    explicit GoodNode(std::string i) : id(std::move(i)) { std::cout << "  + good:" << id << "\n"; }
    ~GoodNode() { std::cout << "  - good:" << id << "\n"; }
};

static void demo_shared()
{
    sec("5. shared_ptr 计数 / weak_ptr 破解循环引用");
    auto sp = std::make_shared<Tracer>("共享对象");
    std::cout << std::format("  use_count = {}\n", sp.use_count());
    {
        auto sp2 = sp;                        /* 多一个主人 */
        std::cout << std::format("  多一份拷贝后 use_count = {}\n", sp.use_count());
    }
    std::cout << std::format("  离开内层作用域后 use_count = {}（对象当然还活着）\n", sp.use_count());

    std::cout << "\n  --- 错误示范：父 child->子，子 parent->父，两边都是 shared_ptr ---\n";
    {
        auto root = std::make_shared<BadNode>("root");
        auto leaf = std::make_shared<BadNode>("leaf");
        root->child  = leaf;
        leaf->parent = root;                  /* 成环！两个计数都下不到 0 */
    }
    std::cout << "  ↑ 上面没有任何 \"- bad:\" 输出 —— 一对对象永远泄漏在堆上\n";

    std::cout << "  --- 正确示范：把反向那条边换成 weak_ptr ---\n";
    {
        auto root = std::make_shared<GoodNode>("root");
        auto leaf = std::make_shared<GoodNode>("leaf");
        root->child  = leaf;
        leaf->parent = root;
        if (auto locked = leaf->parent.lock())
            std::cout << std::format("    weak_ptr.lock() 取到了还活着的 {}（root 引用数={}）\n",
                                     locked->id, root.use_count());
        std::cout << "    weak_ptr 不延长寿命：它只回答「对象还在吗？在就借我用一下」\n";
    }
    std::cout << "  ↑ 这次两行 \"- good:\" 都打出来了，说明真的被释放了\n";
}
/* ---------------- 6. 内存池：new/delete 成为瓶颈时怎么办 ---------------- */
class BlockPool {
    static constexpr size_t kBlock = 128;
    std::vector<std::unique_ptr<char[]>> chunks_;
    std::vector<void *> free_;
    char  *cur_  = nullptr;
    size_t left_ = 0;
public:
    size_t n_alloc = 0, n_reuse = 0, n_chunk = 0;

    void *acquire()
    {
        ++n_alloc;
        if (!free_.empty()) { ++n_reuse; void *p = free_.back(); free_.pop_back(); return p; }
        if (left_ < kBlock) {
            chunks_.push_back(std::make_unique<char[]>(kBlock * 1024));
            cur_  = chunks_.back().get();
            left_ = kBlock * 1024;
            ++n_chunk;
        }
        void *p = cur_;
        cur_  += kBlock;
        left_ -= kBlock;
        return p;
    }
    void recycle(void *p) { free_.push_back(p); }
};

struct Particle { double x, y, z, vx, vy, vz; char tag[64]; };

static void demo_pool()
{
    sec("6. 同样造 30 万个对象：逐个 new vs 走自己的池");
    const int N = 300000;
    {
        ScopedTimer t("std::make_unique 逐个分配");
        std::vector<std::unique_ptr<Particle>> a;
        a.reserve(N);
        for (int i = 0; i < N; i++) a.push_back(std::make_unique<Particle>());
    }
    BlockPool pool;
    {
        ScopedTimer t("内存池 acquire + 复用");
        std::vector<Particle *> v;
        v.reserve(N);
        for (int i = 0; i < N; i++) v.push_back(new (pool.acquire()) Particle);  /* 定位 new */
        for (auto *p : v) { p->~Particle(); pool.recycle(p); }                   /* 手动析构 + 还块 */
    }
    std::cout << std::format("  池统计：acquire {} 次，其中 {} 次是复用空闲块；只找系统要了 {} 大块\n",
                             pool.n_alloc, pool.n_reuse, pool.n_chunk);

    /* 让 unique_ptr 用池回收：deleter 把「怎么分配」和「怎么释放」焊在一起 */
    auto rel = [&pool](Particle *p) { p->~Particle(); pool.recycle(p); };
    {
        std::unique_ptr<Particle, decltype(rel)> one(new (pool.acquire()) Particle, rel);
        one->x = 1.0;
        std::cout << "  这个 Particle 出作用域时不调用 operator delete，而是回到池里等着被复用\n";
    }
    (void)rel;
}

int main()
{
    std::cout << "C  的写法：malloc -> 一路小心每个 return / 异常 -> free\n";
    std::cout << "C++ 的写法：构造。「释放」交给编译器和作用域。\n";
    demo_raii();
    demo_order();
    demo_rule_of_five();
    demo_unique();
    demo_shared();
    demo_pool();
    std::cout << "\n本程序几乎没写 delete/fclose，也不泄漏（BadNode 那一处是「故意的泄漏」）。\n";
    return 0;
}


