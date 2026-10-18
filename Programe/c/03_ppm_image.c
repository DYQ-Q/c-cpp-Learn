/* ==========================================================================
 * 03_ppm_image.c —— 不装任何图形库，纯算术生成真正的图片文件
 *
 * C/C++ 的「图形能力」底层其实就这么朴素：算出一堆 RGB 字节，按某种格式写进文件。
 * 这里用的是 PPM(P6)：一行头部 + 裸 RGB 字节，Windows 可直接用 IrfanView/GIMP/XnView
 * 打开，也能在 https://pinetools.com/convert-image 之类网站转成 PNG，或者用
 * PowerShell 的 ImageMagick / ffmpeg:  ffmpeg -i img/mandelbrot.ppm img/mandelbrot.png
 *
 * 生成三张图 + 一段控制台 ASCII 预览：
 *   img/gradient.ppm     双线性颜色插值（理解「图像 = 数组」）
 *   img/mandelbrot.ppm   分形迭代 + 平滑着色 + sqrt 色带
 *   img/sphere.ppm       最简光线追踪：相机、射线-球求交、法线、Lambert 光照、
 *                        棋盘地面、镜面反射（全是浮点数学，没有一行图形 API）
 *
 * 编译：gcc -std=c17 -Wall -Wextra -O2 c\03_ppm_image.c -o out\03.exe
 * 运行：out\03.exe        （会自动创建 img 目录）
 * ========================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
#  include <direct.h>
#  define MKDIR(p) _mkdir(p)
#else
#  include <sys/stat.h>
#  define MKDIR(p) mkdir(p, 0755)
#endif

typedef struct { int w, h; unsigned char *px; } Image;   /* px = w*h*3 的 RGB 缓冲区 */

static Image *img_new(int w, int h)
{
    Image *im = malloc(sizeof *im);
    if (!im) exit(1);
    im->w = w; im->h = h;
    im->px = calloc((size_t)w * h, 3);          /* calloc 直接得到全黑底 */
    if (!im->px) exit(1);
    return im;
}
static void img_free(Image *im) { free(im->px); free(im); }

static void img_put(Image *im, int x, int y, int r, int g, int b)
{
    if (x < 0 || y < 0 || x >= im->w || y >= im->h) return;   /* 越界直接丢弃 */
    size_t i = ((size_t)y * im->w + x) * 3;
    im->px[i + 0] = (unsigned char)r;
    im->px[i + 1] = (unsigned char)g;
    im->px[i + 2] = (unsigned char)b;
}

static int img_save(const Image *im, const char *path)
{
    FILE *f = fopen(path, "wb");                 /* 二进制写入，千万别用 "w" */
    if (!f) { perror(path); return -1; }
    fprintf(f, "P6\n%d %d\n255\n", im->w, im->h);            /*  magic/宽高/最大值 */
    fwrite(im->px, 1, (size_t)im->w * im->h * 3, f);
    fclose(f);
    printf("  写出 %-22s %dx%d  %ld 字节\n", path, im->w, im->h,
           (long)(im->w * im->h * 3 + 20));
    return 0;
}

static int clamp255(double v) { return v < 0 ? 0 : (v > 255 ? 255 : (int)v); }

/* ---------------- 1. 渐变：证明「图片就是一个大数组」 ---------------- */
static void make_gradient(void)
{
    Image *im = img_new(400, 200);
    for (int y = 0; y < im->h; y++)
        for (int x = 0; x < im->w; x++) {
            double u = (double)x / (im->w - 1);
            double v = (double)y / (im->h - 1);
            img_put(im, x, y, clamp255(u * 255), clamp255(v * 255),
                    clamp255(255.0 * (1.0 - u * v)));
        }
    /* 再画个圆：只是判断 (x-cx)^2+(y-cy)^2 <= r^2 */
    for (int a = 0; a < 360; a++) {
        double t = a * 3.14159265358979 / 180.0;
        img_put(im, 200 + (int)(60 * cos(t)), 100 + (int)(60 * sin(t)), 255, 255, 255);
    }
    img_save(im, "img/gradient.ppm");
    img_free(im);
}

