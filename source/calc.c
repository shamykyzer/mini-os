#include "framebuffer.h"
#include "keyboard.h"
#include "menu.h"

// Tiny string helpers (no libc)
static int k_strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

static int ipow(int base, int exp) {
    int result = 1;
    while (exp > 0) {
        result *= base;
        exp--;
    }
    return result;
}

static void calc_print_menu(uint8_t primary_color) {
    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK);
    write_str("\n=== Calculator ===\n");

    set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK);
    write_str("Commands:\n");

    // Command names in primary_color, descriptions in white.
    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK); write_str("  help"); set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK); write_str("      -> show this menu\n");
    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK); write_str("  add");  set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK); write_str("  a b   -> a + b\n");
    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK); write_str("  sub");  set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK); write_str("  a b   -> a - b\n");
    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK); write_str("  mul");  set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK); write_str("  a b   -> a * b\n");
    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK); write_str("  div");  set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK); write_str("  a b   -> a / b\n");
    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK); write_str("  mod");  set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK); write_str("  a b   -> a % b\n");
    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK); write_str("  pow");  set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK); write_str("  a b   -> a^b (b >= 0)\n");
    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK); write_str("  min");  set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK); write_str("  a b   -> min(a,b)\n");
    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK); write_str("  max");  set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK); write_str("  a b   -> max(a,b)\n");
    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK); write_str("  mean"); set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK); write_str(" a b   -> (a+b)/2 (integer)\n");
    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK); write_str("  quit"); set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK); write_str("      -> return to OS\n");
}

void calculator_mode(uint8_t primary_color) {
    char buf[128];

    // Print the calculator menu once when entering calculator mode.
    calc_print_menu(primary_color);

    for (;;) {
        set_color(primary_color, FRAMEBUFFER_COLOR_BLACK);
        write_str("calc> ");

        // User input should appear in primary_color (calculator "command" color).
        set_color(primary_color, FRAMEBUFFER_COLOR_BLACK);
        kbd_readline(buf, sizeof(buf));

        set_color(primary_color, FRAMEBUFFER_COLOR_BLACK);

        if (k_strcmp(buf, "quit") == 0) {
            write_str("Leaving calculator...\n");
            return;
        }

        if (k_strcmp(buf, "help") == 0) {
            calc_print_menu(primary_color);
            continue;
        }

        if (buf[0] == '\0') {
            continue;
        }

        int a, b;

        const char* args;

        if (k_match_cmd(buf, "add", &args)) {
            if (!k_parse_two_ints(args, &a, &b)) write_str("Usage: add <a> <b>\n");
            else { write_dec(a + b); put_char('\n'); }
        } else if (k_match_cmd(buf, "sub", &args)) {
            if (!k_parse_two_ints(args, &a, &b)) write_str("Usage: sub <a> <b>\n");
            else { write_dec(a - b); put_char('\n'); }
        } else if (k_match_cmd(buf, "mul", &args)) {
            if (!k_parse_two_ints(args, &a, &b)) write_str("Usage: mul <a> <b>\n");
            else { write_dec(a * b); put_char('\n'); }
        } else if (k_match_cmd(buf, "div", &args)) {
            if (!k_parse_two_ints(args, &a, &b)) write_str("Usage: div <a> <b>\n");
            else if (b == 0) write_str("Error: divide by zero\n");
            else { write_dec(a / b); put_char('\n'); }
        } else if (k_match_cmd(buf, "mod", &args)) {
            if (!k_parse_two_ints(args, &a, &b)) write_str("Usage: mod <a> <b>\n");
            else if (b == 0) write_str("Error: divide by zero\n");
            else { write_dec(a % b); put_char('\n'); }
        } else if (k_match_cmd(buf, "pow", &args)) {
            if (!k_parse_two_ints(args, &a, &b)) write_str("Usage: pow <base> <exp>\n");
            else if (b < 0) write_str("Error: exp must be >= 0\n");
            else { write_dec(ipow(a, b)); put_char('\n'); }
        } else if (k_match_cmd(buf, "min", &args)) {
            if (!k_parse_two_ints(args, &a, &b)) write_str("Usage: min <a> <b>\n");
            else { write_dec((a < b) ? a : b); put_char('\n'); }
        } else if (k_match_cmd(buf, "max", &args)) {
            if (!k_parse_two_ints(args, &a, &b)) write_str("Usage: max <a> <b>\n");
            else { write_dec((a > b) ? a : b); put_char('\n'); }
        } else if (k_match_cmd(buf, "mean", &args)) {
            if (!k_parse_two_ints(args, &a, &b)) write_str("Usage: mean <a> <b>\n");
            else { write_dec((a + b) / 2); put_char('\n'); }
        } else {
            write_str("Unknown calculator command (type 'quit' to exit)\n");
        }
    }
}
