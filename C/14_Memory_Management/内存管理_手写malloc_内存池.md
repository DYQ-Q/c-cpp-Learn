# 内存管理：手写 malloc 与内存池

> Stage 4 ｜ `C/14_Memory_Management/` ｜ 计划：**第 5 批**（骨架）

## 一句话定位
`malloc` 不是"申请内存"，它是在**向操作系统借一大块后自己记账**。手写一次就再也不会误用 `free`。

## 待覆盖
- 栈/堆/.data/.bss/.rodata/内存映射区实测地址布局
- `malloc/calloc/realloc/free` 语义、`realloc` 的失败与原地扩容
- 块式分配器：空闲链表、first-fit/best-fit、分裂与合并、对齐
- 内存池 arena（O(1) 整体释放）、slab 思想
- 调试分配器：红色区域 + 影子值 + 泄漏表（ASan 的手写平替）
- 碎片、`mmap`/`VirtualAlloc`、大对象为何绕过堆
