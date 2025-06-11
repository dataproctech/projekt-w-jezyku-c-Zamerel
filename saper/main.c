#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>      // dla srand()/rand()
#include <math.h>      // dla fminf()
#include <allegro5/allegro_native_dialog.h>
#include <direct.h>    // dla _mkdir
#include <shlobj.h>    // dla SHGetFolderPath
#include <windows.h>   // dla FindFirstFile, FindNextFile
#include <commdlg.h>   // dla GetOpenFileName
#include "file_dialog.h"


#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_image.h>

// --- Ustawienia początkowe ---
int window_width;
int window_height;
int grid_cols     = 10;
int grid_rows     = 10;
int next_mines    = 25;

// Tymczasowe wartości dla nowej gry
int temp_cols     = 10;
int temp_rows     = 10;
int temp_mines    = 25;

int seconds       = 0;
int minutes       = 0;
int mines_left    = 25;

bool in_ws_menu    = true;
bool in_setup_menu = false;
bool in_input      = false;
bool game_over     = false;
int  input_target  = -1;
char input_buffer[32] = "";

// --- Rozmiary czcionek (piksele) ---
int sz_header = 32;  // przyciski, input
int sz_info   = 24;  // timer, mines, reset

// --- Struktura stanu pola ---
typedef struct {
    bool has_mine;
    bool revealed;
    bool flagged;
    int  adj_mines;
} Cell;

Cell **grid = NULL;  // dwuwymiarowa tablica

// Etykiety menu
// + trzy zmienne dla kolejnej gry
int new_cols  = 10;
int new_rows  = 10;
int new_mines = 10;

// Etykiety menu głównego
enum WSButtons   { WS_HEIGHT = 0, WS_WIDTH, WS_NEW, WS_SAVE, WS_LOAD, WS_COUNT };
const char *ws_labels[] = { "Height", "Width", "New Game", "Save", "Load" };

enum SetupButtons {
    SET_COLS = 0,
    SET_ROWS,
    SET_MINES,
    SET_START,    
    SET_BACK,
    SET_COUNT
};
const char *setup_labels[] = {
    "Columns",
    "Rows",
    "Mines",
    "Start New Game", 
    "Back"
};


typedef struct { float x, y, w, h; const char *label; } Button;
typedef struct { Button *arr; int count; } ButtonList;

// Allegro
ALLEGRO_DISPLAY     *display      = NULL;
ALLEGRO_FONT        *font_header  = NULL;
ALLEGRO_FONT        *font_info    = NULL;
ALLEGRO_EVENT_QUEUE *queue        = NULL;
ALLEGRO_TIMER       *frame_timer  = NULL;
ALLEGRO_TIMER       *second_timer = NULL;

Button header_buf[(SET_COUNT > WS_COUNT) ? SET_COUNT : WS_COUNT];
ButtonList header;
Button reset_button;

// Procentowe wysokości pasków
const float HEADER_PCT = 0.10f;
const float INFO_PCT   = 0.10f;

// Dodaj nowe zmienne globalne
char save_filename[256] = "";
bool in_save_input = false;

// Struktura do przechowywania listy plików
typedef struct {
    char** files;
    int count;
} FileList;

// --- Zmienne globalne dla rozdzielczości monitora ---
int monitor_min_width = 500;
int monitor_max_width = 0;
int monitor_min_height = 500;
int monitor_max_height = 0;

char app_dir[MAX_PATH] = ".";

// Deklaracje funkcji
void resize_window(int w, int h);
void init_grid(void);
void free_grid(void);
bool save_game_state(const char* filename);
bool load_game_state(const char* filename);
void destroy_allegro(void);
void setup_buttons(void);
bool click(const Button *b, float x, float y);
void draw_header(void);
void draw_info(void);
void draw_game(void);
void draw_input(void);
void place_mines(void);
void reveal_empty_cells(int row, int col);
int count_adjacent_mines(int row, int col);
bool check_win_condition(void);

// Funkcja do pobrania ścieżki do folderu Dokumenty
void get_documents_path(char* path, size_t size) {
    char documents[MAX_PATH];
    SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, documents);
    snprintf(path, size, "%s\\Minesweeper", documents);
}

// Funkcja do zapisywania stanu gry
bool save_game_state(const char* filename) {
    char documents_path[MAX_PATH];
    get_documents_path(documents_path, MAX_PATH);
    
    // Utwórz folder jeśli nie istnieje
    _mkdir(documents_path);
    
    // Utwórz pełną ścieżkę do pliku
    char full_path[MAX_PATH];
    snprintf(full_path, MAX_PATH, "%s\\%s.mine", documents_path, filename);
    
    FILE* file = fopen(full_path, "wb");
    if (!file) return false;
    
    // Zapisz podstawowe informacje
    fwrite(&window_width, sizeof(int), 1, file);
    fwrite(&window_height, sizeof(int), 1, file);
    fwrite(&seconds, sizeof(int), 1, file);
    fwrite(&minutes, sizeof(int), 1, file);
    fwrite(&mines_left, sizeof(int), 1, file);
    fwrite(&grid_cols, sizeof(int), 1, file);
    fwrite(&grid_rows, sizeof(int), 1, file);
    fwrite(&next_mines, sizeof(int), 1, file);
    
    // Zapisz stan siatki (zakodowany)
    for (int r = 0; r < grid_rows; r++) {
        for (int c = 0; c < grid_cols; c++) {
            // Kodujemy stan kratki w jednym bajcie
            unsigned char cell_state = 0;
            if (grid[r][c].has_mine) cell_state |= 0x01;
            if (grid[r][c].revealed) cell_state |= 0x02;
            if (grid[r][c].flagged) cell_state |= 0x04;
            cell_state |= (grid[r][c].adj_mines << 3); // 5 bitów na liczbę sąsiednich min
            
            fwrite(&cell_state, sizeof(unsigned char), 1, file);
        }
    }
    
    fclose(file);
    return true;
}

