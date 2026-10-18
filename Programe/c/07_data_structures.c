/* ==========================================================================
 * 07_data_structures.c —— 用 C 造自己的数据结构（泛型 + 哈希）
 *
 * 演示能力：
 *   1. 用 void* + elem_size 实现「泛型动态数组」—— C 没有模板也能泛型
 *   2. 手写哈希表：FNV-1a 哈希、链地址法、负载因子触发扩容 rehash
 *   3. 对比实验：查 2 万次，「哈希表 O(1)」和「数组线性找 O(n)」差多少个数量级
 *   4. 顺带做一件实用的事：统计一篇文章的词频并输出 Top-N
 *
 * 你会直观理解：std::vector / unordered_map / Python 的 dict 底层是什么。
 *
 * 编译：gcc -std=c17 -Wall -Wextra -O2 c\07_data_structures.c -o out\07.exe
 * 运行：out\07.exe            （或 out\07.exe 某个文本文件 来统计你自己的文件）
 * ========================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

static void die(const char *m) { perror(m); exit(EXIT_FAILURE); }

/* ---------------------- 1. 泛型动态数组 ---------------------- */
typedef struct { void *data; size_t len, cap, esz; } Vec;

static void vec_init(Vec *v, size_t esz)
{
    v->data = NULL; v->len = 0; v->cap = 0; v->esz = esz;
}
static void vec_free(Vec *v) { free(v->data); vec_init(v, v->esz); }

static void vec_push(Vec *v, const void *item)
{
    if (v->len == v->cap) {
        size_t nc = v->cap ? v->cap * 2 : 8;
        void *tmp = realloc(v->data, nc * v->esz);
        if (!tmp) die("realloc");
        v->data = tmp; v->cap = nc;
    }
    /* 关键：按「字节大小」拷贝元素，而不是按类型 —— 这就是泛型的秘密 */
    memcpy((char *)v->data + v->len * v->esz, item, v->esz);
    v->len++;
}
static void *vec_at(Vec *v, size_t i)
{
    if (i >= v->len) { fprintf(stderr, "vec_at 越界: %zu >= %zu\n", i, v->len); abort(); }
    return (char *)v->data + i * v->esz;
}
static void vec_remove(Vec *v, size_t i)          /* 保持顺序：整体前移 */
{
    if (i >= v->len) return;
    memmove((char *)v->data + i * v->esz,
            (char *)v->data + (i + 1) * v->esz,
            (v->len - i - 1) * v->esz);
    v->len--;
}
#define VEC_LEN(v) ((v)->len)

/* ---------------------- 2. 哈希表（链地址法） ---------------------- */
typedef struct Entry {
    char        *key;
    void        *val;         /* 值由用户 own 的字节块，表负责 free */
    size_t       valsz;
    struct Entry *next;
} Entry;

typedef struct {
    Entry **buckets;
    size_t nbuckets;
    size_t size;              /* 实际元素数，用来算负载因子 */
    size_t collisions;        /* 顺便统计冲突次数，看看哈希质量 */
} Map;

static Map *map_new(size_t nbuckets)
{
    Map *m = malloc(sizeof *m);
    if (!m) die("malloc map");
    m->buckets = calloc(nbuckets, sizeof *m->buckets);
    if (!m->buckets) die("calloc buckets");
    m->nbuckets = nbuckets;
    m->size = 0;
    m->collisions = 0;
    return m;
}

/* FNV-1a：几行代码、分布不错、速度极快，很多真实系统就用它 */
static size_t fnv1a(const char *s)
{
    size_t h = (size_t)1469598103934665603ull;      /* 64 位 FNV offset basis */
    while (*s) { h ^= (unsigned char)*s++; h *= (size_t)1099511628211ull; }
    return h;
}

static void map_resize(Map *m, size_t nb)
{
    Entry **nbk = calloc(nb, sizeof *nbk);
    if (!nbk) die("calloc resize");
    for (size_t i = 0; i < m->nbuckets; i++) {
        Entry *e = m->buckets[i];
        while (e) {
            Entry *next = e->next;
            size_t j = fnv1a(e->key) % nb;
            if (nbk[j]) m->collisions++;
            e->next = nbk[j];
            nbk[j]  = e;
            e = next;
        }
    }
    free(m->buckets);
    m->buckets  = nbk;
    m->nbuckets = nb;
}

