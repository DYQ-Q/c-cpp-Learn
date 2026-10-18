/* ==========================================================================
 * 01_modern_stl.cpp —— 现代 C++（C++17/20）写数据处理有多爽
 *
 * 对照实验：c/07_data_structures.c 用 200 行 C 做的词频统计，
 *          这个文件最后用 10 行 C++ 做完同一件事。
 *
 * 演示能力：
 *   1. 容器选型：vector / map / unordered_map / set / array / span
 *   2. 算法库：sort / transform / copy_if / count_if / accumulate /
 *      partial_sort / unique / lower_bound / max_element —— 全都可组合
 *   3. lambda（含捕获）、函数对象、std::function
 *   4. string_view 零拷贝切字符串、optional、variant + std::visit
 *   5. C++20：ranges 管道、std::format
 *   6. 结构化绑定
 *
 * 编译：g++ -std=c++20 -Wall -Wextra cpp\01_modern_stl.cpp -o out\cpp01.exe
 * 运行：out\cpp01.exe
 * ========================================================================== */
#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <format>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <numeric>
#include <optional>
#include <ranges>
#include <set>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

namespace ranges = std::ranges;

struct Player {
    std::string name;
    int         score;
    double      hp;
};

static void sec(std::string_view t) { std::cout << "\n===== " << t << " =====\n"; }

/* GCC 的 <format> 里没有 fmt::join（那是 {fmt} 库的扩展），这里自己补一个，
   这样容器可以直接用 "a, b, c" 的形式打印出来。 */
namespace fmt {
template <class R>
std::string join(R &&r, std::string_view sep)
{
    std::ostringstream os;
    bool first = true;
    for (const auto &x : r) {
        if (!first) os << sep;
        first = false;
        os << x;
    }
    return os.str();
}
} // namespace fmt

/* ---------------- 1. 容器：各有各的用途 ---------------- */
static void demo_containers()
{
    sec("1. 容器选型");
    std::vector<Player> players = {
        {"Aria", 87, 92.5}, {"Bram", 42, 30.0}, {"Cleo", 99, 78.0},
        {"Dane", 63, 61.0}, {"Elin", 42, 88.0}, {"Faye", 91, 12.5},
    };
    std::cout << std::format("vector：{} 个玩家，按下标随机访问 O(1)\n", players.size());

    /* map：红黑树，key 有序，O(log n) */
    std::map<int, std::vector<std::string>> by_score;
    for (const auto &p : players) by_score[p.score].push_back(p.name);
    std::cout << "map（按分数分组，key 自动升序）：\n";
    for (const auto &[score, names] : by_score)            /* 结构化绑定 */
        std::cout << std::format("  {} 分 -> {}\n", score, fmt::join(names, ", "));

    /* unordered_map：哈希表，O(1)，无序 */
    std::unordered_map<std::string, int> idx;
    for (size_t i = 0; i < players.size(); i++) idx[players[i].name] = (int)i;
    std::cout << std::format("unordered_map：查 \"Cleo\" 的下标 = {}（平均 O(1)）\n", idx["Cleo"]);

    /* set：自动去重 + 有序 */
    std::set<int> scores;
    for (const auto &p : players) scores.insert(p.score);
    std::cout << std::format("set：{} 个不同分数（原 {} 人）\n", scores.size(), players.size());

    /* std::array：栈上定长，比 C 数组安全（知道自身大小，越界 .at() 会抛） */
    std::array<double, 3> pos{1.0, 2.0, 3.0};
    std::cout << std::format("std::array：size={}，.at(1)={}\n", pos.size(), pos.at(1));
}