// Funkcja do wczytywania stanu gry
bool load_game_state(const char* filename) {
    char documents_path[MAX_PATH];
    get_documents_path(documents_path, MAX_PATH);
    // Utwórz pełną ścieżkę do pliku
    char full_path[MAX_PATH];
    size_t len = strlen(filename);
    if (len >= 5 && strcmp(filename + len - 5, ".mine") == 0) {
        snprintf(full_path, MAX_PATH, "%s\\%s", documents_path, filename);
    } else {
        snprintf(full_path, MAX_PATH, "%s\\%s.mine", documents_path, filename);
    }
    FILE* file = fopen(full_path, "rb");
    if (!file) return false;

    // Wczytaj podstawowe informacje z kontrolą błędów
    if (fread(&window_width, sizeof(int), 1, file) != 1 ||
        fread(&window_height, sizeof(int), 1, file) != 1 ||
        fread(&seconds, sizeof(int), 1, file) != 1 ||
        fread(&minutes, sizeof(int), 1, file) != 1 ||
        fread(&mines_left, sizeof(int), 1, file) != 1 ||
        fread(&grid_cols, sizeof(int), 1, file) != 1 ||
        fread(&grid_rows, sizeof(int), 1, file) != 1 ||
        fread(&next_mines, sizeof(int), 1, file) != 1) {
        fclose(file);
        al_show_native_message_box(display, "Błąd", "Błąd wczytywania", "Plik zapisu jest uszkodzony (nagłówek)", NULL, ALLEGRO_MESSAGEBOX_ERROR);
        return false;
    }
    // Sprawdź zakresy
    if (grid_cols < 5 || grid_cols > 50 || grid_rows < 5 || grid_rows > 50) {
        fclose(file);
        al_show_native_message_box(display, "Błąd", "Błąd wczytywania", "Nieprawidłowy rozmiar planszy w pliku zapisu", NULL, ALLEGRO_MESSAGEBOX_ERROR);
        return false;
    }
    // Zmień rozmiar okna
    resize_window(window_width, window_height);
    // Zwolnij starą siatkę i utwórz nową
    free_grid();
    init_grid();
    // Wczytaj stan siatki z kontrolą błędów
    for (int r = 0; r < grid_rows; r++) {
        for (int c = 0; c < grid_cols; c++) {
            unsigned char cell_state;
            if (fread(&cell_state, sizeof(unsigned char), 1, file) != 1) {
                fclose(file);
                al_show_native_message_box(display, "Błąd", "Błąd wczytywania", "Plik zapisu jest uszkodzony (siatka)", NULL, ALLEGRO_MESSAGEBOX_ERROR);
                return false;
            }
            grid[r][c].has_mine = (cell_state & 0x01) != 0;
            grid[r][c].revealed = (cell_state & 0x02) != 0;
            grid[r][c].flagged = (cell_state & 0x04) != 0;
            grid[r][c].adj_mines = (cell_state >> 3) & 0x1F;
        }
    }
    fclose(file);
    return true;
}

// Prototypy
void init_allegro();
void destroy_allegro();
void setup_buttons();
void draw_header();
void draw_info();
void draw_game();
void draw_input();

void place_mines();
void reveal_empty_cells(int row, int col);

// Dodaj nową funkcję do liczenia sąsiednich min
int count_adjacent_mines(int row, int col) {
    int count = 0;
    for (int r = -1; r <= 1; r++) {
        for (int c = -1; c <= 1; c++) {
            int new_row = row + r;
            int new_col = col + c;
            if (new_row >= 0 && new_row < grid_rows && 
                new_col >= 0 && new_col < grid_cols && 
                grid[new_row][new_col].has_mine) {
                count++;
            }
        }
    }
    return count;
}

bool check_win_condition() {
    // Sprawdź czy wszystkie miny są oflagowane
    for (int r = 0; r < grid_rows; r++) {
        for (int c = 0; c < grid_cols; c++) {
            if (grid[r][c].has_mine && !grid[r][c].flagged) {
                return false;
            }
        }
    }
    
    // Sprawdź czy wszystkie nie-miny są odkryte
    for (int r = 0; r < grid_rows; r++) {
        for (int c = 0; c < grid_cols; c++) {
            if (!grid[r][c].has_mine && !grid[r][c].revealed) {
                return false;
            }
        }
    }
    
    // Sprawdź czy licznik min wynosi 0
    return mines_left == 0;
}

