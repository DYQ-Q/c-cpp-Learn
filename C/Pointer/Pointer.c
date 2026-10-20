#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RED(format, ...)     "\033[31m" format "\033[0m", ##__VA_ARGS__
#define GREEN(format, ...)   "\033[32m" format "\033[0m", ##__VA_ARGS__
#define YELLOW(format, ...)  "\033[33m" format "\033[0m", ##__VA_ARGS__
#define BLUE(format, ...)    "\033[34m" format "\033[0m", ##__VA_ARGS__

void init();
void pointer_var();
void pointer_array();
void pointer_string();
void pointer_struct();

int main()
{
    init();
    // pointer_var();
    // pointer_array();
    // pointer_string();
    pointer_struct();
    return 0;
}

void init()
{
    system("chcp 65001 > nul");
    printf(GREEN("init--------\n"));
    printf("English Test\n");
    printf("中文测试\n");
}

void pointer_var()
{
    printf(GREEN("pointer-var--------\n"));
    int a = 10;
    int *p = &a;
    printf("a=%d,&a=%p\n"
           "*p=%d,p=%p,&p=%p\n", a, &a, *p, p, &p);
    printf(YELLOW("p=&a,p里面存的a的地址,*p=a,*p里面存的a的值,p也有自己的地址&p\n"));
}

void pointer_array()
{
    printf(GREEN("pointer-array--------\n"));
    int a[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    int *p1 = a;//数组名就是第一个元素的地址，等价于int *p1=&a[0];
    int *p2 = &a[0];
}

void pointer_string()
{
    printf(GREEN("pointer-string--------\n"));
    printf(YELLOW("字符数组\n"));
    char str[] = "hello";//字符数组
    printf("str=%s\n"
           "str[0]=%C,str[4]=%c\n"
           "字符串长度=%d\n"
           "字符串大小=%d\n",str,str[0],str[4],strlen(str),sizeof(str));
    printf(YELLOW("指向字符串的指针\n"));
    char *p = "string";
    printf("*p=%c,p指向字符串首个字符的地址\n"
           " p=%s,%%s会从p指向的地址开始读取,直到遇到\\0\n", *p,p);
    printf(YELLOW("指向字符串的指针数组\n"));
    char *m[] = {"hello","world"};
    printf("*m[0]=%c\n"
           "m[1]=%s\n"
           "指向字符串的指针数组也是一样的道理",*m[0],m[1]);
    
}

void pointer_struct()
{
    printf(GREEN("pointer-struct--------\n"));
    typedef struct{
        int a;
        int* p;
    }stc;
    stc s1;

    int b = 10;
    s1.a = 100;
    s1.p = &b;

    printf("s1.a=%d,s1.p=%p,*s1.p=%d\n"
           "&b=%p,b=%d,结构体s1的一个int型指针成员p指向b\n",s1.a,s1.p,*s1.p,&b,b);
    stc *p1 = &s1;
    printf("p1->a=%d,p1->p=%p,*p1->p=%d\n",p1->a,p1->p,*p1->p);
}