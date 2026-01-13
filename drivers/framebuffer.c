#include "framebuffer.h"

/* Implementation of a basic VGA text-mode framebuffer driver.
 * This driver writes directly to the VGA text buffer at 0xB8000 and
 * manages a simple software cursor which is also propagated to the
 * hardware cursor via VGA I/O ports (0x3D4/0x3D5).
 */

/* Pointer to VGA text-mode memory. Each character cell uses two bytes:
 * [character][attribute]. We index by cell and multiply by 2 when
 * writing bytes directly into the buffer.
*/
static uint8_t *framebuffer = (uint8_t *)FRAMEBUFFER_ADDRESS;

/* Current cursor position and current drawing colours (fg/bg). */
static uint16_t cursor_x = 0;
static uint16_t cursor_y = 0;
static uint8_t current_fg = FRAMEBUFFER_COLOR_LIGHT_GREY;
static uint8_t current_bg = FRAMEBUFFER_COLOR_BLACK;

/* VGA port definitions used to update the hardware text cursor. */
#define FB_COMMAND_PORT        0x3D4
#define FB_DATA_PORT           0x3D5
#define FB_HIGH_BYTE_COMMAND   14
#define FB_LOW_BYTE_COMMAND    15

/* Write one byte to an I/O port. This uses the `outb` instruction and
 * is marked inline so the compiler emits efficient code for port I/O.
 */
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}

/* Write a single character cell at the given cell index using the
 * provided foreground/background colours.
 */
static void write_cell(uint16_t index, char c, uint8_t fg, uint8_t bg) {
    uint8_t attr = (bg << 4) | (fg & 0x0F);
    framebuffer[2 * index]     = (uint8_t)c;
    framebuffer[2 * index + 1] = attr;
}

/* Update the VGA hardware cursor to match the software cursor
 * position stored in `cursor_x`/`cursor_y`.
 */
static void update_cursor(void) {
    uint16_t pos = cursor_y * FRAMEBUFFER_WIDTH + cursor_x;

    outb(FB_COMMAND_PORT, FB_HIGH_BYTE_COMMAND);
    outb(FB_DATA_PORT, (pos >> 8) & 0xFF);

    outb(FB_COMMAND_PORT, FB_LOW_BYTE_COMMAND);
    outb(FB_DATA_PORT, pos & 0xFF);
}

/* Set current foreground/background colours used for subsequent writes. */
void set_color(uint8_t fg, uint8_t bg) {
    current_fg = fg;
    current_bg = bg;
}

/* Move the cursor to a specific (x,y) location. Bounds-checking prevents
 * the cursor from being moved off-screen.
 */
void move_cursor(uint16_t x, uint16_t y) {
    if (x >= FRAMEBUFFER_WIDTH)  x = FRAMEBUFFER_WIDTH - 1;
    if (y >= FRAMEBUFFER_HEIGHT) y = FRAMEBUFFER_HEIGHT - 1;

    cursor_x = x;
    cursor_y = y;
    update_cursor();
}

/* Clear the entire screen by writing spaces using the current colours and
 * reset the cursor to the top-left corner.
 */
void clear_screen(void) {
    uint16_t i;
    for (i = 0; i < FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT; i++) {
        write_cell(i, ' ', current_fg, current_bg);
    }

    cursor_x = 0;
    cursor_y = 0;
    update_cursor();
}

/* Initialise the framebuffer state to defaults and clear the screen. */
void init_framebuffer(void) {
    current_fg = FRAMEBUFFER_COLOR_LIGHT_GREY;
    current_bg = FRAMEBUFFER_COLOR_BLACK;
    clear_screen();
}

/* Scroll the buffer up one line when the cursor moves past the bottom of
 * the screen. This performs a simple memcopy-by-cell to move lines up and
 * clears the last line.
 */
static void scroll_if_needed(void) {
    if (cursor_y < FRAMEBUFFER_HEIGHT)
        return;

    uint16_t x, y;

    for (y = 1; y < FRAMEBUFFER_HEIGHT; y++) {
        for (x = 0; x < FRAMEBUFFER_WIDTH; x++) {
            uint16_t from = y * FRAMEBUFFER_WIDTH + x;
            uint16_t to   = (y - 1) * FRAMEBUFFER_WIDTH + x;

            framebuffer[2 * to]     = framebuffer[2 * from];
            framebuffer[2 * to + 1] = framebuffer[2 * from + 1];
        }
    }

    /* Clear the last line after shifting everything up. */
    for (x = 0; x < FRAMEBUFFER_WIDTH; x++) {
        uint16_t index = (FRAMEBUFFER_HEIGHT - 1) * FRAMEBUFFER_WIDTH + x;
        write_cell(index, ' ', current_fg, current_bg);
    }

    cursor_y = FRAMEBUFFER_HEIGHT - 1;
}

/* Output a single character. Newlines move the cursor to the start of the
 * next line. After writing we advance the cursor and update the hardware
 * cursor; scrolling is performed if necessary.
 */
void put_char(char c) {
    if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
        } else if (cursor_y > 0) {
            cursor_y--;
            cursor_x = FRAMEBUFFER_WIDTH - 1;
        }
        
        uint16_t index = cursor_y * FRAMEBUFFER_WIDTH + cursor_x;
        write_cell(index, ' ', current_fg, current_bg);
        update_cursor();
        return;
    }

    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        scroll_if_needed();
        update_cursor();
        return;
    }

    uint16_t index = cursor_y * FRAMEBUFFER_WIDTH + cursor_x;
    write_cell(index, c, current_fg, current_bg);

    cursor_x++;
    if (cursor_x >= FRAMEBUFFER_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    scroll_if_needed();
    update_cursor();
}

