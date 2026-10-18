/* ==========================================================================
 * 02_cli_tools.c —— 用 C 写一套「真能拿去用」的命令行工具
 *
 * 用法：
 *   02.exe hex     <文件> [偏移] [长度]     xxd 风格：十六进制 + ASCII 视图
 *   02.exe wc      <文件>...                 行 / 词 / 字节统计，多文件带 total
 *   02.exe grep    <模式> <文件> [-i] [-c]    逐行匹配 + 行号 + 反色高亮
 *   02.exe crc32   <文件>                     手写查表 CRC32（纯位运算）
 *   02.exe base64  <文件> [-d]                手写 base64 编码 / 解码
 *   02.exe                                   不带参数 = 自动 selftest：
 *                                            自己造一个文件，再用上面 5 个命令读它
 *
 * 演示能力：argv 手工解析、stdio 缓冲读写、二进制文件、位运算、
 *           动态内存、ANSI 转义颜色、把「文件」当字节流看待的思维
 * 编译：gcc -std=c17 -Wall -Wextra c\02_cli_tools.c -o out\02.exe
 * ========================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

static const char *g_self = "02_selftest.bin";

/* ---------------- hex：把文件当字节流看 ---------------- */
static int cmd_hex(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "用法: hex <文件> [偏移] [长度]\n"); return 1; }
    FILE *f = fopen(argv[1], "rb");
    if (!f) { perror(argv[1]); return 1; }

    long off = argc >= 3 ? strtol(argv[2], NULL, 0) : 0;   /* 基数 0 -> 支持 0x 前缀 */
    long len = argc >= 4 ? strtol(argv[3], NULL, 0) : -1;  /* -1 = 全部 */
    if (off != 0 && fseek(f, off, SEEK_SET) != 0) { perror("fseek"); fclose(f); return 1; }

    printf("%-10s  %-48s  %s\n", "offset", "bytes", "ascii");
    printf("-----------------------------------------------------------------\n");

    unsigned char buf[16];
    size_t n;
    long shown = 0;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0 && (len < 0 || shown < len)) {
        if (len >= 0 && (long)n > len - shown) n = (size_t)(len - shown);
        printf("%08lX  ", (unsigned long)(off + shown));
        for (size_t i = 0; i < 16; i++)            /* 不足 16 字节补空格对齐 */
            printf(i < n ? "%02X " : "   ", buf[i]);
        printf(" |");
        for (size_t i = 0; i < n; i++)
            putchar(buf[i] >= ' ' && buf[i] < 0x7F ? buf[i] : '.');  /* 不可打印用点 */
        printf("|\n");
        shown += (long)n;
    }
    fclose(f);
    printf("共显示 %ld 字节\n", shown);
    return 0;
}

/* ---------------- wc：一次字节扫描统计三种数 ---------------- */
static void wc_one(const char *path, long *lines, long *words, long *bytes)
{
    *lines = *words = *bytes = 0;
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); return; }
    int c, in_word = 0;
    while ((c = fgetc(f)) != EOF) {
        (*bytes)++;
        if (c == '\n') (*lines)++;
        if (isspace(c))            in_word = 0;     /* 状态机：进/出单词 */
        else if (!in_word) { (*words)++; in_word = 1; }
    }
    fclose(f);
}

static int cmd_wc(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "用法: wc <文件>...\n"); return 1; }
    long tl = 0, tw = 0, tb = 0;
    for (int i = 1; i < argc; i++) {
        long l, w, b;
        wc_one(argv[i], &l, &w, &b);
        printf("%6ld %6ld %6ld %s\n", l, w, b, argv[i]);
        tl += l; tw += w; tb += b;
    }
    if (argc > 2) printf("%6ld %6ld %6ld total\n", tl, tw, tb);
    return 0;
}