/* ---------------- 2. Mandelbrot 分形 ---------------- */
static void make_mandelbrot(void)
{
    const int W = 640, H = 480, MAXIT = 128;
    Image *im = img_new(W, H);
    const double xmin = -2.4, xmax = 0.9, ymin = -1.3, ymax = 1.3;

    for (int py = 0; py < H; py++) {
        for (int px = 0; px < W; px++) {
            double cr = xmin + (xmax - xmin) * px / (W - 1);
            double ci = ymin + (ymax - ymin) * py / (H - 1);
            double zr = 0, zi = 0, zr2 = 0, zi2 = 0;
            int it = 0;
            while (zr2 + zi2 <= 4.0 && it < MAXIT) {
                zi = 2.0 * zr * zi + ci;          /* z = z^2 + c  （复数乘法展开） */
                zr = zr2 - zi2 + cr;
                zr2 = zr * zr;
                zi2 = zi * zi;
                it++;
            }
            if (it == MAXIT) {
                img_put(im, px, py, 0, 0, 0);     /* 集合内部 = 黑 */
            } else {
                /* 平滑计数：去掉整条带子状的色阶断层 */
                double mu = it + 1.0 - log(log(sqrt(zr2 + zi2)) / log(2.0));
                double t  = mu / MAXIT;
                img_put(im, px, py,
                        clamp255(255 * (0.5 + 0.5 * sin(6.28318 * (t * 3.0 + 0.0)))),
                        clamp255(255 * (0.5 + 0.5 * sin(6.28318 * (t * 3.0 + 0.33)))),
                        clamp255(255 * (0.5 + 0.5 * sin(6.28318 * (t * 3.0 + 0.66)))));
            }
        }
    }
    img_save(im, "img/mandelbrot.ppm");
    img_free(im);
}
/* ---------------- 3. 最简光线追踪 ---------------- */
typedef struct { double x, y, z; } V;
static V   v(double x, double y, double z) { V r = {x, y, z}; return r; }
static V   vadd(V a, V b) { return v(a.x + b.x, a.y + b.y, a.z + b.z); }
static V   vsub(V a, V b) { return v(a.x - b.x, a.y - b.y, a.z - b.z); }
static V   vmul(V a, double t) { return v(a.x * t, a.y * t, a.z * t); }
static double vdot(V a, V b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static double vlen(V a) { return sqrt(vdot(a, a)); }
static V   vnorm(V a) { double l = vlen(a); return l > 1e-12 ? vmul(a, 1.0 / l) : a; }
static V   vrefl(V d, V n) { return vsub(d, vmul(n, 2.0 * vdot(d, n))); }

#define EPS 1e-4

/* 射线 p + t*d 与球心 c 半径 r 求交，返回最近正 t，没打中返回 -1 */
static double hit_sphere(V p, V d, V c, double r)
{
    V oc = vsub(p, c);
    double b = vdot(oc, d);
    double cc = vdot(oc, oc) - r * r;
    double disc = b * b - cc;
    if (disc < 0) return -1.0;
    double s = sqrt(disc);
    double t = -b - s;
    if (t < EPS) t = -b + s;
    return t > EPS ? t : -1.0;
}

/* 地面：y = -1 的无限平面 */
static double hit_plane(V p, V d)
{
    if (fabs(d.y) < 1e-9) return -1.0;
    double t = (-1.0 - p.y) / d.y;
    return t > EPS ? t : -1.0;
}

static const V  SPHERE = { 0.0, 0.0, -3.0 };
static const double SRAD = 1.0;
static const V  LIGHT = { -0.5, 0.8, -0.3 };   /* 方向光，下面会归一化 */

/* 返回一个像素的颜色（0..255），depth 控制递归反射层数 */
static void trace(V p, V d, int depth, int *r, int *g, int *b)
{
    double ts = hit_sphere(p, d, SPHERE, SRAD);
    double tp = hit_plane(p, d);
    V L = vnorm(LIGHT);

    if (ts > 0 && (tp < 0 || ts < tp)) {              /* 打到球 */
        V hit = vadd(p, vmul(d, ts));
        V n = vmul(vsub(hit, SPHERE), 1.0 / SRAD);
        double diff = vdot(n, L);
        double amb = 0.12, lam = amb + (diff > 0 ? diff * 0.9 : 0.0);
        double fres = 0.15 + 0.85 * pow(1.0 - (diff > 0 ? diff : 0.0), 3.0);  /* 边缘更亮 */
        int cr = 120, cg = 160, cb = 255;             /* 球本体颜色 */
        if (depth < 2) {                              /* 反射一次地面 */
            V rd = vrefl(d, n);
            int rr, rg, rb;
            trace(vadd(hit, vmul(n, EPS * 10)), rd, depth + 1, &rr, &rg, &rb);
            cr = (int)(cr * lam + rr * fres * 0.45);
            cg = (int)(cg * lam + rg * fres * 0.45);
            cb = (int)(cb * lam + rb * fres * 0.45);
        } else {
            cr = (int)(cr * lam); cg = (int)(cg * lam); cb = (int)(cb * lam);
        }
        *r = clamp255(cr); *g = clamp255(cg); *b = clamp255(cb);
        return;
    }
    if (tp > 0) {                                     /* 打到地面：棋盘格 */
        V hit = vadd(p, vmul(d, tp));
        double fx = floor(hit.x), fz = floor(hit.z);
        int checker = (((int)fx + (int)fz) & 1);
        double diff = L.y > 0 ? L.y : 0.0;
        double base = checker ? 0.85 : 0.25;
        double shade = (0.2 + 0.8 * diff) * base;
        /* 距离越远越暗，模拟简单雾 */
        double fog = 1.0 / (1.0 + 0.0015 * tp * tp);
        *r = clamp255(230 * shade * fog);
        *g = clamp255(225 * shade * fog);
        *b = clamp255(215 * shade * fog);
        return;
    }
    /* 天空：按射线仰角做渐变 */
    double t = 0.5 * (d.y + 1.0);
    *r = clamp255(60 + 120 * t);
    *g = clamp255(100 + 120 * t);
    *b = clamp255(160 + 95 * t);
}

static void make_sphere(void)
{
    const int W = 640, H = 400;
    const double focal = 2.0;
    Image *im = img_new(W, H);
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            /* 屏幕坐标 -> 世界方向（注意 y 翻转） */
            double sx = (2.0 * (x + 0.5) / W - 1.0) * (double)W / H;
            double sy = 1.0 - 2.0 * (y + 0.5) / H;
            V dir = vnorm(v(sx, sy, -focal));
            int r, g, b;
            trace(v(0, 0, 0), dir, 0, &r, &g, &b);
            img_put(im, x, y, r, g, b);
        }
    }
    img_save(im, "img/sphere.ppm");
    img_free(im);
}

