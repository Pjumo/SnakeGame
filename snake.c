#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define MAP_SIZE 20
#define MAIN_TEXT_START_Y 4
#define MAIN_TEXT_START_X 8
#define SNAKE_INIT_LENGTH 4
#define SNAKE_INIT_Y 10
#define SNAKE_INIT_X 7
#define FOOD_INIT_Y 10
#define FOOD_INIT_X 16
#define SCORE_TEXT_Y 0
#define SCORE_TEXT_X 2
#define GAME_PERIOD 150
#define AUTO_MODE_PERIOD 50
#define TITLE_PERIOD 200

#define UNICODE_FULL_BLOCK "██"
#define UNICODE_HALF_BLOCK "█"
#define UNICODE_BLOCK "■"
#define UNICODE_2BLANK "　"
#define UNICODE_SELECT "➜"
#define UNICODE_BLUR_BLOCK "▓▓"
#define UNICODE_BLUR_PLUS_BLOCK "░░"
#define UNICODE_HEART "❤"

// Redefine KEY
#define KEY_UP 259
#define KEY_DOWN 258
#define KEY_LEFT 260
#define KEY_RIGHT 261
#define KEY_ENTER 10
#define KEY_ESC 27
#define KEY_A 97

enum ColorType {
    BLACK,    // 0
    RED,      // 1
    GREEN,    // 2
    YELLOW,   // 3
    BLUE,     // 4
    CYAN,     // 5
    MAGENTA,  // 6
    WHITE     // 7
};

enum DisplayMode { MAIN, PLAYGAME, ENDGAME };

enum Direction { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };

struct Food {
    int y;
    int x;  // *2
};

struct SnakeNode {
    int y;
    int x;  // *2
    struct SnakeNode* front;
};

struct Snake {
    int length;
    struct SnakeNode* head;
    enum Direction dir;
};

const int mainText[5][24] = {{1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 1},
                             {1, 0, 0, 0, 1, 1, 2, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0},
                             {1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1},
                             {0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0},
                             {1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1}};

WINDOW* win = NULL;
int autoMode = 0;

// Draw
void drawMainPage();
void drawMenu(int selected);
void drawGame(struct Snake* snake, struct Food* food, int* score);

// Snake
void initSnake(struct Snake* snake);
void moveSnake(struct Snake* snake, int grow);
void inputHandler(struct Snake* snake, int* ch);
void addHead(struct Snake* snake, int y, int x);
void removeTail(struct Snake* snake);
void freeSnake(struct Snake* snake);

// Food
void initFood(struct Food* food);
void spawnFood(struct Snake* snake, struct Food* food);

// Check
int checkCollision(struct Snake* snake);
int checkFood(struct Snake* snake, struct Food* food);

// Auto Mode
void buildMap(int map[MAP_SIZE][MAP_SIZE], struct Snake* snake);
int bfs(int map[MAP_SIZE][MAP_SIZE], int sy, int sx, int fy, int fx, int parent[MAP_SIZE][MAP_SIZE][2]);
enum Direction getDir(int parent[MAP_SIZE][MAP_SIZE][2], int sy, int sx, int fy, int fx);
void autoMove(struct Snake* snake, struct Food* food);

// Auto Mode V2
int isSafeToFood(struct Snake* snake, struct Food* food);
void simulateMove(struct Snake* snake, int parent[MAP_SIZE][MAP_SIZE][2], int fy, int fx);
struct Snake* copySnake(struct Snake* snake);

