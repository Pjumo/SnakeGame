#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
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

#define UNICODE_FULL_BLOCK "██"
#define UNICODE_HALF_BLOCK "█"
#define UNICODE_BLOCK "■"
#define UNICODE_2BLANK "　"
#define UNICODE_SELECT "➜"
#define UNICODE_BLUR_BLOCK "▓▓"
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
    BLACK,   // 0
    RED,     // 1
    GREEN,   // 2
    YELLOW,  // 3
    BLUE,    // 4
    CYAN,    // 5
    MAGENTA, // 6
    WHITE    // 7
};

enum DisplayMode { MAIN, PLAYGAME, ENDGAME };

enum Direction { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };

struct Food {
    int y;
    int x; // *2
};

struct SnakeNode {
    int y;
    int x; // *2
    struct SnakeNode *front;
};

struct Snake {
    int length;
    struct SnakeNode *head;
    enum Direction dir;
};

const int mainText[5][24] = {
    {1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 1},
    {1, 0, 0, 0, 1, 1, 2, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0},
    {1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1},
    {0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0},
    {1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1}};

WINDOW *win = NULL;

// Draw
void drawMainPage();
void drawMenu(int selected);
void drawGame(struct Snake *snake, struct Food *food, int *score);
// Snake
void initSnake(struct Snake *snake);
void moveSnake(struct Snake *snake, int grow);
void inputHandler(struct Snake *snake, int *ch);
void addHead(struct Snake *snake, int y, int x);
void removeTail(struct Snake *snake);
void freeSnake(struct Snake *snake);
// Food
void initFood(struct Food *food);
void spawnFood(struct Snake *snake, struct Food *food);
// Check
int checkCollision(struct Snake *snake);
int checkFood(struct Snake *snake, struct Food *food);

int main() {
    setlocale(LC_ALL, "");
    initscr();
    curs_set(0);           // hide cursor
    noecho();              // hide input
    keypad(stdscr, TRUE);  // activate special key
    nodelay(stdscr, TRUE); // activate non-blocking input
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
                srand(time(NULL)); // Init Random Seed
                score = 0;
                isInit = 0;
            }

            inputHandler(&snake, &ch);

            int grow = checkFood(&snake, &food);

            moveSnake(&snake, grow);
            if (grow) {
                score++;
                spawnFood(&snake, &food);
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

            napms(150);
            break;
        case ENDGAME:
            werase(win);
            box(win, 0, 0);
            mvwprintw(win, MAP_SIZE / 2 - 1, MAP_SIZE - 5, "GAME OVER");
            mvwprintw(win, MAP_SIZE / 2 + 1, MAP_SIZE - 6, "Press Enter");
            wrefresh(win);

            if(ch == KEY_ENTER){
                freeSnake(&snake);
                isInit = 1;
                displayMode = MAIN;
            }
            break;
        }
        if (exit)
            break;
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
                mvwaddstr(win, MAIN_TEXT_START_Y + i, MAIN_TEXT_START_X + j,
                          UNICODE_HALF_BLOCK);
            } else {
                mvwaddstr(win, MAIN_TEXT_START_Y + i, MAIN_TEXT_START_X + j,
                          " ");
            }
            wattroff(win, COLOR_PAIR(textColor));
        }
        napms(300); // 300ms period
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

void drawGame(struct Snake *snake, struct Food *food, int *score) {
    werase(win);
    box(win, 0, 0);

    // Draw Score
    mvwprintw(win, SCORE_TEXT_Y, SCORE_TEXT_X, " score : %d ", *score);

    // Draw Food
    wattron(win, COLOR_PAIR(RED));
    mvwprintw(win, food->y, food->x * 2, UNICODE_HEART);
    wattroff(win, COLOR_PAIR(RED));

    // Draw Snake
    struct SnakeNode *s = snake->head;
    wattron(win, COLOR_PAIR(GREEN));
    mvwprintw(win, s->y, (s->x) * 2, UNICODE_FULL_BLOCK);
    while (s->front != NULL) {
        s = s->front;
        mvwprintw(win, s->y, (s->x) * 2, UNICODE_BLUR_BLOCK);
    }
    wattroff(win, COLOR_PAIR(GREEN));
    wrefresh(win);
}

