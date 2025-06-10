#include "file_dialog.h"
#include <allegro5/allegro.h>
#include <allegro5/allegro_native_dialog.h>
#include <stdio.h>
#include <direct.h>

// Funkcja do wyświetlania okna wyboru pliku
const char* show_file_selection_dialog(void) {
    char documents_path[260];
    get_documents_path(documents_path, 260);
    
    // Upewnij się, że folder istnieje
    _mkdir(documents_path);
    
    // Przygotuj filtr plików
    const char* patterns[] = {"*.mine", "*.*"};
    
    // Wyświetl okno wyboru pliku
    ALLEGRO_FILECHOOSER* dialog = al_create_native_file_dialog(
        documents_path,
        "Wybierz plik zapisu",
        "*.mine;*.*",
        ALLEGRO_FILECHOOSER_FILE_MUST_EXIST
    );
    
    if (!dialog) {
        return NULL;
    }
    
    if (al_show_native_file_dialog(NULL, dialog)) {
        if (al_get_native_file_dialog_count(dialog) > 0) {
            const char* path = al_get_native_file_dialog_path(dialog, 0);
            if (path) {
                // Znajdź ostatni znak '\' w ścieżce
                const char* last_slash = strrchr(path, '\\');
                if (last_slash) {
                    // Zwróć tylko nazwę pliku bez ścieżki
                    return strdup(last_slash + 1);
                }
                return strdup(path);
            }
        }
    }
    
    al_destroy_native_file_dialog(dialog);
    return NULL;
} 