int main() {
    setlocale(LC_ALL, "");
    initscr();
    curs_set(0);            // hide cursor
    noecho();               // hide input
    keypad(stdscr, TRUE);   // activate special key
    nodelay(stdscr, TRUE);  // activate non-blocking input
    start_color();

    // Init Default Color
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_BLUE, COLOR_BLACK);
    init_pair(5, COLOR_CYAN, COLOR_BLACK);
    init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(7, COLOR_WHITE, COLOR_BLACK);

    // Window Size 30x60 | Start Point (10,20)
    // real x coordinate is (x in this program)*2
    win = newwin(MAP_SIZE, MAP_SIZE * 2, 10, 20);
    refresh();
    wrefresh(win);

    enum DisplayMode displayMode = MAIN;
    int ch, score = 0;
    int isInit = 1, exit = 0, selected = 0;
    struct Snake snake;
    struct Food food;

    while (1) {
        ch = getch();
        switch (displayMode) {
            case MAIN:
                if (isInit) {
                    werase(win);
                    box(win, 0, 0);
                    drawMainPage();
                    isInit = 0;
                }
                drawMenu(selected);

                switch (ch) {
                    case KEY_ENTER:
                        if (selected) {
                            exit = 1;
                        } else {
                            displayMode = PLAYGAME;
                            isInit = 1;
                        }
                        break;
                    case KEY_UP:
                        selected = 0;
                        break;
                    case KEY_DOWN:
                        selected = 1;
                        break;
                }
                break;
            case PLAYGAME:
                if (isInit) {
                    initSnake(&snake);
                    initFood(&food);
                    srand(time(NULL));  // Init Random Seed
                    score = 0;
                    autoMode = 0;
                    isInit = 0;
                }

                // Auto Mode Check
                if (autoMode) {
                    autoMove(&snake, &food);
                } else {
                    inputHandler(&snake, &ch);
                }

                // Main Logic
                moveSnake(&snake, 0);
                int grow = checkFood(&snake, &food);
                if (grow) {
                    score++;
                    spawnFood(&snake, &food);
                } else {
                    removeTail(&snake);
                }
                if (checkCollision(&snake)) {
                    displayMode = ENDGAME;
                    break;
                }
                drawGame(&snake, &food, &score);

                if (ch == KEY_ESC) {
                    displayMode = ENDGAME;
                    break;
                }

                // AutoMode Toggle
                if (ch == KEY_A) {
                    autoMode = !autoMode;
                }

                napms(autoMode ? AUTO_MODE_PERIOD : GAME_PERIOD);
                break;
            case ENDGAME:
                werase(win);
                box(win, 0, 0);
                mvwprintw(win, MAP_SIZE / 2 - 1, MAP_SIZE - 5, "GAME OVER");
                mvwprintw(win, MAP_SIZE / 2 + 1, MAP_SIZE - 6, "Press Enter");
                wrefresh(win);

                if (ch == KEY_ENTER) {
                    freeSnake(&snake);
                    isInit = 1;
                    displayMode = MAIN;
                }
                break;
        }
        if (exit) break;
    }

    endwin();

    return 0;
}

void drawMainPage() {
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 24; j++) {
            int textColor;
            if (j < 4)
                textColor = RED;
            else if (j < 10)
                textColor = MAGENTA;
            else if (j < 16)
                textColor = YELLOW;
            else if (j < 21)
                textColor = GREEN;
            else
                textColor = BLUE;

            wattron(win, COLOR_PAIR(textColor));
            if (mainText[i][j] == 1) {
                mvwaddstr(win, MAIN_TEXT_START_Y + i, MAIN_TEXT_START_X + j, UNICODE_HALF_BLOCK);
            } else {
                mvwaddstr(win, MAIN_TEXT_START_Y + i, MAIN_TEXT_START_X + j, " ");
            }
            wattroff(win, COLOR_PAIR(textColor));
        }
        napms(TITLE_PERIOD);  // 200ms period
        wrefresh(win);
    }
}

void drawMenu(int selected) {
    int startY = MAIN_TEXT_START_Y + 8;
    int startX = MAIN_TEXT_START_X + 8;

    // GAME START
    mvwaddstr(win, startY, startX - 4, selected ? " " : UNICODE_SELECT);
    mvwaddstr(win, startY, startX, "GAME START");

    // EXIT
    mvwaddstr(win, startY + 2, startX - 4, selected ? UNICODE_SELECT : " ");
    mvwaddstr(win, startY + 2, startX, "EXIT");

    wrefresh(win);
}

