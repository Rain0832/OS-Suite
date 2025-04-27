# OS-Suite
类 UNIX 操作系统内核及应用集成工具包

2025.03–2025.04

项目描述：基于类UNIX操作系统API的命令行解析、系统调用封装、文本补全等工具包的开发。
应用技术：C/C++17、Linux/GCC、GDB、Git、GNU Make、TestKit测试框架
主要工作：      

- Pstree 父子进程树可视化工具，优化高并发环境下的遍历性能。
- Sperf 基于strace实时统计目标程序各系统调用耗时工具，实现短/长时运行模式下不同报告策略。
- Crepl 实现行级REPL，动态编译加载函数定义，及输入表达式的即时求值与输出。
-  Mymalloc 在零依赖环境下基于mmap/munmap构建多核安全的内存分配器，支持可重入与高效释放。
- GPT.c在Transformer框架下完成GPT‑2 124M模型的Token序列文本补全，实现并行优化推理。
- Libco基于ucontext API实现轻量级用户态协程，支持co_start启动与co_yield切换多执行流。
- Fsrecov从快速格式化的FAT32系统镜像中恢复 BMP 图片，实现完成的图像恢复。
