#include "framebuffer.h"
#include "idt.h"
#include "isr.h"
#include "io.h"
#include "keyboard.h"
#include "menu.h"
#include "version.h"

// Helper function to compare two strings
// Returns 0 if strings are equal, non-zero otherwise
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// Helper function to compare first n characters of two strings
// Used for command parsing (e.g. "echo ")
int strncmp(const char *s1, const char *s2, int n) {
    while (n > 0 && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

static void print_os_version(void) {
    // Versioning policy: v<hundreds>.<tens>.<ones>, derived from git commit count at build time.
    // Example: 41 commits => v0.4.1, 137 commits => v1.3.7
    int count = (int)OS_GIT_COMMIT_COUNT;
    int hundreds = count / 100;
    int tens = (count / 10) % 10;
    int ones = count % 10;

    write_str("SnowOS v");
    write_dec(hundreds);
    put_char('.');
    write_dec(tens);
    put_char('.');
    write_dec(ones);
    write_str(" (alpha)\n");
}

static void print_boot_banner(void) {
    // Keep boot output short, clean, and readable.
    set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLUE);
    write_str(" SnowOS ");
    set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK);
    put_char('\n');

    set_color(FRAMEBUFFER_COLOR_LIGHT_GREY, FRAMEBUFFER_COLOR_BLACK);
    print_os_version();
}

static void vga_test(void) {
    clear_screen();

    set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLUE);
    write_str(" VGA framebuffer test ");
    set_color(FRAMEBUFFER_COLOR_LIGHT_GREY, FRAMEBUFFER_COLOR_BLACK);
    put_char('\n');
    put_char('\n');

    // 1) Color/attribute proof: show all 16 foreground colors on a few backgrounds.
    for (int bg = 0; bg < 4; bg++) {
        write_str("bg=");
        write_dec(bg);
        write_str(": ");

        for (int fg = 0; fg < 16; fg++) {
            set_color((uint8_t)fg, (uint8_t)bg);
            put_char('0' + (fg / 10));
            put_char('0' + (fg % 10));
            put_char(' ');
        }

        set_color(FRAMEBUFFER_COLOR_LIGHT_GREY, FRAMEBUFFER_COLOR_BLACK);
        put_char('\n');
    }

    put_char('\n');

    // 2) Hardware cursor sync proof: jump the cursor and print at the new location.
    write_str("Moving cursor to (10, 8) ...\n");
    move_cursor(10, 8);
    set_color(FRAMEBUFFER_COLOR_LIGHT_GREEN, FRAMEBUFFER_COLOR_BLACK);
    write_str("Cursor moved here.");
    set_color(FRAMEBUFFER_COLOR_LIGHT_GREY, FRAMEBUFFER_COLOR_BLACK);
    put_char('\n');
    put_char('\n');

    // 3) Scrolling proof: print a few lines (keep output short for screenshots).
    write_str("Scroll test (10 lines):\n");
    for (int i = 0; i < 10; i++) {
        write_str("line ");
        write_dec(i);
        put_char('\n');
    }

    set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK);
}

