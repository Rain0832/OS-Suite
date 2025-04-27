#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include "pstree.h"

HashNode hashTable[HASH_SIZE];

Node *createNode(int pid)
{
  Node *node = (Node *)malloc(sizeof(Node));
  node->pid = pid;
  node->child_count = 0;
  for (int i = 0; i < MAX_NODES; i++)
  {
    node->children[i] = NULL;
  }
  return node;
}

int hashFunction(int key)
{
  return key % HASH_SIZE;
}

void insertHash(int pid, int ppid)
{
  int index = hashFunction(pid);
  hashTable[index].pid = pid;
  hashTable[index].ppid = ppid;
}

// 添加子节点
void addChild(Node *parent, Node *child)
{
  parent->children[parent->child_count++] = child;
}

// 查找节点
Node *findNode(Node *nodes[], int pid, int node_count)
{
  for (int i = 0; i < node_count; i++)
  {
    if (nodes[i]->pid == pid)
    {
      return nodes[i];
    }
  }
  return NULL;
}

// 比较函数，用于 qsort
int compareNodes(const void *a, const void *b)
{
  Node *node_a = *(Node **)a;
  Node *node_b = *(Node **)b;
  return node_a->pid - node_b->pid;
}

// 递归打印树
void printTree(Node *node, int depth, int is_last_child, int show_pid, int sort)
{
  //
  for (int i = 0; i < depth - 1; i++)
  {
    printf("  ");
  }
  if (depth > 0)
  {
    if (is_last_child)
    {
      printf("└─");
    }
    else
    {
      printf("├─");
    }
  }

  char *name = getName(node->pid);
  if (name != NULL)
  {
    printf("%s", name);
  }
  if (show_pid)
  {
    printf("(%d)", node->pid);
  }

  // 处理重复进程
  int count = 0;
  for (int i = 0; i < node->child_count; i++)
  {
    if (node->children[i]->pid == node->pid)
    {
      count++;
    }
  }
  if (count > 0)
  {
    printf("─%d*[Process]", count + 1);
    if (show_pid)
    {
      printf("(%d)", node->pid);
    }
    for (int i = 0; i < node->child_count; i++)
    {
      if (node->children[i]->pid != node->pid)
      {
        printf("\n");
        printTree(node->children[i], depth + 1, i == node->child_count - 1, show_pid, sort);
      }
    }
  }
  else
  {
    // Test
    if (sort)
    {
      qsort(node->children, node->child_count, sizeof(Node *), compareNodes);
    }
    for (int i = 0; i < node->child_count; i++)
    {
      printf("\n");
      printTree(node->children[i], depth + 1, i == node->child_count - 1, show_pid, sort);
    }
  }
}

int main(int argc, char *argv[])
{
  int show_pid = 0;
  int sort = 0;
  int opt;
  int long_index = 0;
  static struct option long_options[] = {
      {"show-pids", no_argument, 0, 'p'},
      {"numeric-sort", no_argument, 0, 'n'},
      {"version", no_argument, 0, 'V'},
      {0, 0, 0, 0}};

  while ((opt = getopt_long(argc, argv, "pnV", long_options, &long_index)) != -1)
  {
    switch (opt)
    {
    case 'p':
      // --show-pids
      // 打印每个进程的进程号
      {
        show_pid = 1;
      }
      break;
    case 'n':
      // --numeric-sort
      // 按照 pid 的数值从小到大顺序输出一个进程的直接孩子
      {
        sort = 1;
      }
      break;
    case 'V':
      // --version
      // 打印版本信息
      {
        if (argc == 2)
        {
          printVersion();
        }
        else
        {
          printf("except only --version and not have other arg...\n");
          return 1;
        }
      }
      break;
    default:
      // 未知选项
      printUsage();
      return 1;
    }
  }

  Node *nodes[MAX_NODES];
  int node_count = 0;

  traversePROC();
  for (int i = 0; i < HASH_SIZE; i++)
  {
    if (hashTable[i].pid != 0)
    {

      int ppid = hashTable[i].ppid;
      int pid = hashTable[i].pid;
      Node *parent = findNode(nodes, ppid, node_count);
      Node *child = findNode(nodes, pid, node_count);

      if (!parent)
      {
        parent = createNode(ppid);
        nodes[node_count++] = parent;
      }
      if (!child)
      {
        child = createNode(pid);
        nodes[node_count++] = child;
      }
      addChild(parent, child);
    }
  }

  Node *root = NULL;
  for (int i = 0; i < node_count; i++)
  {
    int is_root = 1;
    for (int j = 0; j < node_count; j++)
    {
      for (int k = 0; k < nodes[j]->child_count; k++)
      {
        if (nodes[j]->children[k] == nodes[i])
        {
          is_root = 0;
          break;
        }
      }
      if (!is_root)
        break;
    }
    if (is_root)
    {
      root = nodes[i];
      break;
    }
  }

  // 打印树
  if (root)
  {
    printTree(root, 0, 1, show_pid, sort);
    printf("\n");
  }

  // 释放内存
  for (int i = 0; i < node_count; i++)
  {
    free(nodes[i]);
  }

  return 0;
}

