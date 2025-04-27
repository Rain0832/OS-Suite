#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <regex.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>

double cnt = 0.0;
#define MAX_SYSCALLS 1024 // 最大系统调用数
#define TOP_N 5           // 输出前TOP_N个系统调用
#define MAX_MATCHES 2     // 正则匹配结果数
#define INTERVAL_MS 1000  // 1000ms更新

// 一种系统调用的统计信息
typedef struct
{
    char name[64];
    double total_time;
    int count;
} syscall_stat;

// 所有系统调用统计信息
typedef struct
{
    syscall_stat stats[MAX_SYSCALLS];
    int count;
    double total_time;
} syscall_stats;

syscall_stats stats;

// 正则表达式
// 系统调用名称匹配     时间匹配
regex_t name_regex, time_regex;

/**
 * @brief 初始化 编译正则表达式
 * @param void
 * @return 0 on success, -1 on failure
 * @note 编译后的表达式赋值给 name_regex 和 time_regex
 *       编译失败时会打印错误信息并返回-1
 */
int regex_init(void)
{
    // 系统调用名称匹配：以字母开头，后跟左括号
    // 时间匹配：以小数点或数字开头，后跟小数点或数字结尾
    if (regcomp(&name_regex, "^([a-zA-Z]+)\\(", REG_EXTENDED) != 0 ||
        regcomp(&time_regex, "<([0-9.]+)>", REG_EXTENDED) != 0)
    {
        fprintf(stderr, "Regex compilation failed\n");
        return -1;
    }
    return 0;
}

/**
 * @brief 清理正则表达式资源
 * @param void
 * @return void
 * @note none
 */
void regex_cleanup(void)
{
    regfree(&name_regex);
    regfree(&time_regex);
}

/**
 * @brief 解析strace输出行
 * @param line 待解析的strace输出行
 * @param stats 系统调用统计信息 全局变量
 * @return void
 * @note 解析成功时会更新stats
 */
void parse_strace_line(const char *line, syscall_stats *stats)
{
    regmatch_t name_matches[MAX_MATCHES], time_matches[MAX_MATCHES];
    char name[64] = {0};
    double time = 0.0;

    // 提取系统调用名称
    if (regexec(&name_regex, line, MAX_MATCHES, name_matches, 0) != 0)
        return;
    size_t len = name_matches[1].rm_eo - name_matches[1].rm_so;
    if (len >= sizeof(name))
        return;
    strncpy(name, line + name_matches[1].rm_so, len);
    name[len] = '\0';

    // 提取时间
    if (regexec(&time_regex, line, MAX_MATCHES, time_matches, 0) != 0)
        return;
    len = time_matches[1].rm_eo - time_matches[1].rm_so;
    char time_str[16] = {0};
    if (len >= sizeof(time_str))
        return;
    strncpy(time_str, line + time_matches[1].rm_so, len);
    time = atof(time_str);

    // 合并系统调用
    for (int i = 0; i < stats->count; i++)
    {
        if (strcmp(stats->stats[i].name, name) == 0)
        {
            stats->stats[i].total_time += time;
            stats->stats[i].count++;
            stats->total_time += time;
            return;
        }
    }

    // 新增系统调用
    if (stats->count < MAX_SYSCALLS)
    {
        strcpy(stats->stats[stats->count].name, name);
        stats->stats[stats->count].total_time = time;
        stats->stats[stats->count].count = 1;
        stats->total_time += time;
        stats->count++;
    }
}

int cmp(const void *a, const void *b)
{
    const syscall_stat *stat1 = (const syscall_stat *)a;
    const syscall_stat *stat2 = (const syscall_stat *)b;
    return stat1->total_time < stat2->total_time;
}

/**
 * @brief 打印系统调用统计信息
 * @param stats 系统调用统计信息
 * @return void
 * @note 按时间排序并输出前TOP_N个系统调用的统计信息
 */