/* 把 key/value 都复制进堆里，调用方之后可以随便销毁自己的原始数据 */
static int map_put(Map *m, const char *key, const void *val, size_t valsz)
{
    if ((m->size + 1) * 4 > m->nbuckets * 3) {       /* 负载因子 > 0.75 就翻倍 */
        size_t old = m->nbuckets;
        map_resize(m, old * 2);
        printf("  [rehash] %zu -> %zu 个桶\n", old, m->nbuckets);
    }
    size_t b = fnv1a(key) % m->nbuckets;
    for (Entry *e = m->buckets[b]; e; e = e->next) {
        if (!strcmp(e->key, key)) {                  /* 已存在 -> 覆盖 */
            if (e->valsz >= valsz) { memcpy(e->val, val, valsz); }
            else { free(e->val); e->val = malloc(valsz); e->valsz = valsz; memcpy(e->val, val, valsz); }
            return 0;
        }
        m->collisions++;
    }
    Entry *e = malloc(sizeof *e);
    e->key = malloc(strlen(key) + 1);
    strcpy(e->key, key);
    e->val = malloc(valsz ? valsz : 1);
    memcpy(e->val, val, valsz);
    e->valsz = valsz;
    e->next = m->buckets[b];
    m->buckets[b] = e;
    m->size++;
    return 1;
}

static void *map_get(Map *m, const char *key)
{
    Entry *e = m->buckets[fnv1a(key) % m->nbuckets];
    while (e) { if (!strcmp(e->key, key)) return e->val; e = e->next; }
    return NULL;
}

static int map_remove(Map *m, const char *key)
{
    size_t b = fnv1a(key) % m->nbuckets;
    Entry **pp = &m->buckets[b];
    while (*pp) {
        if (!strcmp((*pp)->key, key)) {
            Entry *dead = *pp;
            *pp = dead->next;
            free(dead->key); free(dead->val); free(dead);
            m->size--;
            return 1;
        }
        pp = &(*pp)->next;
    }
    return 0;
}

typedef void (*MapCb)(const char *key, void *val, void *ctx);
static void map_each(Map *m, MapCb cb, void *ctx)
{
    for (size_t i = 0; i < m->nbuckets; i++)
        for (Entry *e = m->buckets[i]; e; e = e->next) cb(e->key, e->val, ctx);
}

static void map_free(Map *m)
{
    for (size_t i = 0; i < m->nbuckets; i++) {
        Entry *e = m->buckets[i];
        while (e) { Entry *n = e->next; free(e->key); free(e->val); free(e); e = n; }
    }
    free(m->buckets);
    free(m);
}
/* ---------------------- 3. 实战：词频统计（Top-N） ---------------------- */
typedef struct { char word[40]; int count; } WordCount;

static void count_word(Map *m, const char *w)
{
    int *c = (int *)map_get(m, w);
    if (c) { (*c)++; }
    else   { int one = 1; map_put(m, w, &one, sizeof one); }
}

static void collect_cb(const char *key, void *val, void *ctx)
{
    Vec *v = (Vec *)ctx;
    WordCount wc;
    snprintf(wc.word, sizeof wc.word, "%s", key);
    wc.count = *(int *)val;
    vec_push(v, &wc);
}

static int cmp_wc_desc(const void *a, const void *b)
{
    const WordCount *x = a, *y = b;
    if (x->count != y->count) return y->count - x->count;
    return strcmp(x->word, y->word);
}

static const char *k_corpus =
    "the quick brown fox jumps over the lazy dog\n"
    "the dog barks and the fox runs and the fox runs again\n"
    "a C program manages its own memory and a C program can crash the world\n"
    "memory safety is a promise you either keep or break\n"
    "the pointer points at the array and the array decays into the pointer\n";