int getPPID(int pid)
{
  char filename[64];
  sprintf(filename, "/proc/%d/stat", pid);

  int fd = open(filename, O_RDONLY);
  if (fd < 0)
  {
    perror("open");
    return -1;
  }

  char buffer[256];
  ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
  if (bytes_read < 0)
  {
    perror("read");
    close(fd);
    return -1;
  }
  buffer[bytes_read] = '\0';

  close(fd);

  int ppid;
  if (sscanf(buffer, "%*d %*s %*c %d", &ppid) != 1)
  {
    fprintf(stderr, "Failed to parse /proc/%d/stat\n", pid);
    return -1;
  }

  return ppid;
}

char *getName(int pid)
{
  char filename[64];
  sprintf(filename, "/proc/%d/stat", pid);

  int fd = open(filename, O_RDONLY);
  if (fd < 0)
  {
    perror("open");
    return strdup("unknown");
  }

  char buffer[256];
  ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
  if (bytes_read < 0)
  {
    perror("read");
    close(fd);
    return strdup("unknown");
  }
  buffer[bytes_read] = '\0';

  close(fd);

  char *name = (char *)malloc(MAX_NAME_LENGTH);
  if (name == NULL)
  {
    perror("malloc");
    return strdup("unknown");
  }

  if (sscanf(buffer, "%*d (%255[^)])", name) != 1)
  {
    fprintf(stderr, "Failed to parse /proc/%d/stat\n", pid);
    free(name);
    return strdup("unknown");
  }

  return name;
}

bool isNumber(const char *str)
{
  while (*str)
  {
    if (!isdigit((unsigned char)*str))
    {
      return false;
    }
    str++;
  }
  return true;
}

void traversePROC(void)
{
  DIR *dir;
  struct dirent *entry;

  dir = opendir("/proc");
  if (dir == NULL)
  {
    perror("opendir");
    return;
  }

  while ((entry = readdir(dir)) != NULL)
  {
    if (isNumber(entry->d_name))
    {
      // 进程号
      int pid = atoi(entry->d_name);
      int ppid = getPPID(pid);
      if (ppid != -1)
      {
        insertHash(pid, ppid);
      }
    }
  }
  closedir(dir);
}

int compareInt(const void *a, const void *b)
{
  int int_a = *(const int *)a;
  int int_b = *(const int *)b;

  if (int_a < int_b)
    return -1;
  if (int_a > int_b)
    return 1;
  return 0;
}

void printPID(void)
{
  traversePROC();
  int array[HASH_SIZE] = {0};
  for (int i = 0; i < HASH_SIZE; i++)
  {
    if (hashTable[i].pid != 0)
    {
      array[i] = hashTable[i].pid;
    }
  }
  qsort(array, HASH_SIZE, sizeof(int), compareInt);
  for (int i = 0; i < HASH_SIZE; i++)
  {
    if (array[i] != 0)
    {
      printf("%d\n", array[i]);
    }
  }
}

void printVersion(void)
{
  printf("\n");
  printf("----PsTree----\n");
  printf("- Coding by Rain0832 since 20250409.\n");
  printf("- This is the version v2.0 - update at 20250410\n");
  printf("\n");
}

void printUsage(void)
{
  printf("\n");
  printf("Usage:\n");
  printf("pstree [-p] [-n] [-V]\n");
  printf("-p or --show-pids: print each process's pid\n");
  printf("-n or --numeric-sort: sort processes by pid in ascending order\n");
  printf("-V or --version: print version information\n");
  printf("\n");
}