// menu.h - helpers for displaying command menus / help screens

#ifndef MENU_H
#define MENU_H

#include "types.h"

// Display the nicely formatted "Available commands" box
void show_help_menu(uint8_t primary_color);

// Enters calculator mode; returns to OS shell when user types "quit".
void calculator_mode(uint8_t primary_color);

// Enters TicTacToe mode; returns to OS shell when user types "quit".
void tictactoe_mode(uint8_t primary_color);

// --- Worksheet 2 Part 1 — Task 2 ---
// C functions called from `loader.asm` to demonstrate stack argument passing.
int sum_of_three(int arg1, int arg2, int arg3);
int max_of_three(int a, int b, int c);
long long product_of_three(int a, int b, int c);

#endif // MENU_H