static void demo_wordfreq(const char *path)
{
    puts("===== 3. 词频统计（哈希表 + 泛型数组 + qsort 三件套）=====");
    Map *m = map_new(64);
    Vec all; vec_init(&all, sizeof(WordCount));

    char w[64];
    int  wi = 0;

    /* 一个字符一个字符地扫，遇到非字母数字就切一个词 —— 这就是「分词器」的最小形态 */
#define FLUSH()                                        \
    do {                                               \
        if (wi > 0) { w[wi] = 0; count_word(m, w); }   \
        wi = 0;                                        \
    } while (0)

    FILE *f = path ? fopen(path, "rb") : NULL;
    if (path && !f) { perror(path); }
    int ch;
    if (f) {
        while ((ch = fgetc(f)) != EOF) {
            if (isalnum(ch) && wi < (int)sizeof w - 1) w[wi++] = (char)tolower(ch);
            else FLUSH();
        }
        fclose(f);
    } else {
        const char *s = k_corpus;
        for (; *s; s++) {
            ch = (unsigned char)*s;
            if (isalnum(ch) && wi < (int)sizeof w - 1) w[wi++] = (char)tolower(ch);
            else FLUSH();
        }
    }
    FLUSH();
#undef FLUSH

    printf("  不同词数 = %zu，哈希冲突次数 = %zu，桶数 = %zu\n",
           m->size, m->collisions, m->nbuckets);

    map_each(m, collect_cb, &all);
    qsort(all.data, all.len, all.esz, cmp_wc_desc);

    printf("  Top-%zu 高频词：", VEC_LEN(&all) < 10 ? VEC_LEN(&all) : 10);
    size_t top = all.len < 10 ? all.len : 10;
    for (size_t i = 0; i < top; i++) {
        WordCount *p = (WordCount *)vec_at(&all, i);
        printf(" %s(%d)", p->word, p->count);
    }
    printf("\n  演示 vec_remove：删掉排名第 1 的词后，剩 %zu 个\n", all.len - 1);
    vec_remove(&all, 0);

    map_free(m);
    vec_free(&all);
    puts("");
}

/* ---------------------- 4. 基准：哈希 O(1) vs 线性 O(n) ---------------------- */
typedef struct { char key[24]; int val; } KV;

static int linear_get(Vec *v, const char *key)     /* 最朴素的「全都看一遍」 */
{
    for (size_t i = 0; i < v->len; i++) {
        KV *p = (KV *)vec_at(v, i);
        if (!strcmp(p->key, key)) return p->val;
    }
    return -1;
}

static void demo_benchmark(void)
{
    puts("===== 4. 同样的需求，两种数据结构，跑速差多少 =====");
    const int N = 8000;
    Map *m = map_new(64);
    Vec  v; vec_init(&v, sizeof(KV));

    char key[24];
    for (int i = 0; i < N; i++) {
        snprintf(key, sizeof key, "item_%06d", i);
        map_put(m, key, &i, sizeof i);
        KV kv; snprintf(kv.key, sizeof kv.key, "%s", key); kv.val = i;
        vec_push(&v, &kv);
    }

    clock_t t0 = clock();
    long hs = 0;
    for (int i = 0; i < N; i++) {
        snprintf(key, sizeof key, "item_%06d", i);
        int *p = (int *)map_get(m, key);
        if (p) hs += *p;
    }
    double t_hash = (double)(clock() - t0) / CLOCKS_PER_SEC * 1000.0;

    t0 = clock();
    long ls = 0;
    for (int i = 0; i < 500; i++) {              /* 线性太慢，只查 500 次做对比 */
        snprintf(key, sizeof key, "item_%06d", i);
        ls += linear_get(&v, key);
    }
    double t_lin = (double)(clock() - t0) / CLOCKS_PER_SEC * 1000.0;
    double t_lin_full = t_lin / 500.0 * N;       /* 外推到 N 次要多久 */

    printf("  哈希表 %d 次查找: %8.2f ms   (校验和 %ld)\n", N, t_hash, hs);
    printf("  线性查找 %d 次: 约 %8.2f ms（按 %d 次实测外推）\n", N, t_lin_full, 500);
    printf("  差距约 %.0f 倍 —— 这就是「选对数据结构」的价值\n",
           t_lin_full / (t_hash > 0.01 ? t_hash : 0.01));

    map_free(m);
    vec_free(&v);
}

int main(int argc, char **argv)
{
    puts("C 没有模板、没有 STL，但 vector 和 unordered_map 的底层就是下面这两百行。\n");
    demo_wordfreq(argc > 1 ? argv[1] : NULL);
    demo_benchmark();
    puts("\n顺带一提：把上面这段重写一遍，就是 std::vector / std::unordered_map 的雏形。");
    return 0;
}

