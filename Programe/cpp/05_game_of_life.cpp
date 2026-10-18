/* ==========================================================================
 * 05_game_of_life.cpp —— 在终端里做「彩色动画 + 模拟」
 *
 * 终端其实是块可以逐像素（逐字符）写的画布。这个程序用 ANSI 转义序列做到：
 *   - 隐藏光标 / 光标归位 / 局部重绘 -> 不刷屏、不闪烁（双缓冲：只画变化的格子）
 *   - 24 位真彩色（\x1b[38;2;r;g;bm）
 *   - 每一代用不同颜色表示「年龄」，看起来就是流动的图
 *   - 键盘中断退出（PeekConsoleInput）
 *
 * 生命规则（康威 1970）：数 8 个邻居
 *   活细胞 2 或 3 个邻居 -> 活下去；否则死亡
 *   死细胞 正好 3 个邻居 -> 诞生
 * 就这一条规则，能长出滑翔机、枪、甚至图灵完备的电路。
 *
 * 编译：g++ -std=c++20 -O2 -Wall -Wextra cpp\05_game_of_life.cpp -o out\cpp05.exe
 * 运行：out\cpp05.exe         （任意键退出；建议用 Windows Terminal，颜色更准）
 *      ：out\cpp05.exe 80 30 200  （可选参数：宽 高 代数）
 * ========================================================================== */
#include <windows.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

static bool enable_ansi()
{
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (!GetConsoleMode(h, &mode)) return false;
    /* Windows 10 起控制台支持 VT100 转义序列，但要显式打开这个开关 */
    return SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
}

static void any_key_pressed()
{
    INPUT_RECORD rec;
    DWORD n = 0;
    HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
    while (PeekConsoleInput(in, &rec, 1, &n) && n > 0) {
        ReadConsoleInput(in, &rec, 1, &n);
        if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown) {
            if (rec.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE) exit(0);
        }
    }
}

/* ---------------- 棋盘：双缓冲 + 环面（上下左右相连） ---------------- */
class Board {
public:
    Board(int w, int h, unsigned seed) : w_(w), h_(h), cur_(w * h, 0), nxt_(w * h, 0), age_(w * h, 0)
    {
        std::mt19937 rng(seed);
        std::uniform_int_distribution<int> d(0, 99);
        for (int i = 0; i < w * h; i++) cur_[i] = (d(rng) < 22) ? 1 : 0;   /* 22% 初始密度 */
        add_glider(w / 4, h / 4);
        add_glider(w * 3 / 4, h / 4);
        add_glider(w / 4, h * 3 / 4);
    }

    int w() const { return w_; }
    int h() const { return h_; }

    int alive_at(int x, int y) const { return cur_[y * w_ + x]; }
    int age_at(int x, int y)   const { return age_[y * w_ + x]; }

    /* 优化点：不去给每个格子数邻居（O(9N)），而是让每个活细胞向 8 个邻居「投票」。
       场面稀疏时（大部分格子是死的）这一步快得多。 */
    void step()
    {
        std::vector<int> votes(w_ * h_, 0);
        for (int y = 0; y < h_; y++) {
            for (int x = 0; x < w_; x++) {
                if (!cur_[y * w_ + x]) continue;
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        int ny = (y + dy + h_) % h_;      /* 环面：越界绕回对面 */
                        int nx = (x + dx + w_) % w_;
                        votes[ny * w_ + nx]++;
                    }
                }
            }
        }
        born_ = died_ = 0;
        for (int i = 0; i < w_ * h_; i++) {
            const bool live = cur_[i] != 0;
            const int  v    = votes[i];
            const bool next = live ? (v == 2 || v == 3) : (v == 3);
            nxt_[i] = next ? 1 : 0;
            if (next && !live) { ++born_; age_[i] = 1; }
            else if (next)     { age_[i] = std::min(age_[i] + 1, 255); }
            else if (live)     { ++died_; age_[i] = 0; }
        }
        std::swap(cur_, nxt_);                            /* 双缓冲交换，无拷贝 */
        ++gen_;
    }

    int  gen()  const { return gen_; }
    long alive() const { long s = 0; for (int v : cur_) s += v; return s; }
    long births() const { return born_; }
    long deaths() const { return died_; }
    void add_glider(int x, int y)
    {
        static const int cells[3][2] = {{1, 0}, {2, 1}, {0, 2}};   /* 经典滑翔机 */
        for (auto &c : cells) {
            int nx = (x + c[0] + w_) % w_, ny = (y + c[1] + h_) % h_;
            cur_[ny * w_ + nx] = 1;
            age_[ny * w_ + nx] = 1;
        }
    }
private:
    int w_, h_, gen_ = 0;
    std::vector<int> cur_, nxt_, age_;
    long born_ = 0, died_ = 0;
};
/* ---------------- 渲染：差分刷新，只改变化的格子 ---------------- */
static const char kBlock[] = "\xE2\x96\x88\xE2\x96\x88";   /* ██ 两个 U+2588，宽高比正好接近方形 */

