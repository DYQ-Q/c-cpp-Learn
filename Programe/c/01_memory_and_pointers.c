/* ==========================================================================
 * 01_memory_and_pointers.c —— C 的核心竞争力：亲手管理内存
 *
 * 演示能力：
 *   1. 值 / 地址 / 指针算术 / 数组退化
 *   2. malloc、calloc、realloc、free —— 手写一个会自己扩容的动态数组
 *   3. 结构体内存布局（对齐、padding）、union、用 memcpy 做类型双关
 *   4. 单向链表的建立 / 遍历 / 排序 / 完整释放（不留内存泄漏）
 *   5. 函数指针 + 回调 + void* 上下文 —— C 语言版的「接口 / 闭包」
 *   6. C 里最常见的未定义行为坑
 *
 * 编译：gcc -std=c17 -Wall -Wextra -g c\01_memory_and_pointers.c -o out\01.exe
 * 运行：out\01.exe
 * ========================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static void hr(const char *t) { printf("\n===== %s =====\n", t); }
static void die(const char *m) { perror(m); exit(EXIT_FAILURE); }

/* ---------- 1. 值、地址、指针算术 ---------- */
static void demo_pointer(void)
{
    hr("1. 值 / 地址 / 指针算术");
    int  a = 10;
    int  arr[5] = {11, 12, 13, 14, 15};
    int *p = &a;

    printf("a = %d, &a = %p, *p = %d\n", a, (void *)&a, *p);
    printf("指针变量本身占 %zu 字节\n", sizeof(void *));
    printf("sizeof(arr)=%zu  sizeof(arr[0])=%zu  元素个数=%zu\n",
           sizeof arr, sizeof arr[0], sizeof arr / sizeof arr[0]);

    int *q = arr + 2;                    /* 指针 + 整数 = 前进「元素」而不是字节 */
    printf("arr[2]=%d  *(arr+2)=%d  q[-1]=%d  q-arr=%td 个元素\n",
           arr[2], *q, q[-1], (ptrdiff_t)(q - arr));
    printf("元素地址连续：");
    for (int i = 0; i < 5; i++) printf(" %p", (void *)&arr[i]);
    printf("\n");

    /* 数组当参数会退化成指针，sizeof 立刻失效，所以必须「指针 + 长度」成对传递 */
    long long s = 0;
    for (size_t i = 0; i < sizeof arr / sizeof arr[0]; i++) s += arr[i];
    printf("数组和 = %lld\n", s);
}

/* ---------- 2. 手动动态数组（这就是 std::vector 的底层） ---------- */
typedef struct { int *data; size_t len; size_t cap; } IntVec;

static void iv_init(IntVec *v)  { v->data = NULL; v->len = 0; v->cap = 0; }
static void iv_free(IntVec *v)  { free(v->data); iv_init(v); }

static void iv_push(IntVec *v, int x)
{
    if (v->len == v->cap) {
        size_t old = v->cap;
        size_t nc  = v->cap ? v->cap * 2 : 4;
        /* 先收到临时指针：万一 realloc 失败，原来的 v->data 还在，不会泄漏 */
        int *tmp = realloc(v->data, nc * sizeof *tmp);
        if (!tmp) die("realloc");
        v->data = tmp;
        v->cap  = nc;
        printf("  [扩容] capacity %zu -> %zu（一次分配，摊销 O(1)）\n", old, nc);
    }
    v->data[v->len++] = x;
}

static void demo_heap(void)
{
    hr("2. 堆上内存：malloc / calloc / realloc / free");
    int *one = malloc(3 * sizeof *one);           /* 内容未初始化 */
    if (!one) die("malloc");
    one[0] = 7; one[1] = 8; one[2] = 9;
    printf("malloc 3 个 int: %d %d %d\n", one[0], one[1], one[2]);

    int *zeroed = calloc(5, sizeof *zeroed);      /* 全 0 */
    if (!zeroed) die("calloc");
    printf("calloc 5 个 int 已经全是 0：");
    for (int i = 0; i < 5; i++) printf(" %d", zeroed[i]);
    printf("\n");

    /* 二维数组：真实内存仍然是一维的，靠 "行 * 列数 + 列" 手工映射 */
    const int R = 3, C = 4;
    int *grid = malloc((size_t)R * C * sizeof *grid);
    if (!grid) die("malloc grid");
    for (int r = 0; r < R; r++)
        for (int c = 0; c < C; c++)
            grid[r * C + c] = r * 10 + c;         /* 行优先 */
    printf("手动映射的 2D：");
    for (int i = 0; i < R * C; i++) printf(" %d", grid[i]);
    printf("\n");

    IntVec v; iv_init(&v);
    for (int i = 1; i <= 10; i++) iv_push(&v, i * i);
    printf("动态数组内容（平方数）：");
    for (size_t i = 0; i < v.len; i++) printf(" %d", v.data[i]);
    printf("\n");

    free(one); free(zeroed); free(grid); iv_free(&v);
    printf("全部 free 完成 —— 忘记这一步就是内存泄漏（C 没有垃圾回收）\n");
}

/* ---------- 3. 结构体内存布局 ---------- */
struct Bad  { char a; int b; char c; };   /* 为了对齐，中间塞了填充字节 */
struct Good { int b; char a; char c; };   /* 重新排列字段，省 4 字节 */