/* ---------------- grep：手写大小写无关查找 + ANSI 反色高亮 ---------------- */
static long find_sub(const char *hay, const char *needle, int icase)
{
    size_t nl = strlen(needle);
    if (!nl) return -1;
    for (long i = 0; hay[i]; i++) {
        size_t k = 0;
        while (k < nl && hay[i + k]) {
            char a = hay[i + k], b = needle[k];
            if (icase) {
                a = (char)tolower((unsigned char)a);
                b = (char)tolower((unsigned char)b);
            }
            if (a != b) break;
            k++;
        }
        if (k == nl) return i;
    }
    return -1;
}

static int cmd_grep(int argc, char **argv)
{
    if (argc < 3) { fprintf(stderr, "用法: grep <模式> <文件> [-i] [-c]\n"); return 1; }
    const char *pat  = argv[1];
    const char *path = argv[2];
    int icase = 0, count_only = 0;
    for (int i = 3; i < argc; i++) {
        if (!strcmp(argv[i], "-i"))     icase = 1;
        else if (!strcmp(argv[i], "-c")) count_only = 1;
    }

    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); return 1; }

    char line[8192];
    long lineno = 0, hits = 0;
    size_t plen = strlen(pat);
    while (fgets(line, sizeof line, f)) {
        lineno++;
        size_t l = strlen(line);
        if (l && line[l - 1] == '\n') line[--l] = 0;          /* 去掉换行符 */
        long pos = find_sub(line, pat, icase);
        if (pos < 0) continue;
        hits++;
        if (count_only) continue;
        printf("\033[36m%s:%ld:\033[0m ", path, lineno);       /* 文件名+行号：青色 */
        for (long i = 0; line[i]; i++) {
            if (i == pos) fputs("\033[7m", stdout);             /* 反色包住匹配段 */
            putchar((unsigned char)line[i]);
            if (i == (long)(pos + plen) - 1) fputs("\033[27m", stdout);
        }
        putchar('\n');
    }
    fclose(f);
    if (count_only) printf("%s: %ld 行匹配\n", path, hits);
    else if (!hits) printf("没有找到「%s」\n", pat);
    return hits ? 0 : 1;                                        /* 和真 grep 一样用退出码 */
}

/* ---------------- CRC32：查表位运算，zip/png 里那个校验值 ---------------- */
static uint32_t crc_tab[256];
static void crc32_init(void)
{
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int k = 0; k < 8; k++)
            c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);   /* 反射多项式 */
        crc_tab[i] = c;
    }
}
static uint32_t crc32_buf(const unsigned char *p, size_t n, uint32_t prev)
{
    uint32_t c = prev ^ 0xFFFFFFFFu;
    for (size_t i = 0; i < n; i++) c = crc_tab[(c ^ p[i]) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}
static int cmd_crc32(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "用法: crc32 <文件>\n"); return 1; }
    crc32_init();
    FILE *f = fopen(argv[1], "rb");
    if (!f) { perror(argv[1]); return 1; }
    unsigned char buf[8192];
    size_t n;
    uint32_t c = 0;
    long total = 0;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0) { c = crc32_buf(buf, n, c); total += (long)n; }
    fclose(f);
    printf("%s: %ld 字节, CRC32 = %08X\n", argv[1], total, c);
    return 0;
}
/* ---------------- base64：3 字节 <-> 4 可打印字符的位重排 ---------------- */
static const char B64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int cmd_base64(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "用法: base64 <文件> [-d]\n"); return 1; }
    int decode = (argc >= 3 && !strcmp(argv[2], "-d"));
    FILE *f = fopen(argv[1], "rb");
    if (!f) { perror(argv[1]); return 1; }

    if (!decode) {
        unsigned char in[3];
        int n, col = 0;
        while ((n = (int)fread(in, 1, 3, f)) > 0) {
            char o[4];
            o[0] = B64[in[0] >> 2];
            o[1] = '='; o[2] = '='; o[3] = '=';
            if (n == 1) {
                o[1] = B64[(in[0] & 3) << 4];
            } else {
                o[1] = B64[((in[0] & 3) << 4) | (in[1] >> 4)];
                if (n == 2) o[2] = B64[(in[1] & 15) << 2];
                else { o[2] = B64[((in[1] & 15) << 2) | (in[2] >> 6)]; o[3] = B64[in[2] & 63]; }
            }
            fwrite(o, 1, 4, stdout);
            if ((col += 4) >= 72) { putchar('\n'); col = 0; }
        }
        if (col) putchar('\n');
    } else {
        signed char rev[256];
        for (int i = 0; i < 256; i++) rev[i] = -1;
        for (int i = 0; i < 64; i++) rev[(unsigned char)B64[i]] = (signed char)i;
        int q[4], qi = 0, c;
        while ((c = fgetc(f)) != EOF) {
            signed char val = rev[(unsigned char)c];
            if (val < 0) continue;                  /* 换行、空格、'=' 全部跳过 */
            q[qi++] = val;
            if (qi == 4) {
                unsigned char b[3];
                b[0] = (unsigned char)((q[0] << 2) | (q[1] >> 4));
                b[1] = (unsigned char)((q[1] << 4) | (q[2] >> 2));
                b[2] = (unsigned char)((q[2] << 6) | q[3]);
                fwrite(b, 1, 3, stdout);
                qi = 0;
            }
        }
        if (qi >= 2) {                              /* 结尾不足 4 个的补位情况 */
            unsigned char b[3] = {0, 0, 0};
            b[0] = (unsigned char)((q[0] << 2) | (q[1] >> 4));
            if (qi >= 3) b[1] = (unsigned char)((q[1] << 4) | (q[2] >> 2));
            fwrite(b, 1, (size_t)qi - 1, stdout);
        }
    }
    fclose(f);
    return 0;
}