// Funkcja do wyszukiwania plików .mine
FileList find_mine_files(void) {
    FileList list = {NULL, 0};
    char documents_path[MAX_PATH];
    get_documents_path(documents_path, MAX_PATH);
    
    // Utwórz ścieżkę do wyszukiwania
    char search_path[MAX_PATH];
    snprintf(search_path, MAX_PATH, "%s\\*.mine", documents_path);
    
    WIN32_FIND_DATA find_data;
    HANDLE hFind = FindFirstFile(search_path, &find_data);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            // Pomijamy katalogi
            if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                // Alokuj pamięć dla nowego pliku
                char** new_files = realloc(list.files, (list.count + 1) * sizeof(char*));
                if (new_files) {
                    list.files = new_files;
                    list.files[list.count] = strdup(find_data.cFileName);
                    if (list.files[list.count]) {
                        list.count++;
                    }
                }
            }
        } while (FindNextFile(hFind, &find_data));
        FindClose(hFind);
    }
    
    return list;
}

// Funkcja do zwalniania pamięci listy plików
void free_file_list(FileList* list) {
    if (list->files) {
        for (int i = 0; i < list->count; i++) {
            free(list->files[i]);
        }
        free(list->files);
        list->files = NULL;
        list->count = 0;
    }
}

// Funkcja do wyświetlania okna wyboru pliku
const char* show_file_selection_dialog(void) {
    char documents_path[MAX_PATH];
    get_documents_path(documents_path, MAX_PATH);
    
    // Przygotuj filtr plików
    char filter[] = "Minesweeper Save Files (*.mine)\0*.mine\0All Files (*.*)\0*.*\0";
    
    // Przygotuj strukturę okna dialogowego
    OPENFILENAME ofn = {0};
    char szFile[260] = {0};
    
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = documents_path;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    
    // Wyświetl okno wyboru pliku
    if (GetOpenFileName(&ofn)) {
        // Znajdź ostatni znak '\' w ścieżce
        char* last_slash = strrchr(ofn.lpstrFile, '\\');
        if (last_slash) {
            // Użyj tylko nazwy pliku bez ścieżki
            return strdup(last_slash + 1);
        }
        return strdup(ofn.lpstrFile);
    }
    
    return NULL;
}

