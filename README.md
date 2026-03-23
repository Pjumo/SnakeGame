# Snake Game

```bash
sudo apt install libncurses-dev

gcc snake.c -o snake -l ncursesw
```

---

# 1. Snake Game Rule

![](https://media1.tenor.com/m/J-ZRw-fOTboAAAAC/google-snake.gif)

- 뱀은 과일을 먹을 경우 길이가 늘어난다. (꼬리가 1칸 줄어들지 않는다)
- 뱀이 벽에 부딪히면 게임이 종료된다.
- 뱀이 자기 자신의 몸에 부딪히면 게임이 종료된다.
- 뱀이 한 칸씩 이동할 때 마다 꼬리도 한 칸씩 줄어든다.
- 뱀이 과일을 먹을 때마다 점수가 늘어난다.
- 종료 조건을 피하면서 최대한 많은 점수를 내야 하는 게임

---

# 2. 코드 설계

```c
enum DisplayMode { 
    MAIN, 
    PLAYGAME, 
    ENDGAME 
};

enum Direction {
		DIR_UP, 
		DIR_DOWN, 
		DIR_LEFT, 
		DIR_RIGHT 
};

struct Food {
    int y;
    int x;
};

struct SnakeNode {
    int y;
    int x;
    struct SnakeNode *front;
};

struct Snake {
    int lenght;
    struct SnakeNode *head;
    enum Direction dir;
};
```

- **Display Mode (FSM)**
    - MAIN  :  메인 화면
    - PLAYGAME  :  게임 플레이 화면
    - ENDGAME  :  일시 정지 혹은 종료 화면
- **Direction (바라보는 방향)**
    - DIR_UP  :  위쪽
    - DIR_DOWN  :  아래쪽
    - DIR_LEFT  :  왼쪽
    - DIR_RIGHT  :  오른쪽
- **Food**
    - y  :  행
    - x  :  열
- **SnakeNode**
    - y  :  행
    - x  :  열
    - front  :  붙어있는 뒤쪽 snake node 
                  가장 뒤쪽 node는 NULL
- **Snake**
    - length  :  길이
    - head  :  뱀 머리 snake node
    - dir  :  뱀이 바라보는 방향
    

---

```c
void drawMainPage();
void drawMenu(int selected);
void drawGame(struct Snake *snake, struct Food *food, int *score);
void moveSnake(struct Snake *snake, int grow);
int checkCollision(struct Snake *snake);
int checkFood(struct Snake *snake, struct Food *food);

int main() {
    enum DisplayMode displayMode = MAIN;

    while (1) {
        switch (displayMode) {
        case MAIN:
            drawMainPage();  // 메인 페이지 그리기
            drawMenu();  // 메뉴 그리기
            // main_logic
            break;
        case PLAYGAME:
		        drawGame();  // playing 화면 그리기
		        checkFood();  // 음식을 먹었는지 확인
		        moveSnake();  // 뱀 움직이기
		        checkCollision();  // 부딫혔는지 확인
		        // playing game logic
            break;
        case ENDGAME:
		        // logic after game ended
            break;
        }
        if(exit) break;
    }

    wrefresh(win);
    endwin();

    return 0;
}
```

- **MAIN → PLAYGAME → ENDGAME → MAIN / PLAYGAME** 형태의 기본 틀

- 각 상황에 맞는 메서드들을 따로 구현

- 매끄러운 움직임을 구현하기 위한 추가적인 라이브러리를 쓰지 않기 때문에 ncurses 내부의 **napms(ms)** 함수를 사용하여 화면 갱신

- **SnakeNode 구조체**를 사용하여 뱀을 **Linked List** 형태로 구현

- 입력은 방향키, esc키, enter키, **A키(Auto Mode : 도전과제)**를 사용

---

# 3. AutoMode

**함수 선언 및 전역변수 추가**

```c
void buildMap(int map[MAP_SIZE][MAP_SIZE], struct Snake* snake);
int bfs(int map[MAP_SIZE][MAP_SIZE], int sy, int sx, int fy, int fx, int parent[MAP_SIZE][MAP_SIZE][2]);
enum Direction getDir(int parent[MAP_SIZE][MAP_SIZE][2], int sy, int sx, int fy, int fx);
void autoMove(struct Snake* snake, struct Food* food);

int autoMode = 0;
```

---

**PLAYGAME 부분 코드 변경**

```c
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
```

```c
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

napms(autoMode?AUTO_MODE_PERIOD:GAME_PERIOD);
break;
```

- A키를 누르면 Auto Mode Toggle

- AutoMode 일때 inputHandler 대신 autoMove로 방향 변경

- 기존 코드가 checkFood 이후에 moveSnake를 하고 있어서 , 순서 바꿔줌. 
(autoMode에서 food 좌표와 snake head 좌표가 같은 경우가 발생하여 움직임이 없어지는 경우가 생김)

- Init 과정에서 autoMode 0으로 초기화

- AutoMode 일때 50ms period로 진행 (Playing은 150ms)

---

```c
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
```

- dy, dx는 UP DOWN, LEFT, RIGHT를 탐색하기 위함

- buildMap은 map memory 초기화 및 Snake의 좌표 정보를 1로 담은 map을 그리는 함수

- getDir은 bfs에서 나온 parent 배열을 food 좌표부터 시작해 역순으로 참조해가며
다음 Direction을 반환하는 함수

---

```c
// BFS Algorithm
int bfs(int map[MAP_SIZE][MAP_SIZE], int sy, int sx, int fy, int fx, int parent[MAP_SIZE][MAP_SIZE][2]) {
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
```

- BFS 알고리즘 구현

- 여기서 map을 snake head 좌표부터 탐색해가며 bfs 탐색 우선순위에 따라 탐색

- 만약 탐색하다가 food의 좌표 위치를 만나게 되면 탐색 종료

- parent는 현재 좌표를 탐색하기 직전 좌표값을 담고 있음 (칠판에 설명)

- qx, qy는 queue로 FIFO를 구현

---

```c
void autoMove(struct Snake* snake, struct Food* food) {
    int map[MAP_SIZE][MAP_SIZE];
    int parent[MAP_SIZE][MAP_SIZE][2];

    buildMap(map, snake);

    // Find BFS Path
    if (bfs(map, snake->head->y, snake->head->x, food->y, food->x, parent)) {
        snake->dir = getDir(parent, snake->head->y, snake->head->x, food->y, food->x);
        return;
    }

    // If No BFS Path to Food, Find Path to Tail
    struct SnakeNode* tail = snake->head;
    while (tail->front != NULL) tail = tail->front;

    if (bfs(map, snake->head->y, snake->head->x, tail->y, tail->x, parent)) {
        snake->dir = getDir(parent, snake->head->y, snake->head->x, tail->y, tail->x);
        return;
    }

    // Last Fallback (Go Anywhere)
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
```

- 만약 food까지 bfs 탐색이 가능하다면, 그 루트대로 진행

- 만약 안될 경우 꼬리까지 bfs 탐색 → 꼬리를 따라 돌다보면 bfs 탐색이 가능해질 수 있음

- 그것 또한 안될경우 갈 수 있는 아무 방향으로 진행

---

## Upgrade Version

```c
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
```

- 기존 Snake를 그대로 Copy하는 함수

---

```c
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
```

- 음식까지 최단거리 (BFS)를 그대로 진행하는 함수

- 복사해놓은 snake 포인터를 최단 경로대로 시뮬레이션함

- parent에 저장된 최단경로 Path를 BackTracking하며 진행한다.

---

```c
int isSafeToFood(struct Snake* snake, struct Food* food) {
    int map[MAP_SIZE][MAP_SIZE];
    int parent[MAP_SIZE][MAP_SIZE][2];

    buildMap(map, snake);

    if (!bfs(map, snake->head->y, snake->head->x, food->y, food->x, parent))
        return 0;

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
```

- 만약 음식까지의 최단 경로대로 진행했을  경우 그 시점의 뱀이 꼬리까지 길 탐색이 되는지 확인하는 함수

- 동작방식
    1. 기존 방식대로 map을 그리고, 음식까지 탐색 자체가 안되는 경우에는 return 0
    
    2. 만약 음식까지 최단거리 탐색이 된다면, snake를 복사하여 sim에 담음
    
    3. 최단거리대로 sim을 움직여봄
    
    4. 이 시점에서 Head에서 Tail 까지 최단거리 탐색이 되는지 확인함
    
    5. 된다면 1 아니면 0 return

---

```c
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
```

- autoMove 함수 구조 변경 (buildMap, bfs, getDir 함수는 그대로 사용)

- 4단계의 동작으로 변경
    1. BFS Path를 찾고, 그대로 진행했을 때 꼬리까지 탐색이 되는지 확인
    
    2. 만약 꼬리까지 탐색이 안된다면, 일단 꼬리 물기로 방향을 진행
    
    3. 만약 음식을 먹지 않은 상태에서 계속 같은 순환을 반복하게 된다면 
    탈출해서 무조건 음식 BFS 최단루트로 진행.
    
    4. 만약 음식 최단루트 또한 없다면 갈 수 있는 아무 방향으로나 진행
