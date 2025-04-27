#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <getopt.h>
#include "labyrinth.h"

// 定义唯一的整数值来标识长选项
#define LONG_OPTION_ONLY 100

int main(int argc, char *argv[])
{

    // 动态分配内存
    Labyrinth *labyrinth = (Labyrinth *)malloc(sizeof(Labyrinth));
    if (labyrinth == NULL)
    {
        // 处理内存分配失败的情况
        return -1;
    }
    // 初始化为0
    memset(labyrinth, 0, sizeof(Labyrinth));

    // printUsage();

    // 玩家 id
    int playerId = -1;

    // 地图路径
    char *mapName = malloc(256); // 分配足够的内存
    if (mapName == NULL)
    {
        perror("Failed to allocate memory");
        free(labyrinth);
        return -1;
    }

    int opt;
    int long_index = 0;
    static struct option long_options[] = {
        {"map", required_argument, 0, 'm'},
        {"player", required_argument, 0, 'p'},
        {"move", required_argument, 0, LONG_OPTION_ONLY},
        {"version", no_argument, 0, 'v'},
        {0, 0, 0, 0}};

    while ((opt = getopt_long(argc, argv, "m:p:v", long_options, &long_index)) != -1)
    {
        switch (opt)
        {
        case 'm':
            // 处理 --map 选项
            {
                bool rv = loadMap(labyrinth, optarg);
                if (rv == true)
                {
                    // 保存地图路径
                    strcpy(mapName, optarg);
                    printf("mapname: %s\n", mapName);
                    // 打印地图
                    for (int row = 0; row < labyrinth->rows; row++)
                    {
                        printf("%s\n", labyrinth->map[row]);
                    }
                }
                else if (rv == false)
                {
                    printf("file path incorrect\n");
                    free(labyrinth);
                    free(mapName);
                    return 1;
                }
            }
            break;
        case 'p':
            // 处理 --player 选项
            {
                // 确保参数是单字符
                if (strlen(optarg) == 1)
                {
                    if (isValidPlayer(optarg[0]))
                    {
                        printf("valid player ID\n");
                        playerId = optarg[0];
                    }
                    else
                    {
                        printf("invalid player ID\n");
                        free(labyrinth);
                        free(mapName);
                        return 1;
                    }
                }
                else
                {
                    printf("Invalid player ID format. Expected a single character.\n");
                    free(labyrinth);
                    free(mapName);
                    return 1;
                }
            }
            break;
        case LONG_OPTION_ONLY:
            // 处理 --move 选项
            {
                if (playerId == -1)
                {
                    printf("Player ID not specified. Please use --player option.\n");
                    free(labyrinth);
                    free(mapName);
                    return 1;
                }
                if (movePlayer(labyrinth, playerId, optarg))
                {
                    printf("move successfully\n");
                    saveMap(labyrinth, mapName);
                }
                else
                {
                    printf("move failed\n");
                    free(labyrinth);
                    free(mapName);
                    return 1;
                }
            }
            break;
        case 'v':
            // 处理 --version 选项
            {
                if (argc == 2)
                {
                    printf("----Labyrinth Game----\n");
                    printf("- Coding by Rain0832 since 20250402.\n");
                    printf("- this is the version v2.0 - update at 20250406\n");
                }
                else
                {
                    printf("except only --version and not have other arg...\n");
                    free(labyrinth);
                    free(mapName);
                    return 1;
                }
            }
            break;
        default:
            printf("Invalid option detected, exiting with status 1\n");
            printUsage();
            free(labyrinth);
            free(mapName);
            return 1;
        }
    }

    // 释放内存
    free(labyrinth);
    free(mapName);

    // 检查剩余的非选项参数
    for (int i = optind; i < argc; i++)
    {
        fprintf(stderr, "Invalid argument: %s\n", argv[i]);
        return 1;
    }
    return 0;
}

void printUsage(void)
{
    printf("Usage:\n");
    printf("  labyrinth --map map.txt --player id\n");
    printf("  labyrinth -m map.txt -p id\n");
    printf("  labyrinth --map map.txt --player id --move direction\n");
    printf("  labyrinth --version\n");
}

/*
    function: 验证 playerId 是否合法
        1. 0 ~ 9 之间 返回 true
        2. 否则 返回 false
*/
bool isValidPlayer(char playerId)
{
    if (playerId >= '0' && playerId <= '9')
        return true;

    return false;
}

/*
    function: 从文件中加载地图
        1. 验证文件名是否合法
        2. 读取文件内容 到 labyrinth 的 map 中
        3. 加载失败 返回 false
        4. 否则 返回 true
*/
bool loadMap(Labyrinth *labyrinth, const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        perror("Failed to open map file");
        return false;
    }
    else
    {
        // labyrinth should be init with 0......
        while (fgets(labyrinth->map[labyrinth->rows], MAX_COLS, fp) != NULL)
        {
            // 换行符 替换为 '\0'
            labyrinth->map[labyrinth->rows][strcspn(labyrinth->map[labyrinth->rows], "\n")] = '\0';
            labyrinth->rows++;
        }
        labyrinth->cols = strlen(labyrinth->map[0]);
        fclose(fp);

        // 地图过大
        if (labyrinth->rows > 100 || labyrinth->cols > 100)
        {
            perror("map to large\n");
            return false;
        }
        // 地图未连同
        if (!isConnected(labyrinth))
        {
            perror("map isn't connect\n");
            return false;
        }
    }
    return true;
}