/* ---------------- 2. 算法：不写循环，写「做什么」 ---------------- */
static void demo_algorithms()
{
    sec("2. <algorithm> 全家桶");
    std::vector<Player> ps = {
        {"Aria", 87, 92.5}, {"Bram", 42, 30.0}, {"Cleo", 99, 78.0},
        {"Dane", 63, 61.0}, {"Elin", 42, 88.0}, {"Faye", 91, 12.5},
    };

    /* 排序：三种写法 */
    std::sort(ps.begin(), ps.end(), [](const Player &a, const Player &b) {
        return a.score > b.score;                       /* 降序 */
    });
    std::cout << "按分数降序：" << fmt::join(ps | std::views::transform(
        [](const Player &p) { return p.name + "(" + std::to_string(p.score) + ")"; }), " ") << "\n";

    /* 复合排序：stable_sort 保持同分者的原有顺序 */
    std::stable_sort(ps.begin(), ps.end(), [](const Player &a, const Player &b) {
        return a.score == b.score ? a.name < b.name : a.score < b.score;
    });

    /* 查找 / 统计 / 求和 */
    auto best = ranges::max_element(ps, std::ranges::less{}, &Player::score);
    std::cout << std::format("max_element：最高分 {}（{}）\n", best->score, best->name);

    int alive = (int)ranges::count_if(ps, [](const Player &p) { return p.hp > 50; });
    std::cout << std::format("count_if(hp>50) = {}\n", alive);

    double total_hp = std::accumulate(ps.begin(), ps.end(), 0.0,
                                      [](double acc, const Player &p) { return acc + p.hp; });
    std::cout << std::format("accumulate 血量总和 = {:.1f}，平均 {:.2f}\n", total_hp, total_hp / ps.size());

    /* transform + back_inserter：映射成另一个容器 */
    std::vector<std::string> tags;
    std::transform(ps.begin(), ps.end(), std::back_inserter(tags),
                   [](const Player &p) { return p.hp < 50 ? std::string(p.name + "=危险")
                                                          : std::string(p.name + "=安全"); });
    std::cout << "transform：" << fmt::join(tags, ", ") << "\n";

    /* copy_if：过滤 */
    std::vector<Player> rich;
    std::copy_if(ps.begin(), ps.end(), std::back_inserter(rich),
                 [](const Player &p) { return p.score >= 80; });
    std::cout << std::format("copy_if(score>=80) 得到 {} 人\n", rich.size());

    /* partition：把「死掉的」挪到尾部（原地，O(n)，不额外分配） */
    auto mid = std::partition(ps.begin(), ps.end(), [](const Player &p) { return p.hp >= 50; });
    std::cout << std::format("partition：前 {} 个存活，后 {} 个濒危\n",
                             (size_t)std::distance(ps.begin(), mid),
                             (size_t)std::distance(mid, ps.end()));

    /* unique + erase：排序后去重，C++ 最有名的「惯用搭配」 */
    std::vector<int> nums{5, 1, 4, 1, 2, 5, 3};
    ranges::sort(nums);
    nums.erase(std::unique(nums.begin(), nums.end()), nums.end());
    std::cout << "sort + unique + erase 去重：" << fmt::join(nums, " ") << "\n";

    /* 二分查找：数组必须先有序 */
    auto it = ranges::lower_bound(nums, 3);
    std::cout << std::format("lower_bound(3) -> 下标 {}，值 {}\n",
                             (size_t)std::distance(nums.begin(), it), *it);

    /* partial_sort：只要前 K 名，比全排序便宜（Top-K 的正规做法） */
    std::vector<int> big;
    for (int i = 0; i < 20000; i++) big.push_back((i * 1103515245 + 12345) % 100000);
    std::partial_sort(big.begin(), big.begin() + 5, big.end(), std::greater<int>{});
    std::cout << "partial_sort 前 5 名：" << fmt::join(big | std::views::take(5), " ") << "\n";
}
/* ---------------- 3. string_view：切字符串但不复制 ---------------- */
static std::vector<std::string_view> split(std::string_view s, char delim)
{
    std::vector<std::string_view> out;
    size_t pos = 0;
    for (;;) {
        size_t next = s.find(delim, pos);
        if (next == std::string_view::npos) { out.push_back(s.substr(pos)); break; }
        out.push_back(s.substr(pos, next - pos));
        pos = next + 1;
    }
    return out;
}

static int parse_int(std::string_view s)         /* from_chars：不分配、不看 locale */
{
    int v = 0;
    std::from_chars(s.data(), s.data() + s.size(), v);
    return v;
}

static void demo_string_view()
{
    sec("3. string_view：整个解析过程零次字符串分配");
    std::string line = "name=Aria;score=87;hp=92.5;tag=tank,leader";
    for (auto kv : split(line, ';')) {
        size_t eq = kv.find('=');
        std::string_view key = kv.substr(0, eq);
        std::string_view val = kv.substr(eq + 1);
        std::cout << std::format("  {:<6} -> {}\n", key, val);
        if (key == "tag")
            for (auto t : split(val, ',')) std::cout << std::format("       tag: {}\n", t);
        if (key == "score")
            std::cout << std::format("       from_chars -> {} (int)\n", parse_int(val));
    }
    std::cout << std::format("  format 排版：|{:>10}|{:<10}|{:^10}|\n", "right", "left", "center");
}

/* ---------------- 4. optional：把「可能没有」写进类型 ---------------- */
static std::optional<Player> find_by_name(const std::vector<Player> &ps, std::string_view name)
{
    auto it = ranges::find_if(ps, [&](const Player &p) { return p.name == name; });
    if (it == ps.end()) return std::nullopt;        /* 明确的「找不到」，不是 -1 也不是 NULL */
    return *it;
}

static void demo_optional()
{
    sec("4. optional：找不到就是找不到，不用约定 0/-1/NULL");
    std::vector<Player> ps = {{"Aria", 87, 92.5}, {"Bram", 42, 30.0}};

    if (auto p = find_by_name(ps, "Aria"))
        std::cout << std::format("  找到 {}，分数 {}\n", p->name, p->score);
    else
        std::cout << "  没找到\n";

    auto missing = find_by_name(ps, "Nobody");
    std::cout << std::format("  has_value() = {}，value_or(0) = {}\n",
                             missing.has_value(), missing.value_or(Player{"-", 0, 0}).score);
    /* 直接 *missing 是未定义行为 —— optional 的价值就是逼你处理空值 */
}

