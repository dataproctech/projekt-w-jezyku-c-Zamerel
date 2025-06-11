#ifndef FILE_DIALOG_H
#define FILE_DIALOG_H

#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <stdio.h>

// Funkcja do pobrania ścieżki do folderu Dokumenty
void get_documents_path(char* path, size_t size);

// Funkcja do wyświetlania okna wyboru pliku
const char* show_file_selection_dialog(void);

#endif // FILE_DIALOG_H 