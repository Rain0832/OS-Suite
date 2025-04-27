#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <dlfcn.h>
#include <sys/wait.h>

// 表达式名 计数器
static int expr_counter = 0;

int exe_cmd(char *input)
{
    char *cmd[128];
    int tk_cnt = 0;

    char *tk = strtok(input, " ");
    while (tk != NULL && tk_cnt < 32)
    {
        cmd[tk_cnt++] = tk;
        tk = strtok(NULL, " ");
    }
    cmd[tk_cnt] = NULL; // 添加终止符

    pid_t pid = fork();

    if (pid == -1)
    {
        perror("fork");
        return 1;
    }
    else if (pid == 0)
    {
        // 子进程
        if (execvp("gcc", cmd) == -1)
        {
            return -1;
        }
    }
    else
    {
        // 父进程
        int status;
        if (waitpid(pid, &status, 0) == -1)
        {
            return -1;
        }
        if (WIFEXITED(status))
        {
            int exit_code = WEXITSTATUS(status);
            if (exit_code != 0)
            {
                return -1;
            }
        }
    }

    return 0;
}

bool compile_and_load_function(const char *function_def)
{
    if (strncmp(function_def, "int", 3) != 0)
    {
        return false;
    }

    // 生成临时C文件
    char c_temp_path[] = "/tmp/crepl_XXXXXX.c";
    int c_fd = mkstemps(c_temp_path, 2);
    if (c_fd == -1)
    {
        return false;
    }
    int rc = write(c_fd, function_def, strlen(function_def));
    if (rc < 0)
    {
        return false;
    }

    close(c_fd);

    // 编译临时文件
    char input[128];
    snprintf(input, sizeof(input),
             "gcc -shared -fPIC -o /tmp/crepl_temp.so %s -w",
             c_temp_path);
    int status = exe_cmd(input);

    // 删除临时C文件
    unlink(c_temp_path);

    if (status == -1 || WEXITSTATUS(status) != 0)
    {
        return false;
    }

    // 将函数定义添加到汇总文件
    FILE *summary_file = fopen("/tmp/crepl_summary.c", "a");
    if (!summary_file)
    {
        return false;
    }
    fprintf(summary_file, "%s\n", function_def);
    fclose(summary_file);

    // 编译汇总文件成共享库
    snprintf(input, sizeof(input),
             "gcc -shared -fPIC -o /tmp/crepl.so /tmp/crepl_summary.c -w -Wredundant-decls");
    status = exe_cmd(input);

    if (status == -1 || WEXITSTATUS(status) != 0)
    {
        return false;
    }

    return true;
}

// Evaluate an expression
bool evaluate_expression(const char *expression, int *result)
{
    expr_counter++; // 计数器递增
    char func_name[64];
    snprintf(func_name, sizeof(func_name), "__expr_wrapper_%d", expr_counter); // 生成唯一函数名

    // 构造带唯一函数名的代码
    char function_def[1024];
    snprintf(function_def, sizeof(function_def),
             "int %s() { return %s; }", func_name, expression);

    // 生成临时C文件
    char c_temp_path[] = "/tmp/crepl_expr_XXXXXX.c";
    int c_fd = mkstemps(c_temp_path, 2);
    if (c_fd == -1)
    {
        return false;
    }
    int rc = write(c_fd, function_def, strlen(function_def));
    if (rc < 0)
    {
        return false;
    }

    close(c_fd);

    // 生成临时共享库路径
    char so_temp_path[] = "/tmp/crepl_expr_XXXXXX.so";
    int so_fd = mkstemps(so_temp_path, 3);
    close(so_fd);

    // 构造编译命令
    char input[128];

    snprintf(input, sizeof(input),
             "gcc -shared -fPIC -o %s %s /tmp/crepl.so -w",
             so_temp_path, c_temp_path);

    int status = exe_cmd(input);

    // 删除临时C文件
    unlink(c_temp_path);

    if (status == -1 || WEXITSTATUS(status) != 0)
    {
        unlink(so_temp_path);
        return false;
    }

    // 动态加载共享库
    void *handle = dlopen(so_temp_path, RTLD_LAZY);
    if (!handle)
    {
        unlink(so_temp_path);
        return false;
    }

    // 获取函数指针
    typedef int (*expr_func)();
    expr_func func = (expr_func)dlsym(handle, func_name);
    if (!func)
    {
        dlclose(handle);
        unlink(so_temp_path);
        return false;
    }

    // 调用函数获取结果
    *result = func();

    // 清理资源
    dlclose(handle);
    unlink(so_temp_path);
    return true;
}

int main()
{

    // 初始化汇总文件
    FILE *summary_file = fopen("/tmp/crepl_summary.c", "w");
    if (summary_file)
    {
        fclose(summary_file);
    }
    char inputf[128];
    strcpy(inputf, "gcc -shared -fPIC -o /tmp/crepl.so /tmp/crepl_summary.c -w");
    exe_cmd(inputf);

    char *line = NULL;

    while (true)
    {
        line = readline("crepl> ");
        // 退出循环
        if (!line)
        {
            printf("\nExiting...\n");
            break;
        }
        // 继续下一轮循环
        if (line[0] == '\0')
        {
            free(line);
            continue;
        }
        // 添加输入到历史记录（可选）
        add_history(line);

        // 处理输入
        bool flag = false;
        if (compile_and_load_function(line))
        {
            flag = true;
        }
        else
        {
            int result = 0;
            if (evaluate_expression(line, &result))
            {
                flag = true;
                printf("Result: %d\n", result);
            }
        }
        if (!flag)
        {
            printf("error\n");
        }
        free(line); // 释放 readline 分配的内存
    }

    // 删除汇总文件和共享库
    unlink("/tmp/crepl_summary.c");
    unlink("/tmp/crepl.so");

    return 0;
}