/*
    function: 找到地图中的 Player
        1. 找到地图中的 数字 代表 palyerId
        2. 返回坐标
*/
Position findPlayer(Labyrinth *labyrinth, char playerId)
{
    Position pos = {-1, -1};
    for (int row = 0; row < labyrinth->rows; row++)
    {
        for (int col = 0; col < labyrinth->cols; col++)
        {
            if (labyrinth->map[row][col] == playerId)
            {
                pos.row = row;
                pos.col = col;
            }
        }
    }
    return pos;
}

/*
    function: 找到地图上第一个 空地
        1. 从上到下，从左到右遍历
        2. 遇到的 第一个 '.'
        3. 返回对应坐标

 */
Position findFirstEmptySpace(Labyrinth *labyrinth)
{
    Position pos = {-1, -1};
    for (int row = 0; row < labyrinth->rows; row++)
    {
        for (int col = 0; col < labyrinth->cols; col++)
        {
            if (isEmptySpace(labyrinth, row, col))
            {
                pos.row = row;
                pos.col = col;
                return pos;
            }
        }
    }
    return pos;
}

/*
    function: 判断某个点是不是空地
        1. if 判断是不是 '.'
        2. 玩家视为空地
 */
bool isEmptySpace(Labyrinth *labyrinth, int row, int col)
{
    // Check boundary conditions
    if (row < 0 || row >= labyrinth->rows || col < 0 || col >= labyrinth->cols)
    {
        return false; // Out of bounds
    }

    // Check if the cell is an empty space or a valid player
    return labyrinth->map[row][col] == '.' || isValidPlayer(labyrinth->map[row][col]);
}

/*
    function: 玩家的移动
        1. 根据移动方向来绝对偏移位置
        2. 推断玩家的下一个位置
        3. 在 labryinth 中 map 进行修改玩家位置
*/
bool movePlayer(Labyrinth *labyrinth, char playerId, const char *direction)
{
    Position pos = findPlayer(labyrinth, playerId);
    if (pos.row == -1 && pos.col == -1)
    {
        // 地图中没有该玩家记录，将玩家放置在第一个空地
        Position first_empty = findFirstEmptySpace(labyrinth);
        if (first_empty.row != -1 && first_empty.col != -1)
        {
            labyrinth->map[first_empty.row][first_empty.col] = playerId;
            pos = first_empty;
        }
        else
        {
            return false;
        }
    }

    Position next_pos = {pos.row, pos.col};
    if (strcmp(direction, "up") == 0)
    {
        next_pos.row = pos.row - 1;
        next_pos.col = pos.col;
    }
    else if (strcmp(direction, "down") == 0)
    {
        next_pos.row = pos.row + 1;
        next_pos.col = pos.col;
    }
    else if (strcmp(direction, "left") == 0)
    {
        next_pos.row = pos.row;
        next_pos.col = pos.col - 1;
    }
    else if (strcmp(direction, "right") == 0)
    {
        next_pos.row = pos.row;
        next_pos.col = pos.col + 1;
    }
    else
    {
        // UNKNOWN
        return false;
    }

    if (isEmptySpace(labyrinth, next_pos.row, next_pos.col))
    {
        labyrinth->map[pos.row][pos.col] = '.';
        labyrinth->map[next_pos.row][next_pos.col] = playerId;

        return true;
    }

    return false;
}

bool saveMap(Labyrinth *labyrinth, const char *filename)
{
    // 检查输入参数是否有效
    if (labyrinth == NULL || filename == NULL)
    {
        fprintf(stderr, "Error: Invalid input parameters\n");
        return false;
    }

    // 打开文件
    FILE *fp = fopen(filename, "w");
    if (fp == NULL)
    {
        perror("Failed to open file");
        return false;
    }

    // 写入地图数据
    for (int row = 0; row < labyrinth->rows; row++)
    {
        if (fputs(labyrinth->map[row], fp) == EOF)
        {
            fprintf(stderr, "Error: Failed to write to file\n");
            fclose(fp);
            return false;
        }
        fputc('\n', fp);
    }

    // 关闭文件
    fclose(fp);
    return true;
}

// Check if all empty spaces are connected using DFS
void dfs(Labyrinth *labyrinth, int row, int col, bool visited[MAX_ROWS][MAX_COLS])
{
    if (row < 0 || row >= labyrinth->rows || col < 0 || col >= labyrinth->cols ||
        visited[row][col] || !isEmptySpace(labyrinth, row, col))
    {
        return;
    }

    visited[row][col] = true;

    // Recursively visit all adjacent cells (up, down, left, right)
    dfs(labyrinth, row + 1, col, visited); // Down
    dfs(labyrinth, row - 1, col, visited); // Up
    dfs(labyrinth, row, col + 1, visited); // Right
    dfs(labyrinth, row, col - 1, visited); // Left
}

bool isConnected(Labyrinth *labyrinth)
{
    Position first_pos = findFirstEmptySpace(labyrinth);

    // Check if there is no empty space in the labyrinth
    if (first_pos.row == -1 || first_pos.col == -1)
    {
        return true; // If there are no empty spaces, they are trivially connected
    }

    bool visited[MAX_ROWS][MAX_COLS] = {false};

    dfs(labyrinth, first_pos.row, first_pos.col, visited);
    for (int row = 0; row < labyrinth->rows; row++)
    {
        for (int col = 0; col < labyrinth->cols; col++)
        {
            if ((isEmptySpace(labyrinth, row, col)) && visited[row][col] == false)
            {
                return false;
            }
        }
    }
    return true;
}
