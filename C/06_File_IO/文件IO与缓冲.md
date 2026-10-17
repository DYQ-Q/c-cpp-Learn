# 文件 IO 与缓冲

> Stage 1 ｜ `C/06_File_IO/` ｜ 计划：**第 3 批**（骨架）

## 一句话定位
stdio 不是系统调用，它是在 `read/write` 之上套了一层**用户态缓冲**。理解了这层，才能解释"程序崩溃日志少了一半"和"打开 1000 个文件就失败"。

## 待覆盖
- `FILE` 结构、全缓冲/行缓冲/无缓冲、`fflush`/`setvbuf` 时机
- 文本模式 vs 二进制模式（Windows CRLF 翻译）、`"wb+"` 各模式含义
- `fseek`/`ftell`、`feof` 的滞后一位问题
- 错误处理：`ferror`、`errno`、`perror`
- 内存映射与整文件读写策略对比
- Windows 专属坑：文件被占用、路径编码、`_fileno`