/* ---------------- selftest：自己造文件，再用自己写的工具读它 ---------------- */
static int selftest(void)
{
    FILE *f = fopen(g_self, "wb");
    if (!f) { perror(g_self); return 1; }
    fprintf(f, "Hello C systems programming!\n");
    fprintf(f, "the second line hides a needle here.\n");
    fprintf(f, "NEEDLE uppercase also matches with -i\n");
    fprintf(f, "binary tail:\x00\x01\x02\xFE\xFF\x7F\n");
    fclose(f);
    printf("== 已生成测试文件 %s ==\n\n", g_self);

    char *a_hex[]  = { "hex",   (char *)g_self, NULL };
    char *a_wc[]   = { "wc",    (char *)g_self, (char *)"README.md", NULL };
    char *a_grep[] = { "grep",  (char *)"needle", (char *)g_self, (char *)"-i", NULL };
    char *a_crc[]  = { "crc32", (char *)g_self, NULL };
    char *a_b64[]  = { "base64", (char *)g_self, NULL };

    cmd_hex(2, a_hex);    putchar('\n');
    cmd_wc(3, a_wc);      putchar('\n');
    cmd_grep(4, a_grep);  putchar('\n');
    cmd_crc32(2, a_crc);  putchar('\n');
    puts("base64（把任意字节变成可打印文本 —— 邮件附件、data-uri 图片都是这个原理）:");
    cmd_base64(2, a_b64);
    printf("\n自己试试往返：out\\02.exe base64 %s > t.b64", g_self);
    puts("                     然后 out\\02.exe base64 t.b64 -d 就能还原原文件");
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) return selftest();

    const char *cmd = argv[1];
    argc--; argv++;                       /* 剥掉命令名，子命令只看到自己的参数 */
    if (!strcmp(cmd, "hex"))    return cmd_hex(argc, argv);
    if (!strcmp(cmd, "wc"))     return cmd_wc(argc, argv);
    if (!strcmp(cmd, "grep"))   return cmd_grep(argc, argv);
    if (!strcmp(cmd, "crc32"))  return cmd_crc32(argc, argv);
    if (!strcmp(cmd, "base64")) return cmd_base64(argc, argv);

    fprintf(stderr, "未知命令: %s\n可用: hex / wc / grep / crc32 / base64（不带参数=selftest）\n", cmd);
    return 1;
}


