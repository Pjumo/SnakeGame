#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <ncurses.h>
#include <locale.h>

#define MAP_SIZE 30
#define MAIN_TEXT_START_Y 6
#define MAIN_TEXT_START_X 7
#define UNICODE_FULL_BLOCK "██"
#define UNICODE_HALF_BLOCK "█"
#define UNICODE_BLOCK "■"
#define UNICODE_2BLANK "　"

enum ColorType {
	RED,        // 1
	GREEN,      // 2
    YELLOW,     // 3
	BLUE,       // 4
	CYAN,       // 5
	MAGENTA,    // 6
	WHITE       // 7
};

enum Direction { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };

struct Food{
    int y;
    int x;
};

struct SnakeNode{
    int y;
    int x;
    struct SnakeNode *front;
};

struct Snake {
    int lenght;
    struct SnakeNode *head;
    enum Direction dir;
};

const int mainText[5][24] = {
	{1, 1, 1, 0, 1, 2, 0, 0, 1, 0, 0, 3, 1, 2, 0, 0, 1, 0, 3, 2, 0, 1, 1, 1},
	{1, 0, 0, 0, 1, 1, 2, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0},
	{1, 1, 1, 0, 1, 3, 1, 2, 1, 0, 3, 2, 0, 3, 2, 0, 1, 1, 2, 0, 0, 1, 1, 1},
	{0, 0, 1, 0, 1, 0, 3, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0},
	{1, 1, 1, 0, 1, 0, 0, 3, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 3, 2, 0, 1, 1, 1}
};

int main(){
    setlocale(LC_ALL, "");
    initscr();
    curs_set(0);
    start_color();
    init_pair(1, COLOR_RED,     COLOR_BLACK);
    init_pair(2, COLOR_GREEN,   COLOR_BLACK);
    init_pair(3, COLOR_YELLOW,  COLOR_BLACK);
    init_pair(4, COLOR_BLUE,    COLOR_BLACK);
    init_pair(5, COLOR_CYAN,    COLOR_BLACK);
    init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(7, COLOR_WHITE,   COLOR_BLACK);

    WINDOW *win = newwin(MAP_SIZE, MAP_SIZE * 2, 10, 20);
    box(win, 0, 0);

    refresh();
    wrefresh(win);

    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 24; j++) {
            int textColor;
            if (j < 4)
                textColor = GREEN;
            else if (j < 10)
                textColor = RED;
            else if (j < 16)
                textColor = BLUE;
            else if (j < 21)
                textColor = YELLOW;
            else
                textColor = MAGENTA;

            wattron(win, COLOR_PAIR(1));
            if (mainText[i][j] == 1){
                mvwaddstr(win, MAIN_TEXT_START_Y + i, MAIN_TEXT_START_X + j * 2, UNICODE_FULL_BLOCK);
            } else if(mainText[i][j] == 2){
                mvwaddstr(win, MAIN_TEXT_START_Y + i, MAIN_TEXT_START_X + j * 2, UNICODE_HALF_BLOCK);
                mvwaddstr(win, MAIN_TEXT_START_Y + i, MAIN_TEXT_START_X + j * 2 + 1, " ");
            } else if(mainText[i][j] == 3){
                mvwaddstr(win, MAIN_TEXT_START_Y + i, MAIN_TEXT_START_X + j * 2, " ");
                mvwaddstr(win, MAIN_TEXT_START_Y + i, MAIN_TEXT_START_X + j * 2 + 1, UNICODE_HALF_BLOCK);
            } else {
                mvwaddstr(win, MAIN_TEXT_START_Y + i, MAIN_TEXT_START_X + j * 2, "  ");
            }
            wattroff(win, COLOR_PAIR(1));
        }
        sleep(1);
        wrefresh(win);
    }

    wrefresh(win);
    getch();
    endwin();

    return 0;
}