int main(void) {
    // Ustal katalog aplikacji
    GetModuleFileNameA(NULL, app_dir, MAX_PATH);
    char *last_slash = strrchr(app_dir, '\\');
    if (last_slash) *last_slash = 0;
    init_allegro();
    srand((unsigned)time(NULL));

    init_grid();
    place_mines();
    game_over = false;

    bool redraw = true;
    while (true) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(queue, &ev);

        switch (ev.type) {
            case ALLEGRO_EVENT_DISPLAY_CLOSE:
                free_grid();
                destroy_allegro();
                return 0;

            case ALLEGRO_EVENT_TIMER:
                if (ev.timer.source == second_timer) {
                    if (!game_over) {
                        seconds++;
                        if (seconds >= 60) {
                            seconds = 0;
                            minutes++;
                        }
                        redraw = true;
                    }
                } else if (ev.timer.source == frame_timer) {
                    if (redraw) {
                        al_clear_to_color(al_map_rgb(220,220,220));
                        draw_header();
                        draw_info();
                        draw_game();
                        if (in_input) draw_input();
                        al_flip_display();
                        redraw = false;
                    }
                }
                break;

            case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN: {
                float mx = ev.mouse.x, my = ev.mouse.y;
                bool consumed = false;
                float header_h = window_height * HEADER_PCT;
                float info_h   = window_height * INFO_PCT;
                float info_y   = header_h;

                // Zamknięcie okna input po kliknięciu X
                if (in_input) {
                    const int w = 300, h = 100;
                    int x = (window_width - w) / 2;
                    int y = (window_height - h) / 2;
                    int x_btn_size = 24;
                    int x_btn_x = x + w - x_btn_size - 4;
                    int x_btn_y = y + 4;
                    if (mx >= x_btn_x && mx <= x_btn_x + x_btn_size && my >= x_btn_y && my <= x_btn_y + x_btn_size) {
                        in_input = false;
                        redraw = true;
                        break;
                    }
                }

                // Header clicks
                if (!in_input && my < header_h) {
                    for (int i = 0; i < header.count; i++) {
                        if (click(&header.arr[i], mx, my)) {
                            if (in_ws_menu) {
                                switch (i) {
                                    case WS_HEIGHT:
                                    case WS_WIDTH:
                                        in_input = true;
                                        input_target = i;
                                        input_buffer[0] = '\0';
                                        break;
                                    case WS_NEW:
                                        in_setup_menu = true;
                                        in_ws_menu    = false;
                                        temp_cols = grid_cols;
                                        temp_rows = grid_rows;
                                        temp_mines = next_mines;
                                        setup_buttons();
                                        break;
                                    case WS_SAVE:
                                        in_input = true;
                                        in_save_input = true;
                                        input_target = i;
                                        input_buffer[0] = '\0';
                                        break;
                                    case WS_LOAD:
                                        {
                                            const char* selected_file = show_file_selection_dialog();
                                            if (selected_file) {
                                                if (load_game_state(selected_file)) {
                                                    al_show_native_message_box(
                                                        display,
                                                        "Success",
                                                        "Game Loaded",
                                                        "Game state has been loaded successfully!",
                                                        NULL,
                                                        ALLEGRO_MESSAGEBOX_WARN
                                                    );
                                                } else {
                                                    al_show_native_message_box(
                                                        display,
                                                        "Error",
                                                        "Load Failed",
                                                        "Failed to load game state!",
                                                        NULL,
                                                        ALLEGRO_MESSAGEBOX_WARN
                                                    );
                                                }
                                                free((void*)selected_file);
                                            }
                                        }
                                        break;
                                }
                            } else {
                                if (i == SET_BACK) {
                                    in_setup_menu = false;
                                    in_ws_menu    = true;
                                    setup_buttons();
                                } else if (i == SET_START) {
                                    // Zapisz stare wartości
                                    int old_cols = grid_cols;
                                    int old_rows = grid_rows;
                                    int old_mines = next_mines;
                                    
                                    // Ustaw nowe wartości
                                    grid_cols = temp_cols;
                                    grid_rows = temp_rows;
                                    next_mines = temp_mines;
                                    mines_left = temp_mines;
                                    
                                    // Zwolnij starą siatkę
                                    free_grid();
                                    
                                    // Spróbuj zainicjalizować nową siatkę
                                    init_grid();
                                    if (grid) {
                                        place_mines();
                                        game_over = false;
                                        minutes = seconds = 0;
                                        al_start_timer(second_timer);
                                        in_setup_menu = false;
                                        in_ws_menu = true;
                                        setup_buttons();
                                    } else {
                                        // Jeśli inicjalizacja się nie powiodła, przywróć stare wartości
                                        grid_cols = old_cols;
                                        grid_rows = old_rows;
                                        next_mines = old_mines;
                                        mines_left = old_mines;
                                        init_grid();
                                        place_mines();
                                    }
                                } else {
                                    in_input = true;
                                    input_target = i;
                                    input_buffer[0] = '\0';
                                }
                            }
                            consumed = true;
                            break;
                        }
                    }
                }

                // Info clicks (Reset)
                if (!consumed && !in_input && my >= info_y && my < info_y + info_h) {
                    if (click(&reset_button, mx, my)) {
                        minutes = seconds = 0;
                        place_mines();
                        game_over = false;
                        al_start_timer(second_timer);
                        consumed = true;
                    }
                }

                // Grid click LPM
                if (!consumed && !in_input && ev.mouse.button == 1 && !game_over) {
                    float avail_w  = (float)window_width;
                    float avail_h  = (float)window_height - header_h - info_h;
                    float cell     = fminf(avail_w / grid_cols, avail_h / grid_rows);
                    float grid_w   = cell * grid_cols;
                    float grid_h   = cell * grid_rows;
                    float offset_x = (avail_w - grid_w) / 2.0f;
                    float offset_y = header_h + info_h + (avail_h - grid_h) / 2.0f;

                    if (mx >= offset_x && mx < offset_x + grid_w &&
                        my >= offset_y && my < offset_y + grid_h) {
                        int c_idx = (mx - offset_x) / cell;
                        int r_idx = (my - offset_y) / cell;
                        if (!grid[r_idx][c_idx].revealed && !grid[r_idx][c_idx].flagged) {
                            if (grid[r_idx][c_idx].has_mine) {
                                game_over = true;
                                al_stop_timer(second_timer);
                                al_show_native_message_box(
                                    display,
                                    "Game Over",
                                    "Boom!",
                                    "You clicked on a mine!",
                                    NULL,
                                    ALLEGRO_MESSAGEBOX_WARN
                                );
                                redraw = true;
                            } else {
                                reveal_empty_cells(r_idx, c_idx);
                                consumed = true;
                                redraw = true;
                                
                                // Sprawdź warunki zwycięstwa
                                if (check_win_condition()) {
                                    game_over = true;
                                    al_stop_timer(second_timer);
                                    al_show_native_message_box(
                                        display,
                                        "Victory!",
                                        "Congratulations!",
                                        "You have won the game!",
                                        NULL,
                                        ALLEGRO_MESSAGEBOX_WARN
                                    );
                                }
                            }
                        }
                    }
                }

                // Grid click PPM
                if (!consumed && !in_input && ev.mouse.button == 2 && !game_over) {
                    float avail_w  = (float)window_width;
                    float avail_h  = (float)window_height - header_h - info_h;
                    float cell     = fminf(avail_w / grid_cols, avail_h / grid_rows);
                    float grid_w   = cell * grid_cols;
                    float grid_h   = cell * grid_rows;
                    float offset_x = (avail_w - grid_w) / 2.0f;
                    float offset_y = header_h + info_h + (avail_h - grid_h) / 2.0f;

                    if (mx >= offset_x && mx < offset_x + grid_w &&
                        my >= offset_y && my < offset_y + grid_h) {
                        int c_idx = (mx - offset_x) / cell;
                        int r_idx = (my - offset_y) / cell;
                        if (!grid[r_idx][c_idx].revealed) {
                            grid[r_idx][c_idx].flagged = !grid[r_idx][c_idx].flagged;
                            if (grid[r_idx][c_idx].flagged) {
                                mines_left--;
                            } else {
                                mines_left++;
                            }
                            consumed = true;
                            redraw = true;
                            
                            // Sprawdź warunki zwycięstwa
                            if (check_win_condition()) {
                                game_over = true;
                                al_stop_timer(second_timer);
                                al_show_native_message_box(
                                    display,
                                    "Victory!",
                                    "Congratulations!",
                                    "You have won the game!",
                                    NULL,
                                    ALLEGRO_MESSAGEBOX_WARN
                                );
                            }
                        }
                    }
                }

                if (consumed) redraw = true;
                break;
            }

            case ALLEGRO_EVENT_KEY_CHAR:
                if (in_input) {
                    if (ev.keyboard.keycode == ALLEGRO_KEY_ENTER) {
                        int v;
                        if (in_ws_menu && input_target == WS_SAVE) {
                            if (strlen(input_buffer) > 0) {
                                if (save_game_state(input_buffer)) {
                                    al_show_native_message_box(
                                        display,
                                        "Success",
                                        "Game Saved",
                                        "Game state has been saved successfully!",
                                        NULL,
                                        ALLEGRO_MESSAGEBOX_WARN
                                    );
                                } else {
                                    al_show_native_message_box(
                                        display,
                                        "Error",
                                        "Save Failed",
                                        "Failed to save game state!",
                                        NULL,
                                        ALLEGRO_MESSAGEBOX_WARN
                                    );
                                }
                            }
                            in_input = false;
                            redraw = true;
                        }
                        else if (in_ws_menu && input_target == WS_LOAD) {
                            if (strlen(input_buffer) > 0) {
                                if (load_game_state(input_buffer)) {
                                    al_show_native_message_box(
                                        display,
                                        "Success",
                                        "Game Loaded",
                                        "Game state has been loaded successfully!",
                                        NULL,
                                        ALLEGRO_MESSAGEBOX_WARN
                                    );
                                } else {
                                    al_show_native_message_box(
                                        display,
                                        "Error",
                                        "Load Failed",
                                        "Failed to load game state!",
                                        NULL,
                                        ALLEGRO_MESSAGEBOX_WARN
                                    );
                                }
                            }
                            in_input = false;
                            redraw = true;
                        }
                        else if (in_ws_menu &&
                            (input_target == WS_HEIGHT || input_target == WS_WIDTH)) {
                            if (sscanf(input_buffer, "%d", &v) == 1) {
                                if (input_target == WS_HEIGHT && v >= monitor_min_height && v <= monitor_max_height)
                                    window_height = v;
                                else if (input_target == WS_WIDTH && v >= monitor_min_width && v <= monitor_max_width)
                                    window_width = v;
                                resize_window(window_width, window_height);
                            }
                            in_input = false;
                            redraw = true;
                        }
                        else if (in_setup_menu) {
                            if (sscanf(input_buffer, "%d", &v) == 1) {
                                if (input_target == SET_COLS && v >= 5 && v <= 50) {
                                    temp_cols = v;
                                }
                                else if (input_target == SET_ROWS && v >= 5 && v <= 50) {
                                    temp_rows = v;
                                }
                                else if (input_target == SET_MINES && v >= 1 && v < temp_cols * temp_rows) {
                                    temp_mines = v;
                                }
                                else if (input_target == SET_START) {
                                    grid_cols = temp_cols;
                                    grid_rows = temp_rows;
                                    next_mines = mines_left = temp_mines;
                                    free_grid();
                                    init_grid();
                                    place_mines();
                                    game_over = false;
                                    in_setup_menu = false;
                                    in_ws_menu = true;
                                    setup_buttons();
                                }
                            }
                            in_input = false;
                            redraw = true;
                        }
                    }
                    else if (ev.keyboard.keycode == ALLEGRO_KEY_BACKSPACE &&
                             strlen(input_buffer) > 0) {
                        input_buffer[strlen(input_buffer)-1] = '\0';
                        redraw = true;
                    }
                    else if (ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                        in_input = false;
                        redraw = true;
                        break;
                    }
                    else if (isprint(ev.keyboard.unichar) &&
                             strlen(input_buffer) < sizeof(input_buffer)-1) {
                        size_t len = strlen(input_buffer);
                        input_buffer[len] = ev.keyboard.unichar;
                        input_buffer[len+1] = '\0';
                        redraw = true;
                    }
                }
                break;
        }
    }

    return 0;
}

