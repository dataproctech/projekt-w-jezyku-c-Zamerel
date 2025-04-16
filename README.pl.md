---

# Tytuł Projektu: Gra "Saper" wykonana w języku C

🇵🇱 Polski | [🇬🇧 English](README.md)

Opis projektu w języku polskim.

## Opis projektu:
Projekt aplikacji interaktywnej w języku C, wykorzystującej programowanie niskopoziomowe, z modularną strukturą kodu, dynamicznym zarządzaniem pamięcią, kompleksową obsługą błędów oraz graficznym interfejsem użytkownika. Projekt spełnia wymagania zaliczenia na ocenę 5.0, w tym operacje na plikach, efektywne użycie wskaźników i struktur danych oraz intuicyjną obsługę graficzną.

## Harmonogram projektu:
- **Data rozpoczęcia:** 2 kwietnia 2025  
- **Termin zakończenia:** 15 czerwca 2025

---

## Etapy realizacji (kamienie milowe):

### 🚩 Kamień milowy 1: Inicjalizacja projektu i struktura modularna
**Termin:** 23 kwietnia 2025
- [ ] Określenie szczegółowej specyfikacji i wymagań projektu (#Issue 1)
- [ ] Utworzenie struktury repozytorium oraz modularnego szkieletu w C (#Issue 2)
- [ ] Konfiguracja Makefile oraz ustawienia kompilacji (#Issue 3)
- [ ] Projektowanie oraz implementacja wstępnych struktur danych (struct, wskaźniki) (#Issue 4)

### 🚩 Kamień milowy 2: Główna logika gry oraz obsługa plików
**Termin:** 14 maja 2025
- [ ] Implementacja logiki gry (walidacja ruchów, egzekwowanie zasad gry) (#Issue 5)
- [ ] Dynamiczne zarządzanie pamięcią (listy, drzewa, tablice dynamiczne) do przechowywania stanu gry (#Issue 6)
- [ ] Obsługa operacji na plikach (zapis/odczyt stanu gry, logowanie statystyk graczy) (#Issue 7)
- [ ] Intensywne debugowanie, sprawdzanie wycieków pamięci (valgrind), testowanie (#Issue 8)

### 🚩 Kamień milowy 3: Interfejs graficzny i rozszerzona interakcja z użytkownikiem
**Termin:** 11 czerwca 2025
- [ ] Implementacja GUI przy użyciu biblioteki Allegro, SFML lub SDL (#Issue 9)
- [ ] Obsługa graficznej interakcji kursora z ograniczeniem ruchów do poprawnych pól (#Issue 10)
- [ ] Rozbudowana obsługa błędów (walidacja danych wejściowych, bezpieczeństwo pamięci) (#Issue 11)
- [ ] Testy integracyjne, funkcjonalne i użytkowe GUI i mechaniki gry (#Issue 12)

### 🚩 Końcowa wersja projektu
**Termin:** 15 czerwca 2025
- [ ] Refaktoryzacja oraz optymalizacja finalnego kodu (#Issue 13)
- [ ] Końcowa dokumentacja, komentarze w kodzie oraz aktualizacja README (#Issue 14)
- [ ] Oddanie projektu na platformie ekursy (#Issue 15)

---

## Struktura repozytorium:
```
twoj_projekt/
├── assets/
│   └── images/
├── docs/
│   ├── specyfikacja_projektu.md
│   └── instrukcja_uzytkownika.md
├── include/
│   ├── logika_gry.h
│   ├── gui.h
│   └── operacje_plikowe.h
├── src/
│   ├── logika_gry.c
│   ├── gui.c
│   ├── operacje_plikowe.c
│   └── main.c
├── logs/
├── saves/
├── tests/
│   ├── test_logiki.c
│   └── test_gui.c
├── Makefile
├── .gitignore
└── README.md
```

---

## Propozycja issue na GitHub:
Jasno określone zadania ułatwiające zarządzanie projektem:

1. Dokumentacja specyfikacji i wymagań projektu
2. Utworzenie repozytorium i modularnej architektury
3. Konfiguracja Makefile oraz kompilacja projektu
4. Wstępna implementacja dynamicznych struktur danych (struct, wskaźniki)
5. Implementacja logiki i reguł gry
6. Dynamiczne struktury danych do zarządzania stanem gry
7. Operacje plikowe (zapisywanie/wczytywanie gry, logi)
8. Debugowanie i zarządzanie pamięcią (walidacja valgrindem)
9. Implementacja GUI (Allegro/SFML/SDL)
10. Interakcja kursora i ograniczenia ruchów
11. Obsługa błędów i walidacja danych wejściowych
12. Testy integracyjne GUI oraz mechaniki gry
13. Refaktoryzacja i optymalizacja końcowego kodu
14. Końcowa dokumentacja oraz przygotowanie README
15. Oddanie projektu 

---

## Zgodność techniczna i akademicka:
Projekt w pełni spełnia wymagania określone na ocenę 5.0, w tym:

- Modularność ułatwiającą utrzymanie i rozbudowę.
- Zaawansowane zarządzanie pamięcią dynamiczną (alokacja/dealokacja).
- Kompleksowe operacje na plikach (zapisywanie stanów gry i logów).
- Graficzny interfejs użytkownika dla intuicyjnej obsługi.
- Szczegółowa obsługa błędów i odporność na nieprawidłowe operacje użytkownika.

---

## Zasady rozwoju i kontrybucji:
- Kod powinien być modularny, czytelny oraz poprawnie udokumentowany (komentarze).
- Commit powinien jasno odnosić się do konkretnego issue (np. `#3 Wdrożona modularna architektura`).
- Regularne Pull Requesty ułatwiające współpracę i kontrolę jakości kodu.
- Dokumentacja w formie plików tekstowych markdown.

---
