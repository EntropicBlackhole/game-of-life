#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raylib.h"

// assumes columns as width
void renderGrid(const char* matrix, Color* pixel_buffer,
                Texture2D screen_texture, int width, int height) {
  for (int i = 0; i < width * height; i++) {
    pixel_buffer[i] = (matrix[i] == 1) ? WHITE : BLACK;
  }

  UpdateTexture(screen_texture, pixel_buffer);

  BeginDrawing();
  ClearBackground(BLACK);

  DrawTexturePro(screen_texture, (Rectangle){0, 0, (float)width, (float)height},
                 (Rectangle){0, 0, (float)width, (float)height},
                 (Vector2){0, 0}, 0.0f, WHITE);

  EndDrawing();
}

int main() {
  SetTraceLogLevel(LOG_NONE);
  SetConfigFlags(FLAG_WINDOW_HIDDEN);

  // inicializar ventana de raylib
  InitWindow(50, 50,
             "game of life");

  int currentMonitor = GetCurrentMonitor();
  const int monitorWidth = GetMonitorWidth(currentMonitor);
  const int monitorHeight = GetMonitorHeight(currentMonitor);

  int total_cells = monitorWidth * monitorHeight;
  if (total_cells == 0 || total_cells == NULL) total_cells = 1000 * 1000;

  char* current_grid = (char*)calloc(total_cells, sizeof(char));
  char* next_grid = (char*)calloc(total_cells, sizeof(char));
  char* seed = (char*)malloc((total_cells + 1) * sizeof(char));
  printf("===============================\n");
  printf("      conways game of life\n");
  printf("===============================\n");
  printf("1. input a custom string seed\n");
  printf("2. choose a preset\n");
  printf("select 1 or 2: ");

  char menu_choice[10];
  fgets(menu_choice, sizeof(menu_choice), stdin);

  if (menu_choice[0] == '2') {
    printf("\n");
    printf("1. glider gun (runs indefinitely)\n");
    printf("2. random noise\n");
    printf("select 1 or 2: ");

    char blueprint_choice[10];
    fgets(blueprint_choice, sizeof(blueprint_choice), stdin);

    if (blueprint_choice[0] == '1') {
      int gun_nodes[][2] = {
          {5, 1},  {5, 2},  {6, 1},  {6, 2},  {5, 11}, {6, 11},
          {7, 11}, {4, 12}, {8, 12}, {3, 13}, {9, 13}, {3, 14},
          {9, 14}, {6, 15}, {4, 16}, {8, 16}, {5, 17}, {6, 17},
          {7, 17}, {6, 18}, {3, 21}, {4, 21}, {5, 21}, {3, 22},
          {4, 22}, {5, 22}, {2, 23}, {6, 23}, {1, 25}, {2, 25},
          {6, 25}, {7, 25}, {3, 35}, {4, 35}, {3, 36}, {4, 36}};
      int total_nodes = 36;

      int start_row = monitorHeight / 4;
      int start_col = monitorWidth / 4;

      for (int idx = 0; idx < total_nodes; idx++) {
        int r = start_row + gun_nodes[idx][0];
        int c = start_col + gun_nodes[idx][1];
        if (r >= 0 && r < monitorHeight && c >= 0 && c < monitorWidth) {
          current_grid[(r * monitorWidth) + c] = 1;
        }
      }
      printf("\nloaded glider gun\n");
    } else {
      for (int i = 0; i < total_cells; i++) {
        current_grid[i] = (rand() % 100 < 30) ? 1 : 0;
      }
    }
  } else {
    printf(
        "input a seed, it can be any string, longer strings are recommended:\n");
    fgets(seed, total_cells + 1, stdin);
    seed[strcspn(seed, "\n")] = '\0';
    // printf("%s\n", seed);
    // generate a matrix, likely a struct
    // place random cells on it
    // render
    // go into loop
    //  check for neighbors
    //  write to buffer matrix
    //  switch pointers between both matrices
    //  render new matrix
    // char* matrix = (char*)calloc((total_cells + 7)/8, sizeof(char));

    if (current_grid == NULL || next_grid == NULL) {
      printf("clear allocation went wrong\n");
      return 1;
    }

    size_t seed_bytes = strlen(seed);
    if (seed_bytes > 0) {
      for (int i = 0; i < total_cells; i++) {
        int current_byte_idx = (i / 8) % seed_bytes;
        char current_char = seed[current_byte_idx];
        int bit_position = 7 - (i % 8);
        current_grid[i] = (current_char >> bit_position) & 1;
      }
    }
    // renderMatrix(matrix, ROWS, COLS);
    // printf("%d", matrix[ROWS*COLS]);
    free(seed);
  }
  
  ClearWindowState(FLAG_WINDOW_HIDDEN);

  SetWindowState(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_MAXIMIZED);
  SetWindowSize(monitorWidth, monitorHeight);
  SetWindowPosition(0, 0);
  SetTargetFPS(60);

  Color* pixel_buffer = (Color*)malloc(total_cells * sizeof(Color));
  Image screen_image = {.data = pixel_buffer,
                        .width = monitorWidth,
                        .height = monitorHeight,
                        .mipmaps = 1,
                        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
  Texture2D screen_texture = LoadTextureFromImage(screen_image);

  int neighborCount = 0;
  while (!WindowShouldClose()) {
    for (int i = 0; i < total_cells; i++) {
      int r = i / monitorWidth;
      int c = i % monitorWidth;
      // next_grid[i] = current_grid[i];
      /*
      0000000000
      0000000000
      0010000000
      000x100000
      0011100000
      0000000000
      0000000000
      0000000000
      0000000000
      0000000000
      */

      for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
          if (dr == 0 && dc == 0) continue;

          int neighborRow = (r + dr + monitorHeight) % monitorHeight;
          int neighborCol = (c + dc + monitorWidth) % monitorWidth;

          // if (neighborRow >= 0 && neighborRow < ROWS && neighborCol >= 0 &&
          //     neighborCol < COLS) {
          int neighborIndex = (neighborRow * monitorWidth) + neighborCol;
          if (current_grid[neighborIndex] == 1) {
            neighborCount++;
          }
          // }
        }
      }

      if (current_grid[i] == 1) {
        // survive
        if (neighborCount == 2 || neighborCount == 3)
          next_grid[i] = 1;
        else  // die
          next_grid[i] = 0;
      }

      // reproduce
      if (current_grid[i] == 0)
        if (neighborCount == 3)
          next_grid[i] = 1;
        else
          next_grid[i] = 0;

      neighborCount = 0;
    }

    char* temp = current_grid;
    current_grid = next_grid;
    next_grid = temp;

    renderGrid(current_grid, pixel_buffer, screen_texture, monitorWidth,
               monitorHeight);
  }

  free(pixel_buffer);
  free(current_grid);
  free(next_grid);
  UnloadTexture(screen_texture);
  CloseWindow();

  return 0;
}

// fcyvgbhnjmkhubg vyftc drxctfgvhbjnkmlk,;m nkbjhgvhgbjhknm
// is interesting