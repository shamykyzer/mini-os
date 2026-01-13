#include "framebuffer.h"
#include "menu.h"

// Local string helpers (no libc in this freestanding environment)
static int k_strlen(const char *s) {
    int len = 0;
    while (*s) {
        len++;
        s++;
    }
    return len;
}

static void write_n_chars(char c, int count) {
    for (int i = 0; i < count; i++) {
        put_char(c);
    }
}

// Write a string centered within a given width
static void write_centered(const char *text, int width) {
    int len = k_strlen(text);
    if (len >= width) {
        write_str(text);
        return;
    }

    int left = (width - len) / 2;
    int right = width - len - left;

    write_n_chars(' ', left);
    write_str(text);
    write_n_chars(' ', right);
}

// Public API: draw the "Available commands" help menu
void show_help_menu(uint8_t primary_color) {
    const int box_width = 60;
    const int inner_width = box_width - 2;

    // Use the primary color for the box border / command names
    set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK);
    put_char('\n');

    // Top border
    put_char('+');
    write_n_chars('-', inner_width);
    put_char('+');
    put_char('\n');

    // Centered title
    put_char('|');
    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK);
    write_centered("Available commands", inner_width);
    set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK);
    put_char('|');
    put_char('\n');

    // Separator
    put_char('+');
    write_n_chars('-', inner_width);
    put_char('+');
    put_char('\n');

    // Command list lines, nicely indented
    const char *line;

    line = "  help      - Show this help";
    put_char('|');
    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK);
    write_str("  help");
    set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK);
    write_str("      - Show this help");
    write_n_chars(' ', inner_width - k_strlen(line));
    put_char('|');
    put_char('\n');

    line = "  clear     - Clear the screen";
    put_char('|');
    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK);
    write_str("  clear");
    set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK);
    write_str("     - Clear the screen");
    write_n_chars(' ', inner_width - k_strlen(line));
    put_char('|');
    put_char('\n');

    line = "  task1     - Demo VGA output (colors/cursor/scroll)";
    put_char('|');
    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK);
    write_str("  task1");
    set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK);
    write_str("     - Demo VGA output (colors/cursor/scroll)");
    write_n_chars(' ', inner_width - k_strlen(line));
    put_char('|');
    put_char('\n');

    line = "  echo [s]  - Print string s";
    put_char('|');
    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK);
    write_str("  echo [s]");
    set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK);
    write_str("  - Print string s");
    write_n_chars(' ', inner_width - k_strlen(line));
    put_char('|');
    put_char('\n');

    line = "  version   - Show OS version";
    put_char('|');
    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK);
    write_str("  version");
    set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK);
    write_str("   - Show OS version");
    write_n_chars(' ', inner_width - k_strlen(line));
    put_char('|');
    put_char('\n');

    line = "  shutdown  - Dividing by zero...";
    put_char('|');
    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK);
    write_str("  shutdown");
    set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK);
    write_str("  - Dividing by zero...");
    write_n_chars(' ', inner_width - k_strlen(line));
    put_char('|');
    put_char('\n');

    line = "  pink      - Toggle pink mode";
    put_char('|');
    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK);
    write_str("  pink");
    set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK);
    write_str("      - Toggle pink mode");
    write_n_chars(' ', inner_width - k_strlen(line));
    put_char('|');
    put_char('\n');

    line = "  calc      - Enter calculator mode";
    put_char('|');
    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK);
    write_str("  calc");
    set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK);
    write_str("      - Enter calculator mode");
    write_n_chars(' ', inner_width - k_strlen(line));
    put_char('|');
    put_char('\n');

    line = "  task2 a b c - Print Task 2 (sum/max/prod) for a,b,c";
    put_char('|');
    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK);
    write_str("  task2 a b c");
    set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK);
    write_str(" - Print Task 2 (sum/max/prod) for a,b,c");
    write_n_chars(' ', inner_width - k_strlen(line));
    put_char('|');
    put_char('\n');

    line = "  tictactoe - Play TicTacToe";
    put_char('|');
    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK);
    write_str("  tictactoe");
    set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK);
    write_str(" - Play TicTacToe");
    write_n_chars(' ', inner_width - k_strlen(line));
    put_char('|');
    put_char('\n');

    // Bottom border
    put_char('+');
    write_n_chars('-', inner_width);
    put_char('+');
    put_char('\n');
}

// --- Worksheet 2 Part 1 — Task 2 ---
// These functions are called from `drivers/loader.asm` to demonstrate
// passing arguments on the stack from Assembly into C in a freestanding kernel.

int sum_of_three(int arg1, int arg2, int arg3) {
    return arg1 + arg2 + arg3;
}

int max_of_three(int a, int b, int c) {
    int max = a;
    if (b > max) max = b;
    if (c > max) max = c;
    return max;
}

long long product_of_three(int a, int b, int c) {
    return (long long)a * (long long)b * (long long)c;
}
