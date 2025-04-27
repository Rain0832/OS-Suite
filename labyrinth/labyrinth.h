#define MAX_ROWS 100
#define MAX_COLS 100
#define VERSION_INFO "Labyrinth Game"

typedef struct
{
    char map[MAX_ROWS][MAX_COLS]; // map 数组 从 0 开始
    int rows;                     // 行数 从 1 开始
    int cols;                     // 列数 从 1 开始
} Labyrinth;

typedef struct
{
    int row;
    int col;
} Position;

int main(int argc, char *argv[]);
void printUsage(void);
bool isValidPlayer(char playerId);
bool loadMap(Labyrinth *labyrinth, const char *filename);
Position findPlayer(Labyrinth *labyrinth, char playerId);
Position findFirstEmptySpace(Labyrinth *labyrinth);
bool isEmptySpace(Labyrinth *labyrinth, int row, int col);
bool movePlayer(Labyrinth *labyrinth, char playerId, const char *direction);
bool saveMap(Labyrinth *labyrinth, const char *filename);
bool isConnected(Labyrinth *labyrinth);