void init_grid() {
    // Najpierw zwolnij starą siatkę jeśli istnieje
    if (grid) {
        for (int r = 0; r < grid_rows; r++) {
            if (grid[r]) {
                free(grid[r]);
            }
        }
        free(grid);
        grid = NULL;
    }
    
    // Sprawdź czy wartości są prawidłowe
    if (grid_rows <= 0 || grid_cols <= 0) {
        fprintf(stderr, "Nieprawidłowe wymiary siatki: %d x %d\n", grid_rows, grid_cols);
        exit(1);
    }
    
    // Alokuj nową siatkę jako jeden ciągły blok pamięci
    size_t total_size = grid_rows * grid_cols * sizeof(Cell);
    Cell* data = malloc(total_size);
    if (!data) {
        fprintf(stderr, "Brak pamieci na dane siatki\n");
        exit(1);
    }
    
    // Alokuj tablicę wskaźników
    grid = malloc(grid_rows * sizeof(Cell*));
    if (!grid) {
        free(data);
        fprintf(stderr, "Brak pamieci na tablice wskaźników\n");
        exit(1);
    }
    
    // Ustaw wskaźniki na odpowiednie miejsca w ciągłym bloku
    for (int r = 0; r < grid_rows; r++) {
        grid[r] = data + (r * grid_cols);
    }
    
    // Inicjalizuj wszystkie komórki na zero
    memset(data, 0, total_size);
}

