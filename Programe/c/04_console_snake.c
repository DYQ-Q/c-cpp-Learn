/* ==========================================================================
 * 04_console_snake.c —— 一个完整可玩的游戏，只用 Windows 控制台 API
 *
 * 演示能力：
 *   1. SetConsoleMode：关掉「行输入」和「快速编辑」，键盘不用按回车就能被读到
 *   2. PeekConsoleInput + ReadConsoleInput：非阻塞取键（游戏循环的关键）
 *   3. SetConsoleCursorPosition / FillConsoleOutputAttribute：光标定位、上色
 *   4. 双缓冲：整帧先画进内存字符数组，再一次性输出 -> 不闪烁
 *   5. 游戏循环、难度递增、暂停、死亡重开
 *
 * 编译：gcc -std=c17 -Wall -Wextra -O2 c\04_console_snake.c -o out\04.exe
 * 运行：out\04.exe   （方向键或 WASD；P 暂停；R 重开；Q 退出）
 * 说明：用了 windows.h，仅 Windows。
 * ========================================================================== */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FW 26
#define FH 15
#define MAXCELL (FW * FH)

typedef struct { int x, y; } Pt;
typedef struct {
    Pt   body[MAXCELL];       /* body[0] 是头 */
    int  len;
    int  dx, dy;              /* 本帧方向 */
    int  ndx, ndy;            /* 缓存的下一个方向，防止一帧内 180° 掉头 */
    Pt   food;
    int  score, best, delay;
    int  alive;
} Game;

static HANDLE g_out, g_in;

static void gotoxy(int x, int y)
{
    COORD c = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(g_out, c);
}

static void hide_cursor(void)
{
    CONSOLE_CURSOR_INFO ci;
    GetConsoleCursorInfo(g_out, &ci);
    ci.bVisible = FALSE;
    SetConsoleCursorInfo(g_out, &ci);
}

/* 默认控制台是 line-input（必须按回车）+ quick-edit（选中即挂起程序），
   做游戏必须先把这两位关掉 —— 这是很多人写控制台游戏第一个撞的墙。 */
static void setup_input(void)
{
    DWORD mode = 0;
    GetConsoleMode(g_in, &mode);
    mode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_QUICK_EDIT_MODE);
    mode |= ENABLE_EXTENDED_FLAGS;
    SetConsoleMode(g_in, mode);
}

/* 非阻塞取键：有键返回虚拟键码并写出字符，没有键返回 0 */
static DWORD read_key(char *ch)
{
    INPUT_RECORD rec;
    DWORD n = 0;
    *ch = 0;
    while (PeekConsoleInput(g_in, &rec, 1, &n) && n > 0) {
        ReadConsoleInput(g_in, &rec, 1, &n);
        if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown) {
            *ch  = (char)rec.Event.KeyEvent.uChar.AsciiChar;
            return rec.Event.KeyEvent.wVirtualKeyCode;
        }
    }
    return 0;
}

static void place_food(Game *g)
{
    for (;;) {
        g->food.x = rand() % FW;
        g->food.y = rand() % FH;
        int clash = 0;
        for (int i = 0; i < g->len; i++)
            if (g->body[i].x == g->food.x && g->body[i].y == g->food.y) { clash = 1; break; }
        if (!clash) return;
    }
}

static void game_reset(Game *g)
{
    memset(g, 0, sizeof *g);
    g->len   = 4;
    g->dx = g->ndx = 1;
    g->delay = 140;
    g->alive = 1;
    for (int i = 0; i < g->len; i++) { g->body[i].x = 5 - i; g->body[i].y = FH / 2; }
    place_food(g);
}

static void game_step(Game *g)
{
    g->dx = g->ndx; g->dy = g->ndy;
    Pt nh = { g->body[0].x + g->dx, g->body[0].y + g->dy };

    if (nh.x < 0 || nh.y < 0 || nh.x >= FW || nh.y >= FH) { g->alive = 0; return; }
    for (int i = 0; i < g->len - 1; i++)                 /* 尾巴那格会移走，可以不算 */
        if (g->body[i].x == nh.x && g->body[i].y == nh.y) { g->alive = 0; return; }

    for (int i = g->len - 1; i > 0; i--) g->body[i] = g->body[i - 1];
    g->body[0] = nh;

    if (nh.x == g->food.x && nh.y == g->food.y) {
        if (g->len < MAXCELL) g->len++;
        g->score += 10;
        if (g->delay > 55) g->delay -= 6;                /* 越吃越快 */
        place_food(g);
    }
}
/* ---------- 双缓冲：把整帧拼进一个字符串，再一次输出 ---------- */
static char frame_buf[(FH + 2) * (FW + 3) + 1];   /* +1 收尾 0 */
static char grid[FH + 2][FW + 2];

