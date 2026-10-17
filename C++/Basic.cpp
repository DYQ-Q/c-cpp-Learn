#include <stdio.h>
#include <stdlib.h>

int main()
{
    system("chcp 65001 > nul");
    int arr[5] = {1, 2, 3, 4, 5};
    int *p = arr;    // 等价于 &arr[0]

    printf("%d\n", *p);       // 1
    printf("%d\n", *(p+2));   // 3
    printf("中文测试%d\n", p[3]);     // 4
    //printf("%d\n", arr[3]);   // 4
    
    return 0;
}