void free_grid() {
    if (!grid) return;
    
    // Zwolnij ciągły blok danych (jest to pierwszy wiersz)
    if (grid[0]) {
        free(grid[0]);
    }
    
    // Zwolnij tablicę wskaźników
    free(grid);
    grid = NULL;
}

void place_mines() {
    for (int r = 0; r < grid_rows; r++) {
        for (int c = 0; c < grid_cols; c++) {
            grid[r][c].has_mine = false;
            grid[r][c].revealed = false;
            grid[r][c].flagged  = false;
            grid[r][c].adj_mines= 0;
        }
    }
    mines_left = next_mines;

    int placed = 0;
    while (placed < next_mines) {
        int r = rand() % grid_rows;
        int c = rand() % grid_cols;
        if (!grid[r][c].has_mine) {
            grid[r][c].has_mine = true;
            placed++;
        }
    }
}

void reveal_empty_cells(int row, int col) {
    // Sprawdź czy kratka jest w granicach planszy
    if (row < 0 || row >= grid_rows || col < 0 || col >= grid_cols) {
        return;
    }
    
    // Jeśli kratka jest już odkryta lub ma flagę, nic nie rób
    if (grid[row][col].revealed || grid[row][col].flagged) {
        return;
    }
    
    // Policz sąsiednie miny
    int adjacent = count_adjacent_mines(row, col);
    grid[row][col].adj_mines = adjacent;
    
    // Odkryj kratkę
    grid[row][col].revealed = true;
    
    // Jeśli kratka nie ma sąsiednich min, odkryj sąsiednie kratki
    if (adjacent == 0) {
        // Sprawdź wszystkie 8 sąsiednich kratek
        for (int r = -1; r <= 1; r++) {
            for (int c = -1; c <= 1; c++) {
                if (r == 0 && c == 0) continue; // Pomijamy aktualną kratkę
                reveal_empty_cells(row + r, col + c);
            }
        }
    }
}

void init_allegro() {
    if (!al_init()) { fprintf(stderr, "Nie mozna zainicjalizowac Allegro\n"); exit(1); }
    al_install_mouse();
    al_install_keyboard();
    al_init_primitives_addon();
    al_init_font_addon();
    al_init_ttf_addon();
    al_init_image_addon();
    al_init_native_dialog_addon();

    ALLEGRO_MONITOR_INFO m;
    al_get_monitor_info(0, &m);
    window_width  = (int)((m.x2 - m.x1) * 0.3f);
    window_height = (int)((m.y2 - m.x1) * 0.6f);
    // Ustal limity na podstawie rozdzielczości monitora
    monitor_max_width = m.x2 - m.x1;
    monitor_max_height = m.y2 - m.x1;

    display = al_create_display(window_width, window_height);
    if (!display) { fprintf(stderr, "Blad tworzenia display\n"); exit(1); }

    setup_buttons();

    al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
    char font_path[MAX_PATH];
    snprintf(font_path, MAX_PATH, "%s\\arial.ttf", app_dir);
    font_header = al_load_ttf_font(font_path, -sz_header, 0);
    font_info   = al_load_ttf_font(font_path, -sz_info,   0);
    al_set_new_bitmap_flags(0);
    if (!font_header || !font_info) {
        al_show_native_message_box(display, "Błąd", "Błąd czcionki", "Nie można załadować pliku arial.ttf!", NULL, ALLEGRO_MESSAGEBOX_ERROR);
        exit(1);
    }

    queue        = al_create_event_queue();
    frame_timer  = al_create_timer(1.0/60.0);
    second_timer = al_create_timer(1.0);

    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_timer_event_source(frame_timer));
    al_register_event_source(queue, al_get_timer_event_source(second_timer));
    al_register_event_source(queue, al_get_mouse_event_source());
    al_register_event_source(queue, al_get_keyboard_event_source());

    al_start_timer(frame_timer);
    al_start_timer(second_timer);
}

void destroy_allegro() {
    if (frame_timer)   al_destroy_timer(frame_timer);
    if (second_timer)  al_destroy_timer(second_timer);
    if (queue)         al_destroy_event_queue(queue);
    if (font_header)   al_destroy_font(font_header);
    if (font_info)     al_destroy_font(font_info);
    if (display)       al_destroy_display(display);
}