static void build_frame(const Game *g)
{
    for (int y = 0; y < FH + 2; y++)
        for (int x = 0; x < FW + 2; x++)
            grid[y][x] = (y == 0 || y == FH + 1 || x == 0 || x == FW + 1) ? '#' : ' ';
    for (int i = 1; i < g->len; i++) grid[g->body[i].y + 1][g->body[i].x + 1] = 'o';
    grid[g->body[0].y + 1][g->body[0].x + 1] = '@';
    grid[g->food.y + 1][g->food.x + 1] = '*';

    int k = 0;
    for (int y = 0; y < FH + 2; y++) {
        memcpy(frame_buf + k, grid[y], (size_t)(FW + 2));
        k += FW + 2;
        frame_buf[k++] = '\n';
    }
    frame_buf[k] = 0;
}

/* 整帧画完后，逐格上色（字符 -> 颜色 的映射表，其实就是最朴素的「渲染管线」） */
static WORD color_of(char c)
{
    switch (c) {
    case '@': return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY; /* 黄：头 */
    case 'o': return FOREGROUND_GREEN | FOREGROUND_INTENSITY;                  /* 绿：身 */
    case '*': return FOREGROUND_RED | FOREGROUND_INTENSITY;                    /* 红：食物 */
    case '#': return FOREGROUND_BLUE | FOREGROUND_INTENSITY;                   /* 蓝：墙 */
    default:  return 7;
    }
}

static void draw(const Game *g)
{
    build_frame(g);
    gotoxy(0, 0);
    fputs(frame_buf, stdout);
    printf("分数 %d   最高 %d   速度 %d   方向键/WASD 移动   P 暂停   R 重开   Q 退出",
           g->score, g->best, 200 - g->delay);
    if (!g->alive) printf("   \a撞了！");
    fflush(stdout);

    /* 上色：遍历帧缓冲区，同一颜色连续段用一次 FillConsoleOutputAttribute 搞定 */
    COORD pos;
    DWORD written;
    int cols = FW + 2;
    for (int y = 0; y < FH + 2; y++) {
        for (int x = 0; x < cols; x++) {
            char c = frame_buf[y * (cols + 1) + x];
            if (c == ' ') continue;
            pos.X = (SHORT)x; pos.Y = (SHORT)y;
            FillConsoleOutputAttribute(g_out, color_of(c), 1, pos, &written);
        }
    }
}

/* 暂停：只在这里 Sleep，随时检查按键 */
static void wait_unpaused(int *running)
{
    char c;
    for (;;) {
        DWORD k = read_key(&c);
        if (k) {
            if (c == 'p' || c == 'P') return;
            if (c == 'q' || c == 'Q') { *running = 0; return; }
        }
        Sleep(25);
    }
}

int main(void)
{
    g_out = GetStdHandle(STD_OUTPUT_HANDLE);
    g_in  = GetStdHandle(STD_INPUT_HANDLE);
    hide_cursor();
    setup_input();
    srand((unsigned)GetTickCount());

    Game g;
    game_reset(&g);
    int running = 1;

    while (running) {
        draw(&g);

        char ch;
        DWORD key = read_key(&ch);
        switch (key) {
        case 'W': case VK_UP:    if (g.dy !=  1) { g.ndx = 0; g.ndy = -1; } break;
        case 'S': case VK_DOWN:  if (g.dy != -1) { g.ndx = 0; g.ndy =  1; } break;
        case 'A': case VK_LEFT:  if (g.dx !=  1) { g.ndx = -1; g.ndy = 0; } break;
        case 'D': case VK_RIGHT: if (g.dx != -1) { g.ndx =  1; g.ndy = 0; } break;
        default: break;
        }
        if (ch == 'q' || ch == 'Q') break;
        if (ch == 'p' || ch == 'P') wait_unpaused(&running);
        if (ch == 'r' || ch == 'R') { if (g.score > g.best) g.best = g.score; game_reset(&g); continue; }

        if (!g.alive) {
            if (g.score > g.best) g.best = g.score;
            draw(&g);
            for (;;) {                                  /* 死亡界面：等 R 或 Q */
                char c2;
                DWORD k2 = read_key(&c2);
                if (k2) {
                    if (c2 == 'r' || c2 == 'R') { game_reset(&g); break; }
                    if (c2 == 'q' || c2 == 'Q') { running = 0; break; }
                }
                Sleep(20);
            }
            continue;
        }
        game_step(&g);
        Sleep((DWORD)g.delay);                           /* 帧率 = 难度 */
    }

    gotoxy(0, FH + 3);
    SetConsoleTextAttribute(g_out, 7);
    hide_cursor();
    printf("\n这个可玩的游戏里没有任何游戏引擎，只有 stdio + 几个 Win32 API。\n");
    printf("把它改造成「俄罗斯方块 / 迷宫 / 飞机大战」只需要换掉 build_frame 而已。\n");
    return 0;
}

