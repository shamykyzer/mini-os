#include "framebuffer.h"
#include "keyboard.h"
#include "menu.h"

static void ttt_print_help(uint8_t primary_color) {
    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK);
    write_str("\n=== TicTacToe ===\n");

    set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK);
    write_str("Moves:\n");
    write_str("  Type a number 1-9 to place your mark.\n\n");

    write_str("Commands:\n");
    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK); write_str("  help");  set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK); write_str("      -> show this menu\n");
    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK); write_str("  clear"); set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK); write_str("     -> clear screen + redraw board\n");
    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK); write_str("  restart"); set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK); write_str("   -> restart the game (optionally: restart x|o)\n");
    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK); write_str("  quit");  set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK); write_str("      -> return to OS\n");
}

static char ttt_cell_char(uint8_t cell, int cell_index) {
    // 0 = empty, 1 = X, 2 = O
    if (cell == 1) return 'X';
    if (cell == 2) return 'O';
    // Show position numbers for empty cells (1-9)
    return (char)('1' + cell_index);
}

static int ttt_won(const uint8_t board[9], uint8_t player) {
    static const uint8_t w[8][3] = {
        {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, // rows
        {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, // cols
        {0, 4, 8}, {2, 4, 6}             // diagonals
    };

    for (int i = 0; i < 8; i++) {
        uint8_t a = w[i][0], b = w[i][1], c = w[i][2];
        if (board[a] == player && board[b] == player && board[c] == player) return 1;
    }
    return 0;
}

static int ttt_full(const uint8_t board[9]) {
    for (int i = 0; i < 9; i++) {
        if (board[i] == 0) return 0;
    }
    return 1;
}

static void ttt_draw(const uint8_t board[9], uint8_t primary_color) {
    // Header
    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK);
    write_str("TicTacToe  (type 'help' for commands)\n\n");

    // Board
    for (int r = 0; r < 3; r++) {
        int i = r * 3;
        set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK);
        put_char(' ');
        if (board[i + 0] == 1) set_color(FRAMEBUFFER_COLOR_LIGHT_RED, FRAMEBUFFER_COLOR_BLACK);
        else if (board[i + 0] == 2) set_color(FRAMEBUFFER_COLOR_LIGHT_GREEN, FRAMEBUFFER_COLOR_BLACK);
        else set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK);
        put_char(ttt_cell_char(board[i + 0], i + 0));
        set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK);
        write_str(" | ");
        if (board[i + 1] == 1) set_color(FRAMEBUFFER_COLOR_LIGHT_RED, FRAMEBUFFER_COLOR_BLACK);
        else if (board[i + 1] == 2) set_color(FRAMEBUFFER_COLOR_LIGHT_GREEN, FRAMEBUFFER_COLOR_BLACK);
        else set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK);
        put_char(ttt_cell_char(board[i + 1], i + 1));
        set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK);
        write_str(" | ");
        if (board[i + 2] == 1) set_color(FRAMEBUFFER_COLOR_LIGHT_RED, FRAMEBUFFER_COLOR_BLACK);
        else if (board[i + 2] == 2) set_color(FRAMEBUFFER_COLOR_LIGHT_GREEN, FRAMEBUFFER_COLOR_BLACK);
        else set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK);
        put_char(ttt_cell_char(board[i + 2], i + 2));
        set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK);
        put_char('\n');
        if (r < 2) {
            write_str("---+---+---\n");
        }
    }
    put_char('\n');
}

// Parse a single-digit move 1..9, allowing surrounding whitespace.
// Returns 1 on success (writes [0..8] index into *out_idx), else 0.
static int ttt_parse_move(const char *s, int *out_idx) {
    // skip leading spaces/tabs
    while (*s == ' ' || *s == '\t') s++;
    if (*s < '1' || *s > '9') return 0;
    int idx = *s - '1';
    s++;
    // allow trailing whitespace only
    while (*s == ' ' || *s == '\t') s++;
    if (*s != '\0') return 0;
    *out_idx = idx;
    return 1;
}