void drawGame(struct Snake* snake, struct Food* food, int* score) {
    werase(win);
    box(win, 0, 0);

    // Draw Score
    mvwprintw(win, SCORE_TEXT_Y, SCORE_TEXT_X, " score : %d ", *score);

    if (autoMode) {
        mvwprintw(win, 0, 25, " Auto Mode ");
    }

    // Draw Food
    wattron(win, COLOR_PAIR(RED));
    mvwprintw(win, food->y, food->x * 2, UNICODE_HEART);
    wattroff(win, COLOR_PAIR(RED));

    // Draw Snake
    struct SnakeNode* s = snake->head;
    wattron(win, COLOR_PAIR(GREEN));
    wattron(win, A_BOLD);
    mvwprintw(win, s->y, (s->x) * 2, UNICODE_FULL_BLOCK);
    wattroff(win, A_BOLD);
    while (1) {
        s = s->front;
        if(s->front == NULL){
            mvwprintw(win, s->y, (s->x) * 2, UNICODE_BLUR_PLUS_BLOCK);
            break;
        }
        mvwprintw(win, s->y, (s->x) * 2, UNICODE_BLUR_BLOCK);
    }
    wattroff(win, COLOR_PAIR(GREEN));
    wrefresh(win);
}

void initSnake(struct Snake* snake) {
    snake->length = SNAKE_INIT_LENGTH;
    snake->dir = DIR_RIGHT;  // Init Direction

    struct SnakeNode* s = (struct SnakeNode*)malloc(sizeof(struct SnakeNode));
    snake->head = s;

    // Init First Body
    for (int i = 0; i < SNAKE_INIT_LENGTH; i++) {
        s->y = SNAKE_INIT_Y;
        s->x = SNAKE_INIT_X - i;
        s->front = (i == SNAKE_INIT_LENGTH - 1) ? NULL : (struct SnakeNode*)malloc(sizeof(struct SnakeNode));
        s = s->front;
    }
}

void freeSnake(struct Snake* snake) {
    struct SnakeNode* cur = snake->head;

    while (cur != NULL) {
        struct SnakeNode* temp = cur;
        cur = cur->front;
        free(temp);
    }

    snake->head = NULL;
    snake->length = 0;
}

void moveSnake(struct Snake* snake, int grow) {
    int newY = snake->head->y;
    int newX = snake->head->x;

    switch (snake->dir) {
        case DIR_UP:
            newY--;
            break;
        case DIR_DOWN:
            newY++;
            break;
        case DIR_LEFT:
            newX--;
            break;
        case DIR_RIGHT:
            newX++;
            break;
    }

    addHead(snake, newY, newX);
}

void inputHandler(struct Snake* snake, int* ch) {
    printf("%d", *ch);
    switch (*ch) {
        case KEY_UP:
            if (snake->dir != DIR_DOWN) snake->dir = DIR_UP;
            break;
        case KEY_DOWN:
            if (snake->dir != DIR_UP) snake->dir = DIR_DOWN;
            break;
        case KEY_LEFT:
            if (snake->dir != DIR_RIGHT) snake->dir = DIR_LEFT;
            break;
        case KEY_RIGHT:
            if (snake->dir != DIR_LEFT) snake->dir = DIR_RIGHT;
            break;
    }
}

void addHead(struct Snake* snake, int y, int x) {
    struct SnakeNode* newNode = malloc(sizeof(struct SnakeNode));

    // Shift(Add) Head
    newNode->y = y;
    newNode->x = x;
    newNode->front = snake->head;

    snake->head = newNode;
    snake->length++;
}

void removeTail(struct Snake* snake) {
    // Exception
    if (snake->head == NULL) return;

    struct SnakeNode* cur = snake->head;

    // Find Tail
    while (cur->front->front != NULL) {
        cur = cur->front;
    }

    // Free Tail Memory
    free(cur->front);
    cur->front = NULL;

    snake->length--;
}

void initFood(struct Food* food) {
    food->y = FOOD_INIT_Y;
    food->x = FOOD_INIT_X;
}