void setup_buttons() {
    header.count = in_ws_menu ? WS_COUNT : SET_COUNT;
    header.arr   = header_buf;

    float header_h = window_height * HEADER_PCT;
    float info_h   = window_height * INFO_PCT;
    float spacing  = 10.0f;
    float btn_w    = (window_width - (header.count - 1) * spacing) / header.count;
    float btn_h    = header_h * 0.6f;
    float y0       = (header_h - btn_h) / 2.0f;

    if (!in_ws_menu) {
        // Ustawienie przycisków w menu konfiguracji
        float start_w = btn_w;
        float remaining_width = window_width - start_w - spacing;
        float other_btn_w = (remaining_width - (header.count - 2) * spacing) / (header.count - 1);

        // Ustawienie przycisku Start New Game
        header.arr[SET_START].x = 0;
        header.arr[SET_START].y = y0;
        header.arr[SET_START].w = start_w;
        header.arr[SET_START].h = btn_h;
        header.arr[SET_START].label = setup_labels[SET_START];

        // Ustawienie pozostałych przycisków
        float current_x = start_w + spacing;
        for (int i = 0; i < header.count; i++) {
            if (i != SET_START) {
                header.arr[i].x = current_x;
                header.arr[i].y = y0;
                header.arr[i].w = other_btn_w;
                header.arr[i].h = btn_h;
                header.arr[i].label = setup_labels[i];
                current_x += other_btn_w + spacing;
            }
        }
    } else {
        // Ustawienie przycisków w głównym menu
        for (int i = 0; i < header.count; i++) {
            header.arr[i].x = i * (btn_w + spacing);
            header.arr[i].y = y0;
            header.arr[i].w = btn_w;
            header.arr[i].h = btn_h;
            header.arr[i].label = ws_labels[i];
        }
    }

    float reset_w = window_width * 0.2f;
    float reset_h = info_h * 0.6f;
    reset_button.x     = (window_width - reset_w) / 2.0f;
    reset_button.y     = header_h + (info_h - reset_h) / 2.0f;
    reset_button.w     = reset_w;
    reset_button.h     = reset_h;
    reset_button.label = "Reset";
}

void resize_window(int w, int h) {
    al_resize_display(display, w, h);
    window_width  = w;
    window_height = h;
    setup_buttons();

    if (font_header) al_destroy_font(font_header);
    if (font_info)   al_destroy_font(font_info);

    al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
    char font_path[MAX_PATH];
    snprintf(font_path, MAX_PATH, "%s\\arial.ttf", app_dir);
    font_header = al_load_ttf_font(font_path, -sz_header, 0);
    font_info   = al_load_ttf_font(font_path, -sz_info,   0);
    al_set_new_bitmap_flags(0);

    if (!font_header || !font_info) {
        al_show_native_message_box(display, "Błąd", "Błąd czcionki", "Nie można załadować pliku arial.ttf!", NULL, ALLEGRO_MESSAGEBOX_ERROR);
        exit(1);
    }
}

bool click(const Button *b, float x, float y) {
    return x >= b->x && x <= b->x + b->w &&
           y >= b->y && y <= b->y + b->h;
}

void draw_header() {
    float header_h = window_height * HEADER_PCT;
    al_draw_filled_rectangle(0, 0, window_width, header_h, al_map_rgb(50,50,50));
    for (int i = 0; i < header.count; i++) {
        Button *b = &header.arr[i];
        al_draw_filled_rectangle(b->x,b->y,b->x+b->w,b->y+b->h, al_map_rgb(70,70,70));
        al_draw_rectangle(b->x,b->y,b->x+b->w,b->y+b->h, al_map_rgb(255,255,255),2);
        
        if (i == SET_START && !in_ws_menu) {
            // Rysowanie tekstu w dwóch liniach dla przycisku Start New Game
            float line_h = al_get_font_line_height(font_header);
            al_draw_text(font_header, al_map_rgb(255,255,255),
                        b->x + b->w/2.0f,
                        b->y + b->h/2.0f - line_h,
                        ALLEGRO_ALIGN_CENTER,
                        "Start");
            al_draw_text(font_header, al_map_rgb(255,255,255),
                        b->x + b->w/2.0f,
                        b->y + b->h/2.0f,
                        ALLEGRO_ALIGN_CENTER,
                        "New Game");
        } else {
            float line_h = al_get_font_line_height(font_header);
            char buf[64];
            if (!in_ws_menu) {
                switch (i) {
                    case SET_COLS:
                        snprintf(buf, sizeof(buf), "Columns: %d", temp_cols);
                        break;
                    case SET_ROWS:
                        snprintf(buf, sizeof(buf), "Rows: %d", temp_rows);
                        break;
                    case SET_MINES:
                        snprintf(buf, sizeof(buf), "Mines: %d", temp_mines);
                        break;
                    default:
                        strcpy(buf, b->label);
                }
            } else {
                switch (i) {
                    case WS_HEIGHT:
                        snprintf(buf, sizeof(buf), "Height: %d", window_height);
                        break;
                    case WS_WIDTH:
                        snprintf(buf, sizeof(buf), "Width: %d", window_width);
                        break;
                    default:
                        strcpy(buf, b->label);
                }
            }
            al_draw_text(font_header, al_map_rgb(255,255,255),
                        b->x + b->w/2.0f,
                        b->y + b->h/2.0f - line_h/2.0f,
                        ALLEGRO_ALIGN_CENTER,
                        buf);
        }
    }
}