/* 按年龄给颜色：新生蓝 -> 中年绿 -> 老细胞橙红，一眼能看出「哪里活得久」 */
static std::string color_for(int age)
{
    int r, g, b;
    if (age <= 1)      { r = 60;  g = 120; b = 255; }
    else if (age <= 4) { r = 60;  g = 220; b = 180; }
    else if (age <= 9) { r = 200; g = 230; b = 70;  }
    else if (age <= 20){ r = 255; g = 170; b = 40;  }
    else               { r = 255; g = 60;  b = 60;  }
    return std::format("\x1b[38;2;{};{};{}m", r, g, b);
}

static void render(const Board &b, std::vector<int> &prev_age)
{
    static const std::string reset = "\x1b[0m";
    std::string out;
    out.reserve((size_t)b.w() * b.h() * 12);
    bool need_home = true;
    int lastx = -2, lasty = -2;

    for (int y = 0; y < b.h(); y++) {
        for (int x = 0; x < b.w(); x++) {
            const int i   = y * b.w() + x;
            const int age = b.alive_at(x, y) ? b.age_at(x, y) : 0;
            if (prev_age[i] == age) continue;                 /* 没变 -> 一个字节都不输出 */
            if (need_home) { out += "\x1b[H"; need_home = false; }
            if (y != lasty || x != lastx + 1)                 /* 不连续 -> 先跳光标 */
                out += std::format("\x1b[{};{}H", y + 1, x * 2 + 1);
            if (age) out += color_for(age) + kBlock + reset;
            else     out += "\x1b[38;2;18;18;26m  \x1b[0m";    /* 空格当「暗色背景」 */
            prev_age[i] = age;
            lastx = x;
            lasty = y;
        }
    }
    std::fwrite(out.data(), 1, out.size(), stdout);
    std::fflush(stdout);
}

/* 把种群数量画成一张迷你折线图（控制台里最便宜的「图表」手段） */
static void sparkline(const std::vector<long> &hist, int rows = 6)
{
    if (hist.empty()) return;
    long mx = *std::max_element(hist.begin(), hist.end());
    if (mx <= 0) mx = 1;
    std::vector<std::string> lines(rows, std::string(hist.size(), ' '));
    for (size_t i = 0; i < hist.size(); i++) {
        int v = (int)(hist[i] * (rows - 1) / mx);
        for (int r = 0; r <= v; r++) lines[rows - 1 - r][i] = r == v ? '\xB0' : '#';
    }
    std::cout << "\n  种群数量变化（峰值 " << mx << "）：\n";
    for (auto &l : lines) std::cout << "    " << l << "\n";
}

int main(int argc, char **argv)
{
    const int W    = argc > 1 ? std::atoi(argv[1]) : 60;
    const int H    = argc > 2 ? std::atoi(argv[2]) : 24;
    const int gens = argc > 3 ? std::atoi(argv[3]) : 400;

    SetConsoleOutputCP(65001);                     /* 让 █ 正常显示 */
    if (!enable_ansi()) {
        std::cout << "这个终端没有开启 ANSI 支持（试试 Windows Terminal 或 cmd 里 chcp 65001）\n";
        return 1;
    }
    HANDLE hin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hin, &mode);
    SetConsoleMode(hin, mode & ~(ENABLE_LINE_INPUT | ENABLE_QUICK_EDIT_MODE));

    std::fputs("\x1b[2J\x1b[H\x1b[?25l", stdout);  /* 清屏 + 归位 + 隐藏光标 */

    Board b(W, H, 20261017u);
    std::vector<int> prev_age((size_t)W * H, -1);  /* -1 保证第一帧全画 */
    std::vector<long> hist;
    hist.reserve(gens);

    auto t_start = std::chrono::steady_clock::now();
    const auto frame_pause = std::chrono::milliseconds(28);   /* ~35 FPS */
    bool quit = false;

    for (int g = 0; g < gens && !quit; g++) {
        b.step();
        render(b, prev_age);
        hist.push_back(b.alive());
        if ((g % 3) == 0 || b.alive() == 0) {
            auto el = std::chrono::duration<double>(std::chrono::steady_clock::now() - t_start).count();
            std::fputs(std::format("\x1b[{};1H\x1b[38;2;255;255;255m 第 {} 代   存活 {:6}   "
                                   "新生 {:5}   死亡 {:5}   {:.1f}s   按任意键停止\x1b[0m",
                                   H + 1, b.gen(), b.alive(), b.births(), b.deaths(), el)
                           .c_str(),
                       stdout);
            std::fflush(stdout);
        }
        any_key_pressed();                                     /* 非阻塞：按了键就退出循环 */
        std::this_thread::sleep_for(frame_pause);
        if (b.alive() == 0) break;                             /* 全场死光，不用再演 */
    }

    std::fputs(std::format("\x1b[{};1H\x1b[0m", H + 3).c_str(), stdout);
    double total = std::chrono::duration<double>(std::chrono::steady_clock::now() - t_start).count();
    std::cout << std::format("  模拟结束：{} 代，{} 个格子，用时 {:.2f}s（约 {:.0f} FPS）\n",
                             b.gen(), W * H, total, b.gen() / (total > 0 ? total : 1));
    sparkline(hist);
    std::fputs("\x1b[?25h\x1b[0m", stdout);        /* 一定要把光标还回来 */
    return 0;
}

