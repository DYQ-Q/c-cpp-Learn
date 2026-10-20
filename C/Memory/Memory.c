#include <stdio.h>
#include <stdlib.h>



int main()
{
    int *p = (int*)malloc(5*sizeof(int));
    if(p == NULL)
    {
        printf("malloc failed\n");
        return 1;
    }
    for(int i=0;i<5;i++)
    {
        p[i] = i;
    }
    for(int i=0;i<5;i++)
    {
        printf("p[%d]=%d\n",i,p[i]);
    }
    printf("free之前指针p的地址:%p\n",p);
    free(p);
    printf("free之后指针p的地址:%p\n",p);
    p = NULL;
    printf("执行p=NULL后p的地址:%p\n",p);
    printf("p的地址一直存储在栈上\n"
           "free(p)之后堆上的内存被释放,不可以再访问\n"
           "p=NULL之后p的地址变为0,依旧不可访问\n");
    return 0;
}