void spawnFood(struct Snake* snake, struct Food* food) {
    int valid = 0;

    while (!valid) {
        valid = 1;

        // Spawn Food Randomly Inside Wall
        food->y = rand() % (MAP_SIZE - 2) + 1;
        food->x = rand() % (MAP_SIZE - 2) + 1;

        // IF Food Spawn Among SnakeNode
        struct SnakeNode* cur = snake->head;
        while (cur != NULL) {
            if (cur->y == food->y && cur->x == food->x) {
                valid = 0;
                break;
            }
            cur = cur->front;
        }
    }
}

int checkCollision(struct Snake* snake) {
    int y = snake->head->y;
    int x = snake->head->x;

    // Check Collision With Wall
    if (y <= 0 || y >= MAP_SIZE - 1 || x <= 0 || x >= MAP_SIZE - 1) {
        return 1;
    }

    // Check Collision With Body
    struct SnakeNode* cur = snake->head->front;
    while (cur != NULL) {
        if (cur->y == y && cur->x == x) {
            return 1;
        }
        cur = cur->front;
    }

    return 0;
}

int checkFood(struct Snake* snake, struct Food* food) {
    if (snake->head->y == food->y && snake->head->x == food->x) {
        return 1;
    }
    return 0;
}

// Auto Mode
int dy[4] = {-1, 1, 0, 0};
int dx[4] = {0, 0, -1, 1};

// Draw Game Map
void buildMap(int map[MAP_SIZE][MAP_SIZE], struct Snake* snake) {
    memset(map, 0, sizeof(int) * MAP_SIZE * MAP_SIZE);

    struct SnakeNode* cur = snake->head;
    while (cur != NULL) {
        map[cur->y][cur->x] = 1;  // Snake Body
        cur = cur->front;
    }
}

// BFS Algorithm
int bfs(int map[MAP_SIZE][MAP_SIZE], int sy, int sx, int fy, int fx, int parent[MAP_SIZE][MAP_SIZE][2]) {
    memset(parent, 0, sizeof(int) * MAP_SIZE * MAP_SIZE * 2);
    int visited[MAP_SIZE][MAP_SIZE] = {0};
    int qy[MAP_SIZE * MAP_SIZE];
    int qx[MAP_SIZE * MAP_SIZE];
    int front = 0, rear = 0;

    qy[rear] = sy;
    qx[rear++] = sx;
    visited[sy][sx] = 1;

    while (front < rear) {
        int y = qy[front];
        int x = qx[front++];

        if (y == fy && x == fx) return 1;

        for (int i = 0; i < 4; i++) {
            int ny = y + dy[i];
            int nx = x + dx[i];

            if (ny <= 0 || ny >= MAP_SIZE - 1 || nx <= 0 || nx >= MAP_SIZE - 1) continue;
            if (visited[ny][nx]) continue;
            if (map[ny][nx]) continue;

            visited[ny][nx] = 1;
            parent[ny][nx][0] = y;
            parent[ny][nx][1] = x;

            qy[rear] = ny;
            qx[rear++] = nx;
        }
    }

    return 0;
}

// Get First Direction in The BFS Path
enum Direction getDir(int parent[MAP_SIZE][MAP_SIZE][2], int sy, int sx, int fy, int fx) {
    int y = fy, x = fx;

    while (!(parent[y][x][0] == sy && parent[y][x][1] == sx)) {
        int py = parent[y][x][0];
        int px = parent[y][x][1];
        y = py;
        x = px;
    }

    if (y == sy - 1) return DIR_UP;
    if (y == sy + 1) return DIR_DOWN;
    if (x == sx - 1) return DIR_LEFT;
    if (x == sx + 1) return DIR_RIGHT;

    return DIR_RIGHT;
}

