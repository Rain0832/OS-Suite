#define HASH_SIZE 1000
#define MAX_NODES 1000
#define MAX_NAME_LENGTH 256

typedef struct
{
  int pid;
  int ppid;
} HashNode;

typedef struct Node
{
  int pid;
  struct Node *children[MAX_NODES];
  int child_count;
} Node;

void traversePROC(void);
void printPID(void);
void printTree(Node *node, int depth, int is_last_child, int show_pid, int sort);
char *getName(int pid);
void printVersion(void);
void printUsage(void);