void draw_info() {
    float header_h = window_height * HEADER_PCT;
    float info_h   = window_height * INFO_PCT;
    float y0       = header_h;
    al_draw_filled_rectangle(0,y0,window_width,y0+info_h,al_map_rgb(200,200,200));
    char buf[64];
    snprintf(buf,sizeof(buf), "%02d:%02d", minutes, seconds);
    float line_h = al_get_font_line_height(font_info);
    al_draw_text(font_info,al_map_rgb(0,0,0),10,    y0+(info_h-line_h)/2.0f,0, buf);
    snprintf(buf,sizeof(buf),"Mines: %d",mines_left);
    al_draw_text(font_info,al_map_rgb(0,0,0),window_width-10, y0+(info_h-line_h)/2.0f, ALLEGRO_ALIGN_RIGHT, buf);
    Button *b = &reset_button;
    al_draw_filled_rectangle(b->x,b->y,b->x+b->w,b->y+b->h, al_map_rgb(180,180,180));
    al_draw_rectangle(b->x,b->y,b->x+b->w,b->y+b->h, al_map_rgb(0,0,0),2);
    al_draw_text(font_info,al_map_rgb(0,0,0),
                 b->x+b->w/2.0f,
                 b->y+b->h/2.0f - line_h/2.0f,
                 ALLEGRO_ALIGN_CENTER, b->label);
}

void draw_game() {
    float header_h = window_height * HEADER_PCT;
    float info_h   = window_height * INFO_PCT;
    float avail_w  = (float)window_width;
    float avail_h  = (float)window_height - header_h - info_h;
    float cell     = fminf(avail_w / grid_cols, avail_h / grid_rows);
    float grid_w   = cell * grid_cols;
    float grid_h   = cell * grid_rows;
    float offset_x = (avail_w - grid_w) / 2.0f;
    float offset_y = header_h + info_h + (avail_h - grid_h) / 2.0f;

    // Najpierw rysuj wszystkie kratki
    for (int r = 0; r < grid_rows; r++) {
        for (int c = 0; c < grid_cols; c++) {
            int x = offset_x + c * cell;
            int y = offset_y + r * cell;
            
            if (grid[r][c].revealed) {
                al_draw_filled_rectangle(x, y, x + cell, y + cell, 
                    grid[r][c].has_mine ? al_map_rgb(255, 0, 0) : al_map_rgb(50, 50, 50));
                
                if (!grid[r][c].has_mine && grid[r][c].adj_mines > 0) {
                    char num_str[2];
                    sprintf(num_str, "%d", grid[r][c].adj_mines);
                    al_draw_text(font_header, al_map_rgb(255, 255, 255), 
                        x + cell/2, y + cell/2 - 16, 
                        ALLEGRO_ALIGN_CENTER, num_str);
                }
            } else {
                al_draw_filled_rectangle(x, y, x + cell, y + cell, 
                    grid[r][c].flagged ? al_map_rgb(0, 0, 255) : al_map_rgb(200, 200, 200));
            }
            al_draw_rectangle(x, y, x + cell, y + cell, al_map_rgb(0, 0, 0), 1);
        }
    }

    // Jeśli gra się skończyła, pokaż wszystkie miny
    if (game_over) {
        for (int r = 0; r < grid_rows; r++) {
            for (int c = 0; c < grid_cols; c++) {
                if (grid[r][c].has_mine) {
                    int x = offset_x + c * cell;
                    int y = offset_y + r * cell;
                    al_draw_filled_rectangle(x, y, x + cell, y + cell, al_map_rgb(255, 0, 0));
                    al_draw_rectangle(x, y, x + cell, y + cell, al_map_rgb(0, 0, 0), 1);
                }
            }
        }
    }
}

void draw_input() {
    const int w = 300, h = 100;
    int x = (window_width - w) / 2;
    int y = (window_height - h) / 2;
    al_draw_filled_rectangle(x,y, x+w,y+h, al_map_rgb(240,240,240));
    al_draw_rectangle(x,y, x+w,y+h, al_map_rgb(0,0,0),2);

    // Rysuj przycisk X w prawym górnym rogu okna input
    int x_btn_size = 24;
    int x_btn_x = x + w - x_btn_size - 4;
    int x_btn_y = y + 4;
    al_draw_filled_rectangle(x_btn_x, x_btn_y, x_btn_x + x_btn_size, x_btn_y + x_btn_size, al_map_rgb(220,60,60));
    al_draw_text(font_info, al_map_rgb(255,255,255), x_btn_x + x_btn_size/2, x_btn_y + 2, ALLEGRO_ALIGN_CENTER, "X");

    char prompt[64];
    if (in_ws_menu) {
        if (input_target == WS_SAVE) {
            strcpy(prompt, "Save as");
        } else if (input_target == WS_LOAD) {
            strcpy(prompt, "Load file");
        } else {
            if (input_target == WS_HEIGHT) {
                snprintf(prompt, sizeof(prompt), "Height (%d-%d)", monitor_min_height, monitor_max_height);
            } else if (input_target == WS_WIDTH) {
                snprintf(prompt, sizeof(prompt), "Width (%d-%d)", monitor_min_width, monitor_max_width);
            } else {
                strcpy(prompt, "Input");
            }
        }
    } else {
        strcpy(prompt,
               (input_target == SET_COLS)  ? "Columns" :
               (input_target == SET_ROWS)  ? "Rows"    :
               (input_target == SET_MINES) ? "Mines"   :
                                             "Input");
    }
    al_draw_text(font_header, al_map_rgb(0,0,0), x+10, y+10, 0, prompt);
    al_draw_text(font_header, al_map_rgb(0,0,0), x+10, y+40, 0, input_buffer);
}

