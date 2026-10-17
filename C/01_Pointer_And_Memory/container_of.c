/* container_of.c —— offsetof / 容器反推 / 侵入式双向链表最小实现
 *
 * 编译：
 *   gcc -std=c11 -Wall -Wextra -Wpedantic -g container_of.c -o ..\output\container_of.exe
 *
 * 输出全部是确定值（不打印地址本身，只打印偏移与指针相等判定的结果）。
 * assert 若失败会 abort 并打印文件名/行号 —— 这正是 指针与内存.md 第 6 节说的"前提校验"。
 * 【待实测】
 */
#include <stdio.h>
#include <stddef.h>
#include <assert.h>

/* ---------- 1. offsetof：把"结构体布局"变成编译期常量 ---------- */

struct Record {
    char   flag;      /* 1 + 3 padding */
    int    id;        /* 4 */
    char   name[6];   /* 6 + 2 padding */
    double score;     /* 8 */
};

/* ---------- 2. container_of ---------- */

#define container_of(ptr, type, member) \
    ((type *) ((char *) (ptr) - offsetof(type, member)))
/*  必须先转 char*：只有 char 的步长是 1，才能按"字节"回退。                     */
/*  Linux 内核版本还会用 typeof 做编译期类型检查（GCC 扩展，-Wpedantic 会警告），  */
/*  标准 C 里的等价做法是下面这种"类型安全包装函数"：只接受正确类型的指针。       */

struct List {
    struct List *prev;
    struct List *next;
};

struct Task {
    struct List node;      /* 侵入：链表节点嵌在业务结构体里，不是业务结构体挂在节点上 */
    int         id;
    const char *title;
};

/* 类型安全包装：编译器保证只有 struct List* 能进来（不用 const 是为了避开
 * container_of 内部 (char*) 转换带来的限定符去除；-Wcast-qual 会盯这个）。*/
static struct Task *task_of(struct List *n)
{
    return container_of(n, struct Task, node);
}

static void list_init(struct List *head)
{
    head->prev = head;
    head->next = head;
}

static void list_add(struct List *head, struct List *e)   /* 插到头部 */
{
    e->next = head->next;
    e->prev = head;
    head->next->prev = e;
    head->next = e;
}

#define list_for_each(pos, head) \
    for ((pos) = (head)->next; (pos) != (head); (pos) = (pos)->next)

static size_t list_count(const struct List *head)
{
    size_t n = 0;
    const struct List *p;
    list_for_each(p, head) {
        ++n;
    }
    return n;
}

int main(void)
{
    /* ===== 第 1 部分：偏移与填充 ===== */
    puts("== 1. offsetof 与结构体填充 ==");
    printf("  sizeof(struct Record) = %zu\n", sizeof(struct Record));
    printf("  offsetof(flag)   = %zu\n", offsetof(struct Record, flag));
    printf("  offsetof(id)     = %zu   （flag 后有 %zu 字节 padding）\n",
           offsetof(struct Record, id),
           offsetof(struct Record, id) - offsetof(struct Record, flag) - sizeof(((struct Record *) 0)->flag));
    printf("  offsetof(name)   = %zu\n", offsetof(struct Record, name));
    printf("  offsetof(score)  = %zu\n", offsetof(struct Record, score));
    printf("  成员字节数之和   = %zu  <-- 小于 sizeof，差额就是 padding\n",
           sizeof(((struct Record *) 0)->flag)
           + sizeof(((struct Record *) 0)->id)
           + sizeof(((struct Record *) 0)->name)
           + sizeof(((struct Record *) 0)->score));

    struct Record r = { .flag = 'A', .id = 7, .name = "abc", .score = 1.5 };
    /* 运行时测量的偏移必须与编译期 offsetof 一致 */
    char *base = (char *) &r;
    assert((size_t) ((char *) &r.id - base)    == offsetof(struct Record, id));
    assert((size_t) ((char *) &r.name - base)  == offsetof(struct Record, name));
    assert((size_t) ((char *) &r.score - base) == offsetof(struct Record, score));
    puts("  运行时偏移 == 编译期 offsetof：assert 全部通过");

    /* ===== 第 2 部分：container_of 反推 ===== */
    puts("== 2. container_of 反推宿主对象 ==");
    struct Task t = { .id = 101, .title = "first" };
    struct List *n = &t.node;
    struct Task *back = container_of(n, struct Task, node);
    assert(back == &t);                       /* 反推必须回到原点 */
    printf("  取 &t.node 反推回 &t：%s\n", back == &t ? "正确" : "错误");
    printf("  通过反推得到的对象读到 id=%d title=%s\n", back->id, back->title);
    /* 危险示范（注释掉的，别取消注释跑）：
     *   struct List standalone;
     *   struct Task *ghost = container_of(&standalone, struct Task, node);
     * ghost 是个"合法形状"的野指针，指向 standalone 前面的栈内存 —— 编译器一声不吭。
     * 这就是第 6 节说的：container_of 的前提是"这个指针真的嵌在结构体里"。
     */

    /* ===== 第 3 部分：侵入式链表 ===== */
    puts("== 3. 侵入式双向循环链表 ==");
    struct List head;
    list_init(&head);

    struct Task a1 = { .id = 1, .title = "compile" };
    struct Task a2 = { .id = 2, .title = "link"    };
    struct Task a3 = { .id = 3, .title = "debug"   };

    list_add(&head, &a3.node);
    list_add(&head, &a2.node);
    list_add(&head, &a1.node);                /* 头插，所以遍历顺序是 1,2,3 */

    printf("  节点数 = %zu（期望 3）\n", list_count(&head));

    struct List *p;
    int seen = 0;
    list_for_each(p, &head) {
        struct Task *tcur = task_of(p);        /* 唯一的类型安全入口 */
        struct Task *expect = tcur->id == 1 ? &a1 : (tcur->id == 2 ? &a2 : &a3);
        printf("    id=%d title=%-8s  container_of 回到原对象? %s\n",
               tcur->id, tcur->title, tcur == expect ? "yes" : "NO");
        ++seen;
    }
    printf("  遍历到 %d 个节点（期望 3），顺序应为 1 2 3\n", seen);

    /* 摘除一个节点（不需要知道它属于哪个容器 —— 这正是侵入式的价值）*/
    a2.node.prev->next = a2.node.next;
    a2.node.next->prev = a2.node.prev;
    printf("  摘除 id=2 后节点数 = %zu（期望 2）\n", list_count(&head));

    puts("== 4. 要点回顾 ==");
    puts("  侵入式 = 一次分配拿到业务对象+链接关系；遍历无需第二张索引表");
    puts("  代价   = 一个对象同时只能待在一条同类型链表里（要进多条就嵌多个 List 成员）");

    return 0;
}
