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

void stampPreset(char* grid, int width, int height, const Preset* preset,
                 int start_col, int start_row) {
  for (int r = 0; r < preset->height; r++) {
    for (int c = 0; c < preset->width; c++) {
      int target_row = ((start_row + r) % height + height) % height;
      int target_col = ((start_col + c) % width + width) % width;
      int grid_idx = (target_row * width) + target_col;
      int blueprint_idx = (r * preset->width) + c;
      grid[grid_idx] = preset->grid_data[blueprint_idx];
    }
  }
}

void paintLine(char* grid, int width, int height, int c0, int r0, int c1,
               int r1, char val) {
  int dx = abs(c1 - c0);
  int sx = (c0 < c1) ? 1 : -1;
  int dy = -abs(r1 - r0);
  int sy = (r0 < r1) ? 1 : -1;
  int err = dx + dy;

  while (true) {
    if (c0 >= 0 && c0 < width && r0 >= 0 && r0 < height) {
      grid[r0 * width + c0] = val;
    }
    if (c0 == c1 && r0 == r1) break;
    int e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      c0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      r0 += sy;
    }
  }
}

// assumes columns as width
void renderGrid(const char* matrix, Color* pixel_buffer,
                Texture2D screen_texture, int width, int height,
                Camera2D camera, const Preset* presets, int available_presets,
                int active_preset_idx, bool menu_open, bool paused,
                int generations, int hover_col, int hover_row,
                bool mouse_in_ui) {
  for (int i = 0; i < width * height; i++) {
    pixel_buffer[i] = (matrix[i] == 1) ? WHITE : BLACK;
  }

  if (active_preset_idx >= 0 && active_preset_idx < available_presets &&
      !mouse_in_ui) {
    const Preset* p = &presets[active_preset_idx];
    int start_col = hover_col - (p->width / 2);
    int start_row = hover_row - (p->height / 2);
    for (int r = 0; r < p->height; r++) {
      for (int c = 0; c < p->width; c++) {
        int tr = ((start_row + r) % height + height) % height;
        int tc = ((start_col + c) % width + width) % width;
        int g_idx = tr * width + tc;
        int b_idx = r * p->width + c;
        if (p->grid_data[b_idx] == 1) {
          pixel_buffer[g_idx] = GREEN;
        } else if (matrix[g_idx] == 0) {
          pixel_buffer[g_idx] = (Color){25, 45, 25, 255};
        }
      }
    }
  }

  UpdateTexture(screen_texture, pixel_buffer);

  BeginDrawing();
  ClearBackground(BLACK);

  BeginMode2D(camera);
  DrawTexturePro(screen_texture, (Rectangle){0, 0, (float)width, (float)height},
                 (Rectangle){0, 0, (float)width, (float)height},
                 (Vector2){0, 0}, 0.0f, WHITE);
  EndMode2D();

  DrawRectangle(10, 10, 220, 30, (Color){20, 20, 20, 220});
  DrawRectangleLines(10, 10, 220, 30, GRAY);
  DrawText(menu_open ? "[-] presets & controls" : "[+] presets & controls", 20,
           18, 16, WHITE);

  if (menu_open) {
    int panel_h = 95 + (available_presets * 28);
    DrawRectangle(10, 42, 220, panel_h, (Color){15, 15, 15, 230});
    DrawRectangleLines(10, 42, 220, panel_h, GRAY);

    DrawText(paused ? "state: PAUSED (space)" : "state: RUNNING (space)", 20,
             50, 14, paused ? YELLOW : GREEN);
    DrawText(TextFormat("gen: %d | zoom: %.1fx", generations, camera.zoom), 20,
             68, 14, LIGHTGRAY);

    Rectangle clear_btn = {18, 90, 204, 24};
    DrawRectangleRec(clear_btn, (Color){60, 25, 25, 255});
    DrawRectangleLinesEx(clear_btn, 1, RED);
    DrawText("clear universe (c)", 26, 95, 14, WHITE);

    DrawText("spawn preset:", 20, 120, 14, GRAY);
    for (int i = 0; i < available_presets; i++) {
      Rectangle item_rect = {18, (float)(138 + i * 26), 204, 24};
      bool is_selected = (active_preset_idx == i);
      DrawRectangleRec(item_rect, is_selected ? (Color){30, 80, 30, 255}
                                              : (Color){35, 35, 35, 255});
      DrawRectangleLinesEx(item_rect, 1, is_selected ? GREEN : DARKGRAY);
      DrawText(presets[i].name, 24, 143 + i * 26, 14,
               is_selected ? GREEN : WHITE);
    }
  }

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

  FilePathList files = LoadDirectoryFiles("./presets");
  int available_presets = 0;
  Preset preset_list[MAX_PRESETS];
  for (unsigned int i = 0; i < files.count && available_presets < MAX_PRESETS;
       i++) {
    if (IsFileExtension(files.paths[i], ".dat") ||
        IsFileExtension(files.paths[i], ".txt")) {
      if (loadPresetFile(files.paths[i], &preset_list[available_presets])) {
        available_presets++;
      }
    }
  }
  UnloadDirectoryFiles(files);

  printf("===============================\n");
  printf("      conways game of life\n");
  printf("===============================\n");
  printf("1. input a custom string seed\n");
  printf("2. choose a preset\n");
  printf("3. random noise\n");
  printf("4. empty universe\n");
  printf("select 1-4: ");

  char menu_choice[16];
  if (!fgets(menu_choice, sizeof(menu_choice), stdin)) {
    for (int i = 0; i < available_presets; i++) free(preset_list[i].grid_data);
    free(current_grid);
    free(next_grid);
    CloseWindow();
    return 1;
  }

  if (menu_choice[0] == '2') {
    printf("\navailable presets:\n");
    for (int i = 0; i < available_presets; i++) {
      printf("%d. %s\n", i + 1, preset_list[i].name);
    }

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
        for (int i = 0; i < available_presets; i++)
          free(preset_list[i].grid_data);
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
      stampPreset(current_grid, monitorWidth, monitorHeight, &active, start_col,
                  start_row);
      printf("\nloaded %s\n", active.name);
    }
  } else if (menu_choice[0] == '3') {
    for (int i = 0; i < total_cells; i++) {
      current_grid[i] = (rand() % 100 < 30) ? 1 : 0;
    }
  } else if (menu_choice[0] == '4') {
    memset(current_grid, 0, total_cells * sizeof(char));
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
    for (int i = 0; i < available_presets; i++) free(preset_list[i].grid_data);
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
  SetTextureFilter(screen_texture, TEXTURE_FILTER_POINT);

  Camera2D camera = {0};
  camera.zoom = 1.0f;

  bool paused = (menu_choice[0] == '4');
  bool menu_open = false;
  int active_preset_idx = -1;
  bool is_drawing_cells = false;
  char draw_val = 1;
  int prev_draw_col = -1;
  int prev_draw_row = -1;

  int generations = 0;
  while (!WindowShouldClose()) {
    Vector2 mouse_screen = GetMousePosition();
    Vector2 mouse_world = GetScreenToWorld2D(mouse_screen, camera);
    int hover_col = (int)mouse_world.x;
    int hover_row = (int)mouse_world.y;

    Rectangle toggle_btn = {10, 10, 220, 30};
    int panel_h = 95 + (available_presets * 28);
    Rectangle panel_rect = {10, 42, 220, (float)panel_h};
    bool mouse_in_ui =
        CheckCollisionPointRec(mouse_screen, toggle_btn) ||
        (menu_open && CheckCollisionPointRec(mouse_screen, panel_rect));

    if (IsKeyPressed(KEY_SPACE)) paused = !paused;
    if (IsKeyPressed(KEY_TAB)) menu_open = !menu_open;
    if (IsKeyPressed(KEY_C)) {
      memset(current_grid, 0, total_cells * sizeof(char));
      generations = 0;
    }
    if (IsKeyPressed(KEY_R)) {
      camera.zoom = 1.0f;
      camera.target = (Vector2){0, 0};
      camera.offset = (Vector2){0, 0};
    }

    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f && !mouse_in_ui) {
      camera.offset = mouse_screen;
      camera.target = mouse_world;
      camera.zoom *= (1.0f + wheel * 0.15f);
      if (camera.zoom < 1.0f) {
        camera.zoom = 1.0f;
        camera.target = (Vector2){0, 0};
        camera.offset = (Vector2){0, 0};
      }
      if (camera.zoom > 64.0f) camera.zoom = 64.0f;
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) ||
        IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
      Vector2 delta = GetMouseDelta();
      if (delta.x != 0.0f || delta.y != 0.0f) {
        camera.target.x -= delta.x / camera.zoom;
        camera.target.y -= delta.y / camera.zoom;
      }
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && active_preset_idx != -1) {
      active_preset_idx = -1;
    }
    // printf("%d", menu_open);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      if (CheckCollisionPointRec(mouse_screen, toggle_btn)) {
        menu_open = !menu_open;
      } else if (menu_open &&
                 CheckCollisionPointRec(mouse_screen, panel_rect)) {
        Rectangle clear_btn = {18, 90, 204, 24};
        if (CheckCollisionPointRec(mouse_screen, clear_btn)) {
          memset(current_grid, 0, total_cells * sizeof(char));
          generations = 0;
        }
        for (int i = 0; i < available_presets; i++) {
          Rectangle item_rect = {18, (float)(138 + i * 26), 204, 24};
          if (CheckCollisionPointRec(mouse_screen, item_rect)) {
            active_preset_idx = (active_preset_idx == i) ? -1 : i;
          }
        }
      } else if (active_preset_idx >= 0 &&
                 active_preset_idx < available_presets) {
        const Preset* p = &preset_list[active_preset_idx];
        int start_col = hover_col - (p->width / 2);
        int start_row = hover_row - (p->height / 2);
        stampPreset(current_grid, monitorWidth, monitorHeight, p, start_col,
                    start_row);
        if (!IsKeyDown(KEY_LEFT_SHIFT)) {
          active_preset_idx = -1;
        }
      } else if (hover_col >= 0 && hover_col < monitorWidth && hover_row >= 0 &&
                 hover_row < monitorHeight) {
        is_drawing_cells = true;
        int idx = hover_row * monitorWidth + hover_col;
        draw_val = (current_grid[idx] == 1) ? 0 : 1;
        current_grid[idx] = draw_val;
        prev_draw_col = hover_col;
        prev_draw_row = hover_row;
      }
    }

    // prev_draw_col = hover_col;
    if (is_drawing_cells) {
      if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        if (prev_draw_col >= 0 && prev_draw_row >= 0) {
          paintLine(current_grid, monitorWidth, monitorHeight, prev_draw_col,
                    prev_draw_row, hover_col, hover_row, draw_val);
        }
        prev_draw_col = hover_col;
        prev_draw_row = hover_row;
      } else {
        is_drawing_cells = false;
        prev_draw_col = -1;
        prev_draw_row = -1;
      }
    }

    renderGrid(current_grid, pixel_buffer, screen_texture, monitorWidth,
               monitorHeight, camera, preset_list, available_presets,
               active_preset_idx, menu_open, paused, generations, hover_col,
               hover_row, mouse_in_ui);

    bool step_once = paused && IsKeyPressed(KEY_S);
    if ((!paused && !is_drawing_cells) || step_once) {
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
              current_grid[r_offset + right] +
              current_grid[down_offset + left] + current_grid[down_offset + c] +
              current_grid[down_offset + right];

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
  }

  for (int i = 0; i < available_presets; i++) {
    free(preset_list[i].grid_data);
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