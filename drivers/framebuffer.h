/* Simple VGA text-mode framebuffer driver interface
 * Assumes standard 80x25 text mode at physical address 0xB8000.
 * Each cell is 2 bytes: ASCII character (low byte) and attribute (high byte).
 */

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H


/* basic fixed-width types used throughout the driver */
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef signed int     int32_t;

/* Framebuffer geometry and address for VGA text mode */
#define FRAMEBUFFER_WIDTH   80
#define FRAMEBUFFER_HEIGHT  25
#define FRAMEBUFFER_ADDRESS 0x000B8000

/* VGA colours (use when setting foreground/background attributes) */
enum {
    FRAMEBUFFER_COLOR_BLACK = 0,
    FRAMEBUFFER_COLOR_BLUE = 1,
    FRAMEBUFFER_COLOR_GREEN = 2,
    FRAMEBUFFER_COLOR_CYAN = 3,
    FRAMEBUFFER_COLOR_RED = 4,
    FRAMEBUFFER_COLOR_MAGENTA = 5,
    FRAMEBUFFER_COLOR_BROWN = 6,
    FRAMEBUFFER_COLOR_LIGHT_GREY = 7,
    FRAMEBUFFER_COLOR_DARK_GREY = 8,
    FRAMEBUFFER_COLOR_LIGHT_BLUE = 9,
    FRAMEBUFFER_COLOR_LIGHT_GREEN = 10,
    FRAMEBUFFER_COLOR_LIGHT_CYAN = 11,
    FRAMEBUFFER_COLOR_LIGHT_RED = 12,
    FRAMEBUFFER_COLOR_LIGHT_MAGENTA = 13,
    FRAMEBUFFER_COLOR_LIGHT_BROWN = 14,
    FRAMEBUFFER_COLOR_WHITE = 15
};

/* Public API
 * - init_framebuffer(): clears the screen and sets default colours
 * - clear_screen(): fill the screen with spaces using the current colours
 * - set_color(fg,bg): set the current foreground/background colours
 * - move_cursor(x,y): move the hardware text cursor to (x,y)
 * - put_char(c): write a single character at the current cursor (handles \n)
 * - write_str(s): write a NUL-terminated string
 * - write_dec(value): write a signed decimal integer
 */
void init_framebuffer(void);
void clear_screen(void);
void set_color(uint8_t fg, uint8_t bg);
void move_cursor(uint16_t x, uint16_t y);

void put_char(char c);
void write_str(const char *s);
void write_dec(int value);
void write_dec_ll(long long value);

/* Selection Support */
char framebuffer_get_char(uint16_t x, uint16_t y);
void framebuffer_highlight_region(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void framebuffer_clear_selection(void);

/* Cursor getters */
uint16_t get_cursor_x(void);
uint16_t get_cursor_y(void);

/* --- Tiny CLI parsing helpers (no libc) ---
 * Used by the kernel shell and apps (calc/tictactoe) to parse commands and integer arguments.
 */
const char* k_skip_ws(const char* s);
int k_match_cmd(const char* line, const char* cmd, const char** after);
int k_parse_int(const char* s, int* out, const char** end);
int k_parse_two_ints(const char* s, int* a, int* b);
int k_parse_three_ints(const char* s, int* a, int* b, int* c);

#endif