/* ---------------- 4. 控制台 ASCII 预览：把亮度映射成字符 ---------------- */
static void ascii_preview(void)
{
    const int W = 60, H = 22;
    const char *ramp = " .:-=+*#%@";          /* 10 级亮度 */
    puts("\n同一个场景的 ASCII 版本（同一个 trace 函数，只是把亮度换成字符）：\n");
    for (int y = 0; y < H; y++) {
        for (int xi = 0; xi < W; xi++) {          /* 每格横向打两个字符，抵消字符高>宽 */
            double sx = (2.0 * (xi + 0.5) / W - 1.0) * 1.6;
            double sy = 1.0 - 2.0 * (y + 0.5) / H;
            V dir = vnorm(v(sx, sy, -2.0));
            int r, g, b;
            trace(v(0, 0, 0), dir, 0, &r, &g, &b);
            double lum = 0.299 * r + 0.587 * g + 0.114 * b;
            int idx = (int)(lum / 256.0 * 9.999);
            putchar(ramp[idx]);
            putchar(ramp[idx]);
        }
        putchar('\n');
    }
}

int main(void)
{
    MKDIR("img");                    /* Windows: direct.h 的 _mkdir；POSIX: sys/stat.h */
    puts("生成图片中……（不需要任何第三方库，只有 math.h 和 stdio）");
    make_gradient();
    make_mandelbrot();
    make_sphere();
    ascii_preview();
    puts("\n输出在 img/ 目录。想变成 PNG：");
    puts("  ffmpeg -i img/sphere.ppm img/sphere.png");
    puts("或者用 GIMP / IrfanView / 在线转换器打开 .ppm。");
    puts("结论：图形、图像、游戏画面，在 C 眼里都只是「算出来的字节数组」。");
    return 0;
}