void initSnake(struct Snake *snake) {
    snake->length = SNAKE_INIT_LENGTH;
    snake->dir = DIR_RIGHT; // Init Direction

    struct SnakeNode *s = (struct SnakeNode *)malloc(sizeof(struct SnakeNode));
    snake->head = s;

    // Init First Body
    for (int i = 0; i < SNAKE_INIT_LENGTH; i++) {
        s->y = SNAKE_INIT_Y;
        s->x = SNAKE_INIT_X - i;
        s->front = (i == SNAKE_INIT_LENGTH - 1)
                       ? NULL
                       : (struct SnakeNode *)malloc(sizeof(struct SnakeNode));
        s = s->front;
    }
}

void freeSnake(struct Snake *snake) {
    struct SnakeNode *cur = snake->head;

    while (cur != NULL) {
        struct SnakeNode *temp = cur;
        cur = cur->front;
        free(temp);
    }

    snake->head = NULL;
    snake->length = 0;
}

void moveSnake(struct Snake *snake, int grow) {
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

    // 새로운 머리 추가
    addHead(snake, newY, newX);

    // 먹이 안 먹었으면 꼬리 제거
    if (!grow) {
        removeTail(snake);
    }
}

void inputHandler(struct Snake *snake, int *ch) {
    switch (*ch) {
    case KEY_UP:
        if (snake->dir != DIR_DOWN)
            snake->dir = DIR_UP;
        break;
    case KEY_DOWN:
        if (snake->dir != DIR_UP)
            snake->dir = DIR_DOWN;
        break;
    case KEY_LEFT:
        if (snake->dir != DIR_RIGHT)
            snake->dir = DIR_LEFT;
        break;
    case KEY_RIGHT:
        if (snake->dir != DIR_LEFT)
            snake->dir = DIR_RIGHT;
        break;
    }
}

void addHead(struct Snake *snake, int y, int x) {
    struct SnakeNode *newNode = malloc(sizeof(struct SnakeNode));

    // Shift(Add) Head
    newNode->y = y;
    newNode->x = x;
    newNode->front = snake->head;

    snake->head = newNode;
    snake->length++;
}

void removeTail(struct Snake *snake) {
    // Exception
    if (snake->head == NULL)
        return;

    struct SnakeNode *cur = snake->head;

    // Find Tail
    while (cur->front->front != NULL) {
        cur = cur->front;
    }

    // Free Tail Memory
    free(cur->front);
    cur->front = NULL;

    snake->length--;
}

void initFood(struct Food *food) {
    food->y = FOOD_INIT_Y;
    food->x = FOOD_INIT_X;
}

void spawnFood(struct Snake *snake, struct Food *food) {
    int valid = 0;

    while (!valid) {
        valid = 1;

        // Spawn Food Randomly Inside Wall
        food->y = rand() % (MAP_SIZE - 2) + 1;
        food->x = rand() % (MAP_SIZE - 2) + 1;

        // IF Food Spawn Among SnakeNode
        struct SnakeNode *cur = snake->head;
        while (cur != NULL) {
            if (cur->y == food->y && cur->x == food->x) {
                valid = 0;
                break;
            }
            cur = cur->front;
        }
    }
}

int checkCollision(struct Snake *snake) {
    int y = snake->head->y;
    int x = snake->head->x;

    // Check Collision With Wall
    if (y <= 0 || y >= MAP_SIZE - 1 || x <= 0 || x >= MAP_SIZE - 1) {
        return 1;
    }

    // Check Collision With Body
    struct SnakeNode *cur = snake->head->front;
    while (cur != NULL) {
        if (cur->y == y && cur->x == x) {
            return 1;
        }
        cur = cur->front;
    }

    return 0;
}

int checkFood(struct Snake *snake, struct Food *food) {
    if (snake->head->y == food->y && snake->head->x == food->x) {
        return 1;
    }
    return 0;
}