void tictactoe_mode(uint8_t primary_color) {
    uint8_t board[9];
    for (int i = 0; i < 9; i++) board[i] = 0;

    // 1 = X, 2 = O (X always starts)
    uint8_t player = 1u;
    char buf[128];
    int game_over = 0;

    clear_screen();
    ttt_draw(board, primary_color);
    ttt_print_help(primary_color);

    for (;;) {
        // Display whose turn it is (with requested colors).
        set_color(primary_color, FRAMEBUFFER_COLOR_BLACK);
        write_str("Player's ");
        if (player == 1) set_color(FRAMEBUFFER_COLOR_LIGHT_RED, FRAMEBUFFER_COLOR_BLACK);
        else set_color(FRAMEBUFFER_COLOR_LIGHT_GREEN, FRAMEBUFFER_COLOR_BLACK);
        put_char(player == 1 ? 'X' : 'O');
        set_color(primary_color, FRAMEBUFFER_COLOR_BLACK);
        write_str(" turn\n");

        set_color(primary_color, FRAMEBUFFER_COLOR_BLACK);
        write_str("ttt> ");

        set_color(primary_color, FRAMEBUFFER_COLOR_BLACK);
        kbd_readline(buf, sizeof(buf));

        // Normalize output color
        set_color(primary_color, FRAMEBUFFER_COLOR_BLACK);

        const char* args = 0;

        if (k_match_cmd(buf, "quit", &args)) {
            if (k_skip_ws(args)[0] != '\0') {
                write_str("Usage: quit\n");
            } else {
                write_str("Leaving TicTacToe...\n");
                return;
            }
        }

        if (k_match_cmd(buf, "help", &args)) {
            if (k_skip_ws(args)[0] != '\0') {
                write_str("Usage: help\n");
            } else {
                ttt_print_help(primary_color);
            }
            continue;
        }

        if (k_match_cmd(buf, "clear", &args)) {
            if (k_skip_ws(args)[0] != '\0') {
                write_str("Usage: clear\n");
            } else {
                clear_screen();          // explicit utilization of "clear"
                ttt_draw(board, primary_color);
            }
            continue;
        }

        if (k_match_cmd(buf, "restart", &args)) {
            // Optional: "restart x" or "restart o" to choose starting player.
            const char* p = k_skip_ws(args);
            uint8_t start = 1u;
            if (p[0] != '\0') {
                if ((p[0] == 'x' || p[0] == 'X') && k_skip_ws(p + 1)[0] == '\0') start = 1u;
                else if ((p[0] == 'o' || p[0] == 'O') && k_skip_ws(p + 1)[0] == '\0') start = 2u;
                else {
                    write_str("Usage: restart [x|o]\n");
                    continue;
                }
            }

            for (int i = 0; i < 9; i++) board[i] = 0;
            game_over = 0;
            player = start;
            clear_screen();
            ttt_draw(board, primary_color);
            continue;
        }

        if (buf[0] == '\0') {
            continue;
        }

        if (game_over) {
            write_str("Game over. Type 'restart' or 'quit'.\n");
            continue;
        }

        int idx;
        if (!ttt_parse_move(buf, &idx)) {
            write_str("Invalid input. Type 1-9, or 'help'.\n");
            continue;
        }

        if (board[idx] != 0) {
            write_str("That square is taken. Choose another.\n");
            continue;
        }

        board[idx] = player;

        clear_screen();
        ttt_draw(board, primary_color);

        if (ttt_won(board, player)) {
            set_color(primary_color, FRAMEBUFFER_COLOR_BLACK);
            write_str("Winner: ");
            if (player == 1) set_color(FRAMEBUFFER_COLOR_LIGHT_RED, FRAMEBUFFER_COLOR_BLACK);
            else set_color(FRAMEBUFFER_COLOR_LIGHT_GREEN, FRAMEBUFFER_COLOR_BLACK);
            put_char(player == 1 ? 'X' : 'O');
            put_char('\n');
            set_color(primary_color, FRAMEBUFFER_COLOR_BLACK);
            write_str("Type 'restart' to play again, or 'quit' to return to the OS.\n");
            game_over = 1;
            continue;
        }

        if (ttt_full(board)) {
            set_color(primary_color, FRAMEBUFFER_COLOR_BLACK);
            write_str("Draw.\n");
            write_str("Type 'restart' to play again, or 'quit' to return to the OS.\n");
            game_over = 1;
            continue;
        }

        // Alternate turns after a successful move (standard TicTacToe rules).
        if (player == 1u) {
            player = 2u;
        } else {
            player = 1u;
        }
    }
}