/* ---------------- 5. variant：类型安全的 union ---------------- */
static void demo_variant()
{
    sec("5. variant：一个值只能是这几种类型之一");
    using Config = std::variant<int, double, std::string, bool>;
    std::vector<Config> cfg{87, 92.5, std::string("Aria"), true};

    for (const auto &c : cfg) {
        std::visit([](const auto &v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, int>)         std::cout << "int     : ";
            else if constexpr (std::is_same_v<T, double>) std::cout << "double  : ";
            else if constexpr (std::is_same_v<T, bool>)   std::cout << "bool    : ";
            else                                          std::cout << "string  : ";
            std::cout << v;
        }, c);
        std::cout << std::format("   (index={})\n", c.index());
    }

    try {                                     /* 取错类型会抛异常，而不是给你一堆垃圾比特 */
        std::cout << std::get<double>(cfg[0]) << "\n";
    } catch (const std::bad_variant_access &e) {
        std::cout << std::format("  捕获异常：{}\n", e.what());
    }
    std::cout << std::format("  holds_alternative<int>(cfg[0]) = {}\n",
                             std::holds_alternative<int>(cfg[0]));
}
/* ---------------- 6. span：接收「一段连续内存」而不关心它来自哪 ---------------- */
static double mean(std::span<const double> xs)
{
    return xs.empty() ? 0.0 : std::accumulate(xs.begin(), xs.end(), 0.0) / (double)xs.size();
}

static void demo_span()
{
    sec("6. span：同一个函数，vector / array / C 数组都能喂");
    std::vector<double>   v{1.0, 2.0, 3.0, 4.0};
    std::array<double, 3> a{10.0, 20.0, 30.0};
    double                raw[5] = {1, 1, 2, 3, 5};
    std::cout << std::format("  vector -> {:.2f}   array -> {:.2f}   C 数组 -> {:.2f}\n",
                             mean(v), mean(a), mean(raw));
    std::cout << "  C 语言版得写成 (const double *p, size_t n) —— span 把这个约定自动化了\n";
}

/* ---------------- 7. ranges 管道：一句话串起多个操作 ---------------- */
static void demo_ranges()
{
    sec("7. C++20 ranges：惰性求值，没有中间容器");
    std::vector<Player> ps = {
        {"Aria", 87, 92.5}, {"Bram", 42, 30.0}, {"Cleo", 99, 78.0},
        {"Dane", 63, 61.0}, {"Elin", 42, 88.0}, {"Faye", 91, 12.5},
    };

    auto top_alive = ps
        | std::views::filter([](const Player &p) { return p.hp > 50; })
        | std::views::transform([](const Player &p) { return p.name; })
        | std::views::take(3);
    std::cout << "  存活的前 3 人：" << fmt::join(top_alive, ", ") << "\n";

    auto odd_squares = std::views::iota(1, 20)
        | std::views::filter([](int n) { return n % 2 == 1; })
        | std::views::transform([](int n) { return n * n; });
    std::cout << "  1..19 里奇数的平方：" << fmt::join(odd_squares, " ") << "\n";
    std::cout << "  （iota 造出一个「虚拟数组」，只有真正取值时才计算，不占内存）\n";

    std::vector<int> nums{4, 1, 3, 2};
    std::cout << "  reverse 视图：" << fmt::join(nums | std::views::reverse, " ") << "\n";
}

/* ---------------- 8. 与 c/07_data_structures.c 对照 ---------------- */
static void demo_wordfreq_vs_c()
{
    sec("8. 同一件事：C 版约 200 行 vs C++ 版 10 行");
    std::string text =
        "the quick brown fox jumps over the lazy dog\n"
        "the dog barks and the fox runs and the fox runs again\n"
        "a C program manages its own memory and a C program can crash the world\n";

    std::unordered_map<std::string, int> freq;
    std::istringstream in(text);
    for (std::string w; in >> w;) {
        std::string clean;
        for (unsigned char c : w) if (std::isalnum(c)) clean += (char)std::tolower(c);
        if (!clean.empty()) freq[clean]++;          /* 不存在的 key 自动值初始化成 0 */
    }

    std::vector<std::pair<std::string, int>> top(freq.begin(), freq.end());
    std::partial_sort(top.begin(), top.begin() + 5, top.end(),
                      [](const auto &a, const auto &b) {
                          return a.second != b.second ? a.second > b.second : a.first < b.first;
                      });
    std::cout << "  Top-5:";
    for (size_t i = 0; i < top.size() && i < 5; i++)
        std::cout << std::format(" {}({})", top[i].first, top[i].second);
    std::cout << std::format("\n  不同词数 = {}（哈希、扩容、内存全部由库负责）\n", freq.size());
}

int main()
{
    std::cout << "C++ 的立场：既要 C 的零开销，又不想让你手动 malloc/free。\n";
    std::cout << "手段：模板 + 值语义 + RAII + 标准库。\n";
    demo_containers();
    demo_algorithms();
    demo_string_view();
    demo_optional();
    demo_variant();
    demo_span();
    demo_ranges();
    demo_wordfreq_vs_c();
    std::cout << "\n全部演示完成。\n";
    return 0;
}


