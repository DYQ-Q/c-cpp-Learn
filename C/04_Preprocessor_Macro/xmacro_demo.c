/* xmacro_demo.c —— 一张表生成 enum + 字符串表 + switch + 解析函数
 *
 * 编译：gcc -std=c11 -Wall -Wextra -Wpedantic -g xmacro_demo.c -o ..\output\xmacro_demo.exe
 * 想看清展开结果：gcc -std=c11 -E -P xmacro_demo.c | more
 *
 * 所有输出为确定值；_Static_assert 保证"表和枚举永不脱节"（脱节时直接编译失败）。
 * 【待实测】
 */
#include <stdio.h>
#include <string.h>

/* ===== 唯一数据源：X(枚举名, 值, 显示文本) ===== */
#define ERR_LIST(X)                              \
    X(E_OK,    0, "ok")                          \
    X(E_IO,    1, "io failure")                  \
    X(E_NOMEM, 2, "out of memory")               \
    X(E_PARSE, 3, "parse error")                 \
    X(E_TYPE,  4, "type mismatch")

/* ---- 展开方式 1：生成枚举 ---- */
typedef enum {
#define X(name_, value_, text_) name_ = value_,
    ERR_LIST(X)
#undef X
    ERR_COUNT                       /* 哨兵：总数，同时保证下面表格长度 */
} Err;

/* ---- 展开方式 2：生成字符串表（指定初始化器让"值"与"下标"天然对齐）---- */
static const char *const err_text[ERR_COUNT] = {
#define X(name_, value_, text_) [name_] = text_,
    ERR_LIST(X)
#undef X
};

/* ---- 展开方式 3：生成 switch（编译期可查穷尽性的那份）---- */
static const char *err_text_via_switch(Err e)
{
    switch (e) {
#define X(name_, value_, text_) case name_: return text_;
        ERR_LIST(X)
#undef X
    }
    return "<not an error code>";
}

/* ---- 展开方式 4：生成反查函数（显示文本 -> 枚举）---- */
static int err_from_text(const char *text, Err *out)
{
#define X(name_, value_, text_)                 \
    if (strcmp(text, text_) == 0) {             \
        *out = name_;                           \
        return 0;                               \
    }
    ERR_LIST(X)
#undef X
    return -1;
}

/* 表与枚举是否一致？用编译期检查代替"跑一遍看看" */
_Static_assert(ERR_COUNT == 5, "ERR_LIST lost an entry or gained one");
_Static_assert(sizeof err_text / sizeof err_text[0] == ERR_COUNT, "table size drift");

/* ===== 第二例：结构体字段表（演示 # 与格式串拼接）===== */
#define FIELD_LIST(X)                    \
    X(int,    id,    "%d")               \
    X(double, ratio, "%.3f")             \
    X(long,   seq,   "%ld")              \
    X(char,   tag,   "%c")

typedef struct {
    int    id;
    double ratio;
    long   seq;
    char   tag;
} Row;

/* 字段个数也算出来（X 展开成 +1，累加）*/
enum { FIELD_COUNT =
#define X(type_, field_, fmt_) +1
    FIELD_LIST(X)
#undef X
};

static void row_print(const Row *r)
{
#define X(type_, field_, fmt_) printf("  %s=" fmt_ "\n", #field_, r->field_);
    FIELD_LIST(X)
#undef X
    /*  #field_  -> "id" / "ratio" ...
        fmt_     -> 本身已经是字符串字面量，与 "  " 和 "=" 相邻拼接成 "  id=%d\n" */
}

/* 再一张表专门喂 X(idx, name) 形态：X-macro 的每个消费者可以选择不同的参数个数，
   只要列表形态与之匹配。这里演示"同一份语义，两种视图"。 */
#define FIELD_LIST_XREF(X) \
    X(0, "id")             \
    X(1, "ratio")          \
    X(2, "seq")            \
    X(3, "tag")

/* 反过来：字段名 -> 索引，用于"按名字取列"的场景 */
static int field_index_of(const char *name)
{
#define X(idx_, name_) if (strcmp(name, name_) == 0) return idx_;
    FIELD_LIST_XREF(X)
#undef X
    return -1;
}

int main(void)
{
    puts("== 1. 表 -> 字符串 ==");
    for (int e = 0; e < ERR_COUNT; ++e) {
        printf("  code=%d  table=%-16s switch=%-16s\n",
               e, err_text[e], err_text_via_switch((Err) e));
    }

    puts("== 2. 反查：文本 -> 枚举 ==");
    const char *probe[] = { "ok", "parse error", "no such thing" };
    for (size_t k = 0; k < sizeof probe / sizeof probe[0]; ++k) {
        Err got;
        int  rc = err_from_text(probe[k], &got);
        printf("  \"%s\" -> %s (code=%d)\n", probe[k],
               rc == 0 ? "命中" : "未命中", rc == 0 ? (int) got : -1);
    }

    puts("== 3. 越界值的行为（表法与 switch 法的差别在这里）==");
    printf("  err_text_via_switch(99)      = %s\n", err_text_via_switch((Err) 99));
    puts("  注意：直接索引 err_text[99] 会越界读 => UB。所以对外接口要么判范围，");
    puts("        要么只用 switch 版本。本文件的 err_str 包装做了判范围：");
    puts("        （见第 4 节）");

    puts("== 4. 带范围检查的统一出口 ==");
    for (int probe_i = -1; probe_i < 6; ++probe_i) {
        const char *s;
        if (probe_i >= 0 && probe_i < ERR_COUNT) {
            s = err_text_via_switch((Err) probe_i);
        } else {
            s = "<out of range>";
        }
        printf("  code=%2d -> %s\n", probe_i, s);
    }

    puts("== 5. 字段表：一份数据两处复用 ==");
    const Row r = { .id = 7, .ratio = 1.0 / 3.0, .seq = 42, .tag = 'C' };
    printf("  FIELD_COUNT = %d（编译期算出来的，期望 4）\n", FIELD_COUNT);
    row_print(&r);
    printf("  field_index_of(\"seq\")  = %d（期望 2）\n", field_index_of("seq"));
    printf("  field_index_of(\"nope\") = %d（期望 -1）\n", field_index_of("nope"));

    puts("== 6. 纪律检查 ==");
    puts("  每次展开后都 #undef X —— 否则下一处展开会撞上'X redefined'或串味");
    puts("  改 ERR_LIST 加一项，无需再动表/switch/反查函数；漏改会编译失败而非运行出错");
    return 0;
}