void autoMove(struct Snake* snake, struct Food* food) {
    int map[MAP_SIZE][MAP_SIZE];
    int parent[MAP_SIZE][MAP_SIZE][2];

    buildMap(map, snake);

    // 1. Find BFS Path and Safe Check After Path
    if (isSafeToFood(snake, food)) {
        bfs(map, snake->head->y, snake->head->x, food->y, food->x, parent);
        snake->dir = getDir(parent, snake->head->y, snake->head->x, food->y, food->x);
        return;
    }

    // 2. If No BFS Path to Food, Find Path to Tail
    struct SnakeNode* tail = snake->head;
    while (tail->front != NULL) tail = tail->front;
    map[tail->y][tail->x] = 0;

    static int cnt = 0, fy = 0, fx = 0, circular = 0;
    if (fy != food->y || fx != food->x) {
        fy = food->y;
        fx = food->x;
        cnt = 0;
        circular = 0;
    } else if (cnt >= MAP_SIZE * MAP_SIZE) {
        circular = 1;
    } else{
        cnt++;
    }

    if (!circular && bfs(map, snake->head->y, snake->head->x, tail->y, tail->x, parent)) {
        snake->dir = getDir(parent, snake->head->y, snake->head->x, tail->y, tail->x);
        return;
    }

    // 3. Definitely Goto Fruit
    if (bfs(map, snake->head->y, snake->head->x, food->y, food->x, parent)) {
        snake->dir = getDir(parent, snake->head->y, snake->head->x, food->y, food->x);
        return;
    }

    // 4. Last Fallback (Go Anywhere)
    for (int i = 0; i < 4; i++) {
        int ny = snake->head->y + dy[i];
        int nx = snake->head->x + dx[i];

        // Check Wall
        if (ny <= 0 || ny >= MAP_SIZE - 1 || nx <= 0 || nx >= MAP_SIZE - 1) continue;

        // Check SnakeBody
        struct SnakeNode* cur = snake->head;
        int collision = 0;
        while (cur != NULL) {
            if (cur->y == ny && cur->x == nx) {
                collision = 1;
                break;
            }
            cur = cur->front;
        }

        if (!collision) {
            snake->dir = i;  // DIR enum UP, DOWN, LEFT, RIGHT
            return;
        }
    }
}

struct Snake* copySnake(struct Snake* snake) {
    struct Snake* newSnake = malloc(sizeof(struct Snake));
    newSnake->length = snake->length;
    newSnake->dir = snake->dir;

    struct SnakeNode* cur = snake->head;
    struct SnakeNode* prev = NULL;

    while (cur != NULL) {
        struct SnakeNode* node = malloc(sizeof(struct SnakeNode));
        node->y = cur->y;
        node->x = cur->x;
        node->front = NULL;

        if (prev == NULL) {
            newSnake->head = node;
        } else {
            prev->front = node;
        }

        prev = node;
        cur = cur->front;
    }

    return newSnake;
}

void simulateMove(struct Snake* snake, int parent[MAP_SIZE][MAP_SIZE][2], int fy, int fx) {
    int pathY[MAP_SIZE * MAP_SIZE];
    int pathX[MAP_SIZE * MAP_SIZE];
    int len = 0;

    int y = fy, x = fx;

    // Path Backtracking
    while (!(snake->head->y == y && snake->head->x == x)) {
        pathY[len] = y;
        pathX[len] = x;
        int py = parent[y][x][0];
        int px = parent[y][x][1];
        y = py;
        x = px;
        len++;
    }

    for (int i = len - 1; i >= 0; i--) {
        addHead(snake, pathY[i], pathX[i]);

        if (pathY[i] != fy || pathX[i] != fx) {
            removeTail(snake);
        }
    }
}

int isSafeToFood(struct Snake* snake, struct Food* food) {
    int map[MAP_SIZE][MAP_SIZE];
    int parent[MAP_SIZE][MAP_SIZE][2];

    buildMap(map, snake);

    if (!bfs(map, snake->head->y, snake->head->x, food->y, food->x, parent)) return 0;

    // Copy Snake
    struct Snake* sim = copySnake(snake);

    // Simulation BFS Path
    simulateMove(sim, parent, food->y, food->x);

    // Find BFS Path After Simulation
    struct SnakeNode* tail = sim->head;
    while (tail->front != NULL) tail = tail->front;

    // Rebuild Map
    buildMap(map, sim);
    map[tail->y][tail->x] = 0;

    int safe = bfs(map, sim->head->y, sim->head->x, tail->y, tail->x, parent);

    freeSnake(sim);
    free(sim);

    return safe;
}