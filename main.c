#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raylib.h"

#define MAX_PRESETS 10
#define MAX_NAME_LEN 64

typedef struct {
  char name[MAX_NAME_LEN];
  float pos_x_ratio;
  float pos_y_ratio;
  int width;
  int height;
  char* grid_data;
} Preset;

bool loadPresetFile(const char* filepath, Preset* out_preset) {
  FILE* file = fopen(filepath, "r");
  if (!file) return false;

  char line[4096];
  out_preset->width = 0;
  out_preset->height = 0;
  out_preset->pos_x_ratio = 0.0f;
  out_preset->pos_y_ratio = 0.0f;
  out_preset->name[0] = '\0';

  int capacity = 2048;
  out_preset->grid_data = (char*)malloc(capacity * sizeof(char));
  if (!out_preset->grid_data) {
    fclose(file);
    return false;
  }

  int data_size = 0;

  while (fgets(line, sizeof(line), file)) {
    if (strncmp(line, "name:", 5) == 0) {
      sscanf(line, "name:%63[^\r\n]", out_preset->name);
    } else if (strncmp(line, "position:", 9) == 0) {
      sscanf(line, "position:%f,%f", &out_preset->pos_x_ratio,
             &out_preset->pos_y_ratio);
    } else {
      int current_row_width = 0;
      for (int i = 0; line[i] != '\0'; i++) {
        if (line[i] == '0' || line[i] == '1') {
          if (data_size >= capacity) {
            capacity *= 2;
            char* new_grid =
                (char*)realloc(out_preset->grid_data, capacity * sizeof(char));
            if (!new_grid) {
              free(out_preset->grid_data);
              out_preset->grid_data = NULL;
              fclose(file);
              return false;
            }
            out_preset->grid_data = new_grid;
          }
          out_preset->grid_data[data_size++] = (line[i] == '1') ? 1 : 0;
          current_row_width++;
        }
      }
      if (current_row_width > 0) {
        out_preset->width = current_row_width;
        out_preset->height++;
      }
    }
  }

  fclose(file);

  if (data_size == 0) {
    free(out_preset->grid_data);
    out_preset->grid_data = NULL;
    return false;
  }

  return true;
}

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
  InitWindow(400, 400, "game of life");

  int currentMonitor = GetCurrentMonitor();
  const int monitorWidth = GetMonitorWidth(currentMonitor);
  const int monitorHeight = GetMonitorHeight(currentMonitor);

  int total_cells = monitorWidth * monitorHeight;
  if (total_cells <= 0) total_cells = 1000 * 1000;

  char* current_grid = (char*)calloc(total_cells, sizeof(char));
  char* next_grid = (char*)calloc(total_cells, sizeof(char));

  if (!current_grid || !next_grid) {
    printf("clear allocation went wrong\n");
    free(current_grid);
    free(next_grid);
    CloseWindow();
    return 1;
  }

  printf("===============================\n");
  printf("      conways game of life\n");
  printf("===============================\n");
  printf("1. input a custom string seed\n");
  printf("2. choose a preset\n");
  printf("3. random noise\n");
  printf("select 1-3: ");

  char menu_choice[16];
  if (!fgets(menu_choice, sizeof(menu_choice), stdin)) {
    free(current_grid);
    free(next_grid);
    CloseWindow();
    return 1;
  }

  if (menu_choice[0] == '2') {
    FilePathList files = LoadDirectoryFiles("./presets");
    int available_presets = 0;
    Preset preset_list[MAX_PRESETS];
    printf("\navailable presets:\n");
    for (unsigned int i = 0; i < files.count && available_presets < MAX_PRESETS;
         i++) {
      if (IsFileExtension(files.paths[i], ".dat") ||
          IsFileExtension(files.paths[i], ".txt")) {
        if (loadPresetFile(files.paths[i], &preset_list[available_presets])) {
          printf("%d. %s\n", available_presets + 1,
                 preset_list[available_presets].name);
          available_presets++;
        }
      }
    }
    UnloadDirectoryFiles(files);

    if (available_presets == 0) {
      printf(
          "no valid preset found in /presets, input a preset with the "
          "following structure:\n\n");
      printf("name:gosper glider gun\n");
      printf("position:0.25,0.25\n");
      printf("000000000000000000000000100000000000\n");
      printf("000000000000000000000010100000000000\n");
      printf("000000000000110000001100000000000011\n");
      printf("000000000001000100001100000000000011\n");
      printf("110000000010000010001100000000000000\n");
      printf("110000000010001011000010100000000000\n");
      printf("000000000010000010000000100000000000\n");
      printf("000000000001000100000000000000000000\n");
      printf("000000000000110000000000000000000000\n\n");
      printf("then save it as a .dat file inside /presets with any name\n");
      printf("after that, close and reopen this program\n\n");
      getchar();
      free(current_grid);
      free(next_grid);
      CloseWindow();
      return 0;
    }

    int selection = 0;
    if (available_presets == 1) {
      printf("load? (y/n): ");
      char choice;
      scanf(" %c", &choice);
      if (choice == 'n' || choice == 'N') {
        free(preset_list[0].grid_data);
        free(current_grid);
        free(next_grid);
        CloseWindow();
        return 0;
      }
    } else {
      printf("select preset item (1-%d): ", available_presets);
      char choice_str[16];
      if (fgets(choice_str, sizeof(choice_str), stdin)) {
        selection = atoi(choice_str) - 1;
      }
    }

    if (selection >= 0 && selection < available_presets) {
      Preset active = preset_list[selection];

      int start_row = (int)(monitorHeight * active.pos_y_ratio);
      int start_col = (int)(monitorWidth * active.pos_x_ratio);

      for (int r = 0; r < active.height; r++) {
        for (int c = 0; c < active.width; c++) {
          int target_row = start_row + r;
          int target_col = start_col + c;

          if (target_row >= 0 && target_row < monitorHeight &&
              target_col >= 0 && target_col < monitorWidth) {
            int grid_idx = (target_row * monitorWidth) + target_col;
            int blueprint_idx = (r * active.width) + c;
            current_grid[grid_idx] = active.grid_data[blueprint_idx];
          }
        }
      }
      printf("\nloaded %s\n", active.name);
    }

    for (int i = 0; i < available_presets; i++) {
      free(preset_list[i].grid_data);
    }
  } else if (menu_choice[0] == '3') {
    for (int i = 0; i < total_cells; i++) {
      current_grid[i] = (rand() % 100 < 30) ? 1 : 0;
    }
  } else {
    printf(
        "input a seed, it can be any string, longer strings are "
        "recommended:\n");

    char seed[4096];
    if (fgets(seed, sizeof(seed), stdin)) {
      seed[strcspn(seed, "\r\n")] = '\0';
      size_t seed_bytes = strlen(seed);
      if (seed_bytes > 0) {
        for (int i = 0; i < total_cells; i++) {
          int current_byte_idx = (i / 8) % seed_bytes;
          char current_char = seed[current_byte_idx];
          int bit_position = 7 - (i % 8);
          current_grid[i] = (current_char >> bit_position) & 1;
        }
      }
    }
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

    // renderMatrix(matrix, ROWS, COLS);
    // printf("%d", matrix[ROWS*COLS]);
    // free(seed);
  }

  ClearWindowState(FLAG_WINDOW_HIDDEN);

  SetWindowState(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_MAXIMIZED);
  SetWindowSize(monitorWidth, monitorHeight);
  SetWindowPosition(0, 0);
  SetTargetFPS(60);

  Color* pixel_buffer = (Color*)calloc(total_cells, sizeof(Color));
  if (!pixel_buffer) {
    free(current_grid);
    free(next_grid);
    CloseWindow();
    return 1;
  }

  Image screen_image = {.data = pixel_buffer,
                        .width = monitorWidth,
                        .height = monitorHeight,
                        .mipmaps = 1,
                        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
  Texture2D screen_texture = LoadTextureFromImage(screen_image);

  int generations = 0;
  while (!WindowShouldClose()) {
    renderGrid(current_grid, pixel_buffer, screen_texture, monitorWidth,
               monitorHeight);
    for (int r = 0; r < monitorHeight; r++) {
      int up = (r == 0) ? monitorHeight - 1 : r - 1;
      int down = (r == monitorHeight - 1) ? 0 : r + 1;
      int r_offset = r * monitorWidth;
      int up_offset = up * monitorWidth;
      int down_offset = down * monitorWidth;
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

      for (int c = 0; c < monitorWidth; c++) {
        int left = (c == 0) ? monitorWidth - 1 : c - 1;
        int right = (c == monitorWidth - 1) ? 0 : c + 1;

        int neighborCount =
            current_grid[up_offset + left] + current_grid[up_offset + c] +
            current_grid[up_offset + right] + current_grid[r_offset + left] +
            current_grid[r_offset + right] + current_grid[down_offset + left] +
            current_grid[down_offset + c] + current_grid[down_offset + right];

        int idx = r_offset + c;
        if (current_grid[idx] == 1) {
          // survive or die
          next_grid[idx] = (neighborCount == 2 || neighborCount == 3) ? 1 : 0;
        } else {
          // reproduce
          next_grid[idx] = (neighborCount == 3) ? 1 : 0;
        }
      }
    }

    char* temp = current_grid;
    current_grid = next_grid;
    next_grid = temp;
    generations++;
  }

  free(pixel_buffer);
  free(current_grid);
  free(next_grid);
  UnloadTexture(screen_texture);
  CloseWindow();
  printf("ran for %d generations", generations);
  return 0;
}

// fcyvgbhnjmkhubg vyftc drxctfgvhbjnkmlk,;m nkbjhgvhgbjhknm
// is interesting