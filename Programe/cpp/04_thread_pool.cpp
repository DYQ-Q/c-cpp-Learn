/* ==========================================================================
 * 04_thread_pool.cpp —— 用 C++ 标准库做并发（不碰任何 Win32 API）
 *
 * 对照 c/05_win32_threads.c：那里 CreateThread 要自己管 HANDLE、自己等、自己关。
 * C++ 的标准答案是 <thread> / <mutex> / <condition_variable> / <future> /
 * <atomic>，跨平台、且配合 RAII 几乎不可能忘记释放。
 *
 * 演示能力：
 *   1. 手写线程池：工作线程 + 任务队列 + 条件变量唤醒 + 优雅停止（析构里 join）
 *   2. submit() 返回 std::future<T>：调用方拿到的不是「结果」，而是「结果的承诺」
 *   3. 异常可以跨线程传递（future.get() 会重新抛出）
 *   4. Map-Reduce：分块并行求和，并和串行版本实测对比
 *   5. 有界缓冲区生产者/消费者：两个条件变量（非空、非满）
 *   6. std::atomic 与数据竞争的修复
 *
 * 编译：g++ -std=c++20 -O2 -Wall -Wextra cpp\04_thread_pool.cpp -o out\cpp04.exe
 * 运行：out\cpp04.exe
 * ========================================================================== */
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <format>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

static void sec(std::string_view t) { std::cout << "\n===== " << t << " =====\n"; }

/* ---------------- 线程池本体 ---------------- */
class ThreadPool {
public:
    explicit ThreadPool(size_t n)
    {
        if (n == 0) n = 1;
        workers_.reserve(n);
        for (size_t i = 0; i < n; i++)
            workers_.emplace_back([this] { this->worker_loop(); });
        std::cout << std::format("  线程池启动 {} 个工作线程（本机建议核数 {}）\n",
                                 n, std::thread::hardware_concurrency());
    }

    /* 析构 = 停止 + 等所有任务做完 + join。RAII：不会忘记关线程 */
    ~ThreadPool()
    {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            stop_ = true;
        }
        cv_.notify_all();
        for (auto &t : workers_) if (t.joinable()) t.join();
        std::cout << std::format("  线程池关闭，共执行 {} 个任务\n", done_.load());
    }

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    /* 提交任务，拿到 future —— 这就是「异步调用但保持同步写法」的关键 */
    template <class F, class... Args>
    auto submit(F &&f, Args &&...args) -> std::future<std::invoke_result_t<F, Args...>>
    {
        using Ret = std::invoke_result_t<F, Args...>;
        auto bound = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        auto pt = std::make_shared<std::packaged_task<Ret()>>(std::move(bound));
        std::future<Ret> fut = pt->get_future();
        {
            std::lock_guard<std::mutex> lk(mtx_);
            tasks_.emplace([pt] { (*pt)(); });      /* 任务本体：执行 packaged_task */
        }
        cv_.notify_one();
        return fut;
    }

    size_t pending() const { std::lock_guard<std::mutex> lk(mtx_); return tasks_.size(); }
    long   completed() const { return done_.load(); }

private:
    void worker_loop()
    {
        for (;;) {
            std::function<void()> task;
            {
                /* 条件变量必须配 unique_lock；wait 会在等待时自动放锁、醒来时重新拿锁 */
                std::unique_lock<std::mutex> lk(mtx_);
                cv_.wait(lk, [this] { return stop_ || !tasks_.empty(); });
                if (stop_ && tasks_.empty()) return;      /* 干完存量再退 */
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();                                        /* 锁已经放掉，别在锁里跑任务 */
            ++done_;
        }
    }

    std::vector<std::thread>       workers_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex             mtx_;
    std::condition_variable        cv_;
    bool                           stop_ = false;
    std::atomic<long>              done_{0};
};
/* ---------------- 1. Map-Reduce：把一个大批活切成 K 份 ---------------- */
static long partial_sum_range(long lo, long hi)      /* 纯函数：不碰共享数据 -> 天生线程安全 */
{
    long s = 0;
    for (long i = lo; i < hi; i++) s += (i % 7 == 0 ? i : i % 13);
    return s;
}

static void demo_map_reduce(ThreadPool &pool)
{
    sec("1. Map-Reduce：并行求和，并量出到底快多少");
    const long N = 20000000;

    auto t0 = std::chrono::steady_clock::now();
    long serial = partial_sum_range(0, N);
    double ms_serial = std::chrono::duration<double, std::milli>(
                           std::chrono::steady_clock::now() - t0).count();

    const int K = 4;
    auto t1 = std::chrono::steady_clock::now();
    std::vector<std::future<long>> futs;
    for (int i = 0; i < K; i++) {
        long lo = N * i / K, hi = N * (i + 1) / K;
        futs.push_back(pool.submit(partial_sum_range, lo, hi));   /* 立刻返回 future，不阻塞 */
    }
    long total = 0;
    for (auto &f : futs) total += f.get();                         /* get() 在这里才真的等 */
    double ms_par = std::chrono::duration<double, std::milli>(
                        std::chrono::steady_clock::now() - t1).count();

    std::cout << std::format("  串行  ：{}  用时 {:.1f} ms\n", serial, ms_serial);
    std::cout << std::format("  4 任务：{}  用时 {:.1f} ms   加速比 {:.2f}x\n",
                             total, ms_par, ms_serial / (ms_par > 0 ? ms_par : 1));
    std::cout << "  结果一致 = 切分正确；future 让「等结果」这件事有了类型安全的写法\n";
}

/* ---------------- 2. 异常也能跨线程传回来 ---------------- */
static int risky(bool boom)
{
    if (boom) throw std::runtime_error("线程里出的错");
    return 42;
}

static void demo_exception(ThreadPool &pool)
{
    sec("2. future 会把异常一起搬回调用方");
    auto ok  = pool.submit(risky, false);
    auto bad = pool.submit(risky, true);
    std::cout << std::format("  正常任务 get() = {}\n", ok.get());
    try {
        std::cout << bad.get();                 /* 这里重新抛出子线程里的异常 */
    } catch (const std::exception &e) {
        std::cout << std::format("  捕获到子线程的异常：{}\n", e.what());
    }
    std::cout << "  换成 C 的 CreateThread，你得自己设计一个「错误码字段」来传这件事\n";
}

/* ---------------- 3. 生产者 / 消费者：有界缓冲区 ---------------- */
class BoundedBuffer {
public:
    explicit BoundedBuffer(size_t cap) : cap_(cap) {}

    void put(int v)
    {
        std::unique_lock<std::mutex> lk(mtx_);
        not_full_.wait(lk, [&] { return q_.size() < cap_ || closed_; });
        if (closed_) return;
        q_.push(v);
        not_empty_.notify_one();
    }
    bool get(int &out)
    {
        std::unique_lock<std::mutex> lk(mtx_);
        not_empty_.wait(lk, [&] { return !q_.empty() || closed_; });
        if (q_.empty()) return false;            /* 已关闭且排空 */
        out = q_.front();
        q_.pop();
        not_full_.notify_one();
        return true;
    }
    void close()
    {
        std::lock_guard<std::mutex> lk(mtx_);
        closed_ = true;
        not_empty_.notify_all();
        not_full_.notify_all();
    }
private:
    std::deque<int>       q_;
    size_t                cap_;
    bool                  closed_ = false;
    std::mutex            mtx_;
    std::condition_variable not_empty_, not_full_;
};

static void demo_producer_consumer()
{
    sec("3. 生产者/消费者：两个条件变量（非空、非满）");
    BoundedBuffer buf(8);                        /* 容量 8：生产者会被堵住 */
    std::atomic<long> produced{0}, consumed{0};

    std::thread prod([&] {
        for (int i = 1; i <= 100000; i++) { buf.put(i); ++produced; }
        buf.close();
    });
    auto t0 = std::chrono::steady_clock::now();
    std::thread cons([&] {
        int v;
        long acc = 0;
        while (buf.get(v)) { acc += v; ++consumed; }
        std::cout << std::format("  消费者收到 {} 个，累加 = {}\n", consumed.load(), acc);
    });
    prod.join();
    cons.join();
    double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    std::cout << std::format("  耗时 {:.1f} ms；生产者产出 {}，消费者消费 {}\n",
                             ms, produced.load(), consumed.load());
    std::cout << "  没有忙等（while 里空转），阻塞时线程被操作系统挂起，CPU 占用接近 0\n";
}

/* ---------------- 4. 数据竞争：写错一次就少算几百万 ---------------- */
static void demo_atomic(ThreadPool &pool)
{
    sec("4. 同一个累加，普通 int 与 std::atomic 的差距");
    const int K = 8, EACH = 500000;

    long long naive = 0;                         /* 被 8 个线程同时 ++，典型未定义行为 */
    std::atomic<long long> safe{0};
    std::vector<std::future<void>> futs;
    for (int i = 0; i < K; i++) {
        futs.push_back(pool.submit([&naive, &safe] {
            for (int j = 0; j < EACH; j++) { ++naive; safe.fetch_add(1, std::memory_order_relaxed); }
        }));
    }
    for (auto &f : futs) f.get();
    std::cout << std::format("  理论值      = {}\n", (long long)K * EACH);
    std::cout << std::format("  普通 longlong = {}   {} \n", naive,
                             naive == (long long)K * EACH ? "（这次侥幸，多跑几遍会露馅）" : "<-- 少了，被线程互相覆盖");
    std::cout << std::format("  atomic      = {}   永远正确\n", safe.load());
}

int main()
{
    std::cout << "上一份 c/05 里，线程是操作系统的概念；这一份里，线程是「对象 + 模板」的概念。\n";
    {
        ThreadPool pool(4);
        demo_map_reduce(pool);
        demo_exception(pool);
        demo_atomic(pool);
    }                                  /* pool 在这里被析构：自动 notify_all + join */
    demo_producer_consumer();
    std::cout << "\n注意：线程池的析构函数就是「优雅关闭」本身，不需要调用方记得做任何事。\n";
    return 0;
}