/* Write a NUL-terminated string by repeatedly calling put_char. */
void write_str(const char *s) {
    while (*s)
        put_char(*s++);
}

/* Write a signed decimal integer. Uses a small buffer to build the digits
 * in reverse order then emits them in the correct order. Zero is handled
 * as a special case.
 */
void write_dec(int value) {
    char buf[16];
    int i = 0;
    int neg = 0;

    if (value == 0) {
        put_char('0');
        return;
    }

    if (value < 0) {
        neg = 1;
        value = -value;
    }
    
    /* Build the number in reverse order in the buffer */
    while (value > 0 && i < (int)(sizeof(buf) - 1)) {
        buf[i++] = '0' + (value % 10);
        value /= 10;
    }
    
    if (neg)
        buf[i++] = '-';

    /* Output the digits in correct order */      
    for (int j = i - 1; j >= 0; j--)
        put_char(buf[j]);
}

/* Write a signed decimal long long. Safe for LLONG_MIN (avoids negation overflow). */
void write_dec_ll(long long value) {
    char buf[32];
    int i = 0;
    unsigned long long u;

    if (value == 0) {
        put_char('0');
        return;
    }

    if (value < 0) {
        put_char('-');
        /* Convert to magnitude in unsigned without overflowing on LLONG_MIN */
        u = (unsigned long long)(-(value + 1)) + 1ULL;
    } else {
        u = (unsigned long long)value;
    }

    while (u > 0ULL && i < (int)(sizeof(buf) - 1)) {
        buf[i++] = (char)('0' + (u % 10ULL));
        u /= 10ULL;
    }

    for (int j = i - 1; j >= 0; j--)
        put_char(buf[j]);
}

/* --- New Selection Functions --- */

char framebuffer_get_char(uint16_t x, uint16_t y) {
    if (x >= FRAMEBUFFER_WIDTH || y >= FRAMEBUFFER_HEIGHT) return 0;
    uint16_t index = y * FRAMEBUFFER_WIDTH + x;
    return (char)framebuffer[2 * index];
}

void framebuffer_highlight_region(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    /* Ensure coordinates are ordered correctly (top-left to bottom-right) */
    if (y1 > y2 || (y1 == y2 && x1 > x2)) {
        /* Swap */
        uint16_t tx = x1; x1 = x2; x2 = tx;
        uint16_t ty = y1; y1 = y2; y2 = ty;
    }

    uint16_t start = y1 * FRAMEBUFFER_WIDTH + x1;
    uint16_t end   = y2 * FRAMEBUFFER_WIDTH + x2;

    for (uint16_t i = start; i <= end; i++) {
        /* Preserve existing character, change background to Blue (1) and foreground to White (15) */
        /* Highlight style: White on Blue */
        uint8_t attr = (FRAMEBUFFER_COLOR_BLUE << 4) | (FRAMEBUFFER_COLOR_WHITE & 0x0F);
        framebuffer[2 * i + 1] = attr;
    }
}

void framebuffer_clear_selection(void) {
    /* Reset all attributes to standard Light Grey on Black */
    /* Note: This is a simple implementation that wipes custom colors. 
       A better one would only reset the selected region. */
    uint16_t i;
    for (i = 0; i < FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT; i++) {
        uint8_t current_attr = framebuffer[2 * i + 1];
        /* If it looks like our highlight color (White on Blue), reset it */
        if (current_attr == ((FRAMEBUFFER_COLOR_BLUE << 4) | (FRAMEBUFFER_COLOR_WHITE & 0x0F))) {
             uint8_t attr = (FRAMEBUFFER_COLOR_BLACK << 4) | (FRAMEBUFFER_COLOR_LIGHT_GREY & 0x0F);
             framebuffer[2 * i + 1] = attr;
        }
    }
}

uint16_t get_cursor_x(void) { return cursor_x; }
uint16_t get_cursor_y(void) { return cursor_y; }

/* --- Tiny CLI parsing helpers (no libc) --- */

const char* k_skip_ws(const char* s) {
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

int k_match_cmd(const char* line, const char* cmd, const char** after) {
    const char* p = k_skip_ws(line);
    const char* c = cmd;

    while (*c && *p == *c) {
        p++;
        c++;
    }

    // Must match the full command token.
    if (*c != '\0') return 0;

    // Next character must be whitespace or end-of-string (token boundary).
    if (*p != '\0' && *p != ' ' && *p != '\t') return 0;

    *after = p;
    return 1;
}

int k_parse_int(const char* s, int* out, const char** end) {
    s = k_skip_ws(s);

    int sign = 1;
    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+') {
        s++;
    }

    if (*s < '0' || *s > '9') return 0;

    int value = 0;
    while (*s >= '0' && *s <= '9') {
        value = value * 10 + (*s - '0');
        s++;
    }

    *out = sign * value;
    *end = s;
    return 1;
}

int k_parse_two_ints(const char* s, int* a, int* b) {
    const char* p;
    if (!k_parse_int(s, a, &p)) return 0;
    if (!k_parse_int(p, b, &p)) return 0;
    p = k_skip_ws(p);
    return (*p == '\0');
}

int k_parse_three_ints(const char* s, int* a, int* b, int* c) {
    const char* p;
    if (!k_parse_int(s, a, &p)) return 0;
    if (!k_parse_int(p, b, &p)) return 0;
    if (!k_parse_int(p, c, &p)) return 0;
    p = k_skip_ws(p);
    return (*p == '\0');
}