void print_top_syscalls(syscall_stats *stats)
{
    // 如果没有数据 不输出
    if (stats->total_time == 0)
        return;

    // 按耗时排序
    qsort(stats->stats, stats->count, sizeof(syscall_stat), cmp);

    // 清屏
    int rtc = system("clear"); // Linux/Unix 系统

    // 检查命令是否成功执行
    if (rtc != 0)
    {
        fprintf(stderr, "Failed to clear the screen\n");
    }

    printf("Time: %.2lfs\n", cnt);
    // 输出前TOP_N个
    for (int i = 0; i < TOP_N && i < stats->count; i++)
    {
        int ratio = (int)((stats->stats[i].total_time / stats->total_time) * 100);
        printf("%s (%d%%)\n", stats->stats[i].name, ratio);
    }
    printf("=====================\n");
    cnt += 0.1;

    // 输出80个\0分隔符并刷新缓冲区
    for (int i = 0; i < 80; i++)
        putchar('\0');

    fflush(stdout);
}

/**
 * @brief 定时器信号处理函数
 * @param signum 信号编号
 * @return void
 * @note 打印系统调用统计信息并刷新缓冲区
 */
void signal_handler(int signum)
{
    print_top_syscalls(&stats);
}

/**
 * @brief 设置定时器
 * @param void
 * @return void
 * @note 设定定时器信号处理函数并启动定时器
 */
void setup_timer(void)
{
    struct itimerval itv;
    itv.it_interval.tv_sec = INTERVAL_MS / 1000;           // 秒部分
    itv.it_interval.tv_usec = (INTERVAL_MS % 1000) * 1000; // 微秒部分（100ms=100000us）
    itv.it_value = itv.it_interval;                        // 首次触发间隔与重复间隔相同

    struct sigaction sa;
    sa.sa_handler = signal_handler; // 设置信号处理函数
    sigemptyset(&sa.sa_mask);       // 清空信号掩码
    sa.sa_flags = 0;

    if (sigaction(SIGALRM, &sa, NULL) == -1 ||
        setitimer(ITIMER_REAL, &itv, NULL) == -1)
    {
        perror("Timer setup failed");
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    // 命令行参数检查
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        return 1;
    }

    // 初始化正则表达式
    if (regex_init() != 0)
    {
        return 1;
    }

    // 初始化系统调用统计信息
    memset(&stats, 0, sizeof(stats));

    // 创建管道
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        return 1;
    }

    // fork() 系统调用
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork");
        return 1;
    }

    // 子进程：运行strace并重定向输出（同原实现，增强路径兼容性）
    if (pid == 0)
    {
        close(pipefd[0]);               // 关闭管道读端
        dup2(pipefd[1], STDOUT_FILENO); // 标准输出重定向到管道写端
        dup2(pipefd[1], STDERR_FILENO); // 标准错误重定向到管道写端
        close(pipefd[1]);               // 关闭管道写端

        // 自动搜索strace路径
        char *strace_paths[] = {"/usr/bin/strace", "/bin/strace", NULL};

        // 命令行参数转变为 execve 的参数...
        char **exec_argv = malloc((argc + 2) * sizeof(char *));
        exec_argv[0] = "strace";
        exec_argv[1] = "-T";
        for (int i = 1; i < argc; i++)
        {
            exec_argv[i + 1] = argv[i];
        }
        exec_argv[argc + 1] = NULL;

        // 寻找 strace 命令路径
        char *exec_envp[] = {"PATH=/bin:/usr/bin", NULL};
        int found = 0;
        for (int i = 0; strace_paths[i]; i++)
        {
            if (execve(strace_paths[i], exec_argv, exec_envp) == 0)
            {
                found = 1;
                break;
            }
        }
        if (!found)
        {
            perror("execve strace");
            free(exec_argv);
            return 1;
        }
        free(exec_argv);
        return 0;
    }

    // 父进程：设置定时器并读取管道数据
    close(pipefd[1]);
    setup_timer();

    char buffer[8192]; // 增大缓冲区减少read次数
    while (1)
    {
        // 读取管道数据
        ssize_t n = read(pipefd[0], buffer, sizeof(buffer));
        if (n <= 0)
            break;

        // 手动分割行
        char *start = buffer;
        char *end = buffer;
        while (end < buffer + n)
        {
            if (*end == '\n')
            {
                *end = '\0';
                // 解析数据 更新 stats 表
                parse_strace_line(start, &stats);
                start = end + 1;
            }
            end++;
        }
        // 处理最后一行（无换行符）
        if (start < buffer + n)
        {
            parse_strace_line(start, &stats);
        }
    }

    close(pipefd[0]);           // 关闭管道读端
    wait(NULL);                 // 等待子进程结束
    print_top_syscalls(&stats); // 打印信息
    regex_cleanup();            // 释放正则资源
    return 0;
}