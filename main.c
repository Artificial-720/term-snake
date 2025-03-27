#define _POSIX_C_SOURCE 199309L // Needed to include nanosleep
#include <ncurses.h>
#include <stdlib.h> // malloc
#include <assert.h> // assert
#include <time.h>
#include <sys/time.h> // nanosleep, gettimeofday

#define UP 0
#define RIGHT 1
#define DOWN 2
#define LEFT 3
#define ATTR_DEFAULT 0
#define ATTR_BODY 1
#define ATTR_HEAD 2
#define ATTR_FOOD 3

typedef struct {
  int x, y;
} Position;

static int collision_check(const Position pos);
static void draw_game(WINDOW* window);
static int game_step(int direction);
static long current_time_millis();

int game_width = 20;
int game_height = 20;
Position food = {5, 5};
Position* segments;
Position head = {1, 1};
int size = 0;
int length = 0;

int main() {
  int running = 1;
  int direction = RIGHT;

  // Setup ncurses
  initscr();
  keypad(stdscr, TRUE); // enabled function keys and arrow keys
  curs_set(0);          // hide the cursor
  noecho();             // stop characters from being echoed
  timeout(10);          // timeout for getch
  // setup Color
  start_color();
  init_pair(ATTR_DEFAULT, COLOR_WHITE, COLOR_BLACK);
  init_pair(ATTR_BODY, COLOR_WHITE, COLOR_WHITE);
  init_pair(ATTR_HEAD, COLOR_WHITE, COLOR_GREEN);
  init_pair(ATTR_FOOD, COLOR_WHITE, COLOR_RED);

  // Create window for game
  WINDOW* window;
  window = newwin(game_height + 2, (game_width * 2) + 2, 1, 1);
  wbkgdset(window, COLOR_PAIR(ATTR_DEFAULT));

  // Initialize game
  size = game_height * game_width;
  segments = (Position*)malloc(size * sizeof(Position));
  if (!segments) {
    endwin();
    printf("Memory allocation failed\n");
    return 0;
  }
  for (int i = 0; i < size; i++) {
    segments[i] = (Position){0, 0};
  }
  draw_game(window);

  // Seed random
  srand(time(NULL));

  int input = 0;
  int snake_speed = 10; // Moves 10 times per seconds
  time_t time_per_frame = 1000 / snake_speed;
  time_t previous_time = current_time_millis();
  time_t accumulated_time = 0;
  while(running) {
    time_t now = current_time_millis();
    time_t deltatime = now -previous_time;
    previous_time = now;
    accumulated_time += deltatime;

    if (accumulated_time >= time_per_frame) {
      accumulated_time -= time_per_frame;
      // Game Step
      running = game_step(direction);

      // print score
      move(0, 0);
      printw("Score: %d", length);

      // Draw graphics
      draw_game(window);
    }

    // get user input
    if (running) {
      input = getch();
      switch (input) {
        case KEY_UP:
        case (int)'w':
          direction = UP;
          break;
        case KEY_LEFT:
        case (int)'a':
          direction = LEFT;
          break;
        case KEY_DOWN:
        case (int)'s':
          direction = DOWN;
          break;
        case KEY_RIGHT:
        case (int)'d':
          direction = RIGHT;
          break;
        case (int)'q':
          running = false;
          break;
      }
    }

    // Sleep to prevent CPU hog
    struct timespec req = {
      .tv_sec = 0,
      .tv_nsec = 1000000L
    };
    nanosleep(&req, NULL);
  }


  free(segments);
  endwin();

  printf("Score: %d\n", length);

  return 0;
}


static int game_step(int direction) {
  int spawn_food = 0;
  Position new_head = head;

  switch (direction) {
    case UP:
      new_head.y--;
      break;
    case DOWN:
      new_head.y++;
      break;
    case LEFT:
      new_head.x--;
      break;
    case RIGHT:
      new_head.x++;
      break;
    default:
      assert(0); // Should never reach here
  }
  // Check if new head pos is valid
  int collision_val = collision_check(new_head);
  // want 0 or 2
  if (collision_val == 1 || collision_val == 3) {
    // YOU LOSE
    return 0;
  }
  if (collision_val == 2) {
    // Food, grow body
    assert(length < size);
    segments[length] = head;
    length++;
    spawn_food = 1;
  }

  // move body
  if (!spawn_food) {
    Position next = head;
    for (int i = length - 1; i >= 0; i--) {
      Position temp = segments[i];
      segments[i].x = next.x;
      segments[i].y = next.y;
      next = temp;
    }
  }
  head = new_head;

  // Spawn food
  if (spawn_food) {
    Position new_food = {0, 0};
    // WARN: this might infinite loop
    do {
      new_food.x = rand() % game_width;
      new_food.y = rand() % game_height;
    } while (collision_check(new_food));
    food = new_food;
  }

  return 1;
}


static int collision_check(const Position pos) {
  // 0 valid, no collision
  // 1 invalid out of bounds
  // 2 food collision
  // 3 body collision
  if (pos.x < 0 || pos.x >= game_width) return 1;
  if (pos.y < 0 || pos.y >= game_height) return 1;
  if (pos.x == food.x && pos.y == food.y) return 2;

  // TODO handle last segment cause it moves
  Position seg;
  for (int i = 0; i < length; i++) {
    seg = segments[i];
    if (pos.x == seg.x && pos.y == seg.y) {
      return 3;
    }
  }

  return 0;
}

void draw_game(WINDOW* window) {
  wclear(window);
  box(window, 0, 0);

  // Body
  wbkgdset(window, COLOR_PAIR(ATTR_BODY));
  for (int i = 0; i < length; i++) {
    Position seg = segments[i];
    wmove(window, seg.y + 1, (seg.x * 2) + 1);
    waddch(window, ' ');
    waddch(window, ' ');
  }
  // Food
  wbkgdset(window, COLOR_PAIR(ATTR_FOOD));
  wmove(window, food.y + 1, (food.x * 2) + 1);
  waddch(window, ' ');
  waddch(window, ' ');
  // Head
  wbkgdset(window, COLOR_PAIR(ATTR_HEAD));
  wmove(window, head.y + 1, (head.x * 2) + 1);
  waddch(window, ' ');
  waddch(window, ' ');

  wbkgdset(window, COLOR_PAIR(ATTR_DEFAULT));
  wrefresh(window);
}

static long current_time_millis() {
  struct timeval tp;
  gettimeofday(&tp, NULL);
  return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}