// Main kernel entry point
// Called by loader.asm
void kmain(void) {
    // Set up the framebuffer and clear the screen
    init_framebuffer();

    print_boot_banner();
    put_char('\n');

    // Initialize descriptor tables and interrupts
    // Set up Interrupt Descriptor Table structure and load it into CPU
    init_idt();
    // Populate IDT with ISR (CPU exceptions) and IRQ (hardware interrupts) handlers
    // Also remaps PIC to avoid conflicts with CPU exceptions
    init_interrupt_gates();
    
    // Initialize drivers
    // Initialize keyboard driver and register IRQ1 handler
    init_keyboard();

    // Enable interrupts
    // STI instruction enables maskable interrupts
    // Allow CPU to respond to hardware interrupts
    __asm__ __volatile__("sti");

    // Normal text: white on black
    set_color(FRAMEBUFFER_COLOR_LIGHT_CYAN, FRAMEBUFFER_COLOR_BLACK);
    write_str("Status: IRQ on | keyboard ready\n");
    
    // Restore color for shell
    set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK);
    write_str("Type 'help' for commands.\n\n");

    // Shell Loop: Interactive command-line interface
    // A simple infinite loop that reads commands and executes them
    char buffer[128];
    int pink_mode = 0; // 0 = Cyan mode (default), 1 = Pink mode
    unsigned char primary_color;

    for (;;) {
        // Determine primary color based on mode
        // This feature is not in worksheets, added for customization
        primary_color = pink_mode ? FRAMEBUFFER_COLOR_LIGHT_MAGENTA : FRAMEBUFFER_COLOR_LIGHT_CYAN;

        // Display prompt in primary color
        set_color(primary_color, FRAMEBUFFER_COLOR_BLACK);
        write_str("snowos> ");
        
        // Set input color to white for user typing
        set_color(FRAMEBUFFER_COLOR_WHITE, FRAMEBUFFER_COLOR_BLACK);
        // Read a line of text from the keyboard driver (blocking call)
        kbd_readline(buffer, sizeof(buffer));
        
        // Set output color to primary color (for command responses and errors)
        // Individual commands may override this if needed
        set_color(primary_color, FRAMEBUFFER_COLOR_BLACK);

        // Simple command parsing
        if (strcmp(buffer, "help") == 0) {
            show_help_menu(primary_color);
        } else if (strcmp(buffer, "pink") == 0) {
            // Custom command to toggle UI color
            pink_mode = !pink_mode;
            if (pink_mode) {
                // Set color to pink for the welcome message
                set_color(FRAMEBUFFER_COLOR_LIGHT_MAGENTA, FRAMEBUFFER_COLOR_BLACK);
                write_str("Pink mode enabled.\n");
            } else {
                // When disabling pink mode, switch back to cyan (default primary color)
                // The next prompt will use cyan automatically, but we set it here for the message
                set_color(FRAMEBUFFER_COLOR_LIGHT_CYAN, FRAMEBUFFER_COLOR_BLACK);
                write_str("Pink mode disabled.\n");
            }
        } else if (strcmp(buffer, "shutdown") == 0) {
            // Exit command logic
            write_str("Dividing by zero...\n");

            // Busy wait for approximately 3 seconds to allow message to be visible
            volatile uint32_t i;
            for (i = 0; i < 125000000U; i++) {
            }
            
            // System halt: Write 0x2000 to port 0x604 to trigger shutdown
            // This is a standard x86 shutdown method that works in QEMU and other emulators
            // Using inline assembly for 16-bit port write (outw instruction)
            __asm__ __volatile__("outw %0, %1" : : "a"((unsigned short)0x2000), "Nd"((unsigned short)0x604));
            
            // Fallback: Disable interrupts and halt the CPU if port write doesn't work
            __asm__ __volatile__("cli; hlt");
            break;
        } else if (strcmp(buffer, "clear") == 0) {
            // Clear the framebuffer
            clear_screen();
        } else if (strcmp(buffer, "task1") == 0) {
            vga_test();
        } else if (strcmp(buffer, "version") == 0) {
            print_os_version();
        } else if (strncmp(buffer, "echo ", 5) == 0) {
            // Echo back the string after "echo "
            set_color(FRAMEBUFFER_COLOR_LIGHT_GREEN, FRAMEBUFFER_COLOR_BLACK);
            write_str(buffer + 5);
            put_char('\n');
        } else if (strcmp(buffer, "calc") == 0) {
            calculator_mode(primary_color);
        } else if (strcmp(buffer, "tictactoe") == 0) {
            tictactoe_mode(primary_color);
        } else if (buffer[0] == '\0') {
            // Empty command, just newline
        } else {
            // Commands with arguments (use token matching).
            const char* args = 0;
            int a, b, c;

            if (k_match_cmd(buffer, "task2", &args)) {
                // Worksheet 2 Part 1 — Task 2: stack argument passing demo
                // Accepts any three ints; if none given, use the worksheet examples.
                int ok = 1;
                if (k_skip_ws(args)[0] == '\0') {
                    a = 1; b = 2; c = 3;
                } else {
                    ok = k_parse_three_ints(args, &a, &b, &c);
                }

                if (!ok) {
                    write_str("Usage: task2 <a> <b> <c>\n");
                } else {
                    int s2 = sum_of_three(a, b, c);
                    int m2 = max_of_three(a, b, c);
                    long long p2 = product_of_three(a, b, c);

                    set_color(FRAMEBUFFER_COLOR_LIGHT_CYAN, FRAMEBUFFER_COLOR_BLACK);
                    write_str("Task 2 stack-argument passing results:\n");

                    set_color(FRAMEBUFFER_COLOR_LIGHT_GREY, FRAMEBUFFER_COLOR_BLACK);
                    write_str("sum_of_three("); write_dec(a); write_str(", "); write_dec(b); write_str(", "); write_dec(c); write_str(") = ");
                    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK);
                    write_dec(s2);
                    set_color(FRAMEBUFFER_COLOR_LIGHT_GREY, FRAMEBUFFER_COLOR_BLACK);
                    put_char('\n');

                    write_str("max_of_three("); write_dec(a); write_str(", "); write_dec(b); write_str(", "); write_dec(c); write_str(") = ");
                    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK);
                    write_dec(m2);
                    set_color(FRAMEBUFFER_COLOR_LIGHT_GREY, FRAMEBUFFER_COLOR_BLACK);
                    put_char('\n');

                    write_str("product_of_three("); write_dec(a); write_str(", "); write_dec(b); write_str(", "); write_dec(c); write_str(") = ");
                    set_color(primary_color, FRAMEBUFFER_COLOR_BLACK);
                    write_dec_ll(p2);
                    set_color(FRAMEBUFFER_COLOR_LIGHT_GREY, FRAMEBUFFER_COLOR_BLACK);
                    put_char('\n');
                }
            } else {
                write_str("Unknown command: ");
                write_str(buffer);
                put_char('\n');
            }
        }
    }
}