static void demo_layout(void)
{
    hr("3. 内存布局：对齐 / padding / union / 类型双关");
    printf("struct Bad  = %zu 字节 (a@%zu b@%zu c@%zu)\n",
           sizeof(struct Bad), offsetof(struct Bad, a),
           offsetof(struct Bad, b), offsetof(struct Bad, c));
    printf("struct Good = %zu 字节 —— 同样的字段，换个顺序就小了\n", sizeof(struct Good));

    union { uint32_t u; uint8_t b[4]; } e = { .u = 0x0A0B0C0Du };
    printf("本机字节序：%s\n", e.b[0] == 0x0D ? "小端 (little-endian)" : "大端 (big-endian)");

    float f = 0.1f;
    uint32_t bits;
    memcpy(&bits, &f, sizeof bits);          /* 安全的类型双关（不像强转指针） */
    printf("0.1f 的原始位 = 0x%08X\n", bits);
    printf("0.1 + 0.2 == 0.3 ? -> %s（浮点只有二进制位，无法精确表示 0.1）\n",
           (0.1 + 0.2 == 0.3) ? "true" : "false");
}
/* ---------- 4. 链表 + qsort ---------- */
typedef struct Node {
    int          score;
    char         name[16];
    struct Node *next;
} Node;

static Node *list_push(Node *head, const char *name, int score)
{
    Node *n = malloc(sizeof *n);
    if (!n) die("malloc node");
    snprintf(n->name, sizeof n->name, "%s", name);   /* 永远用 snprintf，别用 strcpy */
    n->score = score;
    n->next  = head;                                   /* 头插法 */
    return n;
}

static int cmp_desc(const void *x, const void *y)
{
    const Node *a = *(const Node *const *)x;
    const Node *b = *(const Node *const *)y;
    return (b->score > a->score) - (b->score < a->score);  /* 不用 a-b，避免溢出 */
}

static void demo_list(void)
{
    hr("4. 链表：自己控制每一个节点的生死");
    Node *head = NULL;
    head = list_push(head, "Tom",   72);
    head = list_push(head, "Alice", 95);
    head = list_push(head, "Bob",   83);
    head = list_push(head, "Zoe",  100);

    printf("链表插入顺序（头插法所以是反的）：");
    for (Node *p = head; p; p = p->next) printf(" %s(%d)", p->name, p->score);
    printf("\n");

    size_t n = 0;
    for (Node *p = head; p; p = p->next) n++;
    Node **arr = malloc(n * sizeof *arr);
    if (!arr) die("malloc arr");
    size_t i = 0;
    for (Node *p = head; p; p = p->next) arr[i++] = p;   /* 只排指针，不动节点 */

    qsort(arr, n, sizeof arr[0], cmp_desc);
    printf("qsort 按分数降序：");
    for (size_t k = 0; k < n; k++) printf(" %s(%d)", arr[k]->name, arr[k]->score);
    printf("\n");

    while (head) { Node *nx = head->next; free(head); head = nx; }
    free(arr);
    printf("链表节点与指针数组都已释放\n");
}

/* ---------- 5. 函数指针 / 回调 ---------- */
typedef void (*EachFn)(int value, void *ctx);       /* void* 是 C 的「泛型参数」 */

static void accumulate_cb(int v, void *ctx) { long long *s = ctx; *s += v; }
static void print_even_cb(int v, void *ctx) { (void)ctx; if (v % 2 == 0) printf(" %d", v); }

static void vec_each(const IntVec *v, EachFn fn, void *ctx)
{
    for (size_t i = 0; i < v->len; i++) fn(v->data[i], ctx);
}

static int op_add(int a, int b) { return a + b; }
static int op_sub(int a, int b) { return a - b; }
static int op_mul(int a, int b) { return a * b; }

static void demo_callbacks(void)
{
    hr("5. 函数指针、回调、跳转表");
    IntVec v; iv_init(&v);
    for (int i = 1; i <= 10; i++) iv_push(&v, i);

    long long sum = 0;
    vec_each(&v, accumulate_cb, &sum);
    printf("用回调求和 = %lld\n", sum);

    printf("用另一个回调只打印偶数：");
    vec_each(&v, print_even_cb, NULL);
    printf("\n");

    int (*const ops[])(int, int) = { op_add, op_sub, op_mul };   /* 跳转表 */
    const char *names[] = { "add", "sub", "mul" };
    for (size_t k = 0; k < sizeof ops / sizeof ops[0]; k++)
        printf("  %s(20, 6) = %d\n", names[k], ops[k](20, 6));

    iv_free(&v);
}

/* ---------- 6. 典型踩坑（都在注释里，真跑了是未定义行为） ---------- */
static void demo_pitfalls(void)
{
    hr("6. 五个经典坑（记住比会写代码更重要）");
    printf("a) 返回局部变量的地址：int *f(void){ int x=1; return &x; }  // x 出栈就没了\n");
    printf("b) 越界写：int a[3]; a[3]=0;  // 不报错，但它踩的是隔壁变量或 canary\n");
    printf("c) 忘记 free：循环里 malloc 而不 free —— 长跑程序内存一直涨\n");
    printf("d) double free / use-after-free：free 后指针仍是非 NULL，最好 free(p), p=NULL;\n");
    printf("e) realloc 直接赋值：p = realloc(p, n); 失败时旧 p 丢失 -> 泄漏。用临时变量\n");

    int *p = malloc(sizeof *p);
    if (!p) die("malloc");
    *p = 42;
    free(p);
    p = NULL;                    /* 好习惯：释放后立刻置空 */
    printf("\n本程序 malloc/free 完全配对（可加 -fsanitize=address 重新编译来验证）\n");
}

int main(void)
{
    printf("C 语言 = 你直接面对内存、字节和操作系统调用；\n");
    printf("代价是要自己负责，回报是没有任何隐藏开销。\n");
    demo_pointer();
    demo_heap();
    demo_layout();
    demo_list();
    demo_callbacks();
    demo_pitfalls();
    hr("完成");
    return 0;
}

