#include "keyboard.h"
#include "isr.h"
#include "io.h"
#include "framebuffer.h"

/* US Keyboard Layout scancode table. */
unsigned char kbdus[128] =
{
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};

/* Circular Buffer for Keyboard Input */
#define KB_BUFFER_SIZE 256
static char kb_buffer[KB_BUFFER_SIZE];
static volatile uint32_t kb_write_ptr = 0;
static volatile uint32_t kb_read_ptr = 0;

/* Write a character to the circular buffer */
static void buffer_write(char c) {
    uint32_t next_write = (kb_write_ptr + 1) % KB_BUFFER_SIZE;
    if (next_write != kb_read_ptr) { // Buffer not full
        kb_buffer[kb_write_ptr] = c;
        kb_write_ptr = next_write;
    }
    // Else drop character (buffer full)
}

/* Read a character from the circular buffer (blocking call) */
static char buffer_read(void) {
    // Wait until buffer has data (read and write pointers differ)
    while (kb_read_ptr == kb_write_ptr) {
        // Halt CPU until next interrupt (keyboard IRQ will wake us up)
        // In a real OS with multitasking, we would yield to another process here
        __asm__ __volatile__("hlt");
    }
    
    // Read character and advance read pointer
    char c = kb_buffer[kb_read_ptr];
    kb_read_ptr = (kb_read_ptr + 1) % KB_BUFFER_SIZE;
    return c;
}

/* Public API: Get a single character */
char kbd_getc(void) {
    return buffer_read();
}

/* Public API: Read a line of input until Enter is pressed or max_len is reached */
void kbd_readline(char* buffer, uint32_t max_len) {
    uint32_t i = 0;
    while (i < max_len - 1) {
        char c = kbd_getc();  // Blocking call, waits for keyboard input
        
        if (c == '\n') {
            // Enter key pressed, terminate string and return
            buffer[i] = '\0';
            return;
        } else if (c == '\b') {
            // Backspace: remove last character from buffer if any
            if (i > 0) {
                i--;
            }
        } else {
            // Regular character: add to buffer
            buffer[i++] = c;
        }
    }
    // Max length reached, null terminate the string
    buffer[i] = '\0';
}

/* Keyboard interrupt handler (IRQ1) called when a key is pressed or released */
static void keyboard_callback(registers_t *regs) {
    (void)regs;  // Unused parameter

    /* Read scancode from keyboard controller data port (0x60) */
    /* We skip the status check (inb(0x64) & 1) because the IRQ implies data is ready */
    uint8_t scancode = inb(0x60);

    if (scancode & 0x80) {
        /* Bit 7 set = key release event - ignore for now */
    } else {
        /* Bit 7 clear = key press event */
        char c = kbdus[scancode];
        if (c != 0) {
            put_char(c);      /* Echo character to screen immediately */
            buffer_write(c);  /* Store character in circular buffer for kbd_getc() */
        }
    }
    
    /* PIC EOI (End of Interrupt) is handled by irq_handler wrapper in isr.c */
}

/* Initialize keyboard driver: register IRQ handler and enable keyboard interrupts */
void init_keyboard(void) {
   // Register our callback function to handle IRQ1 (keyboard interrupt)
   register_interrupt_handler(IRQ1, keyboard_callback);
   
   /* Flush keyboard controller buffer to prevent initial stuck key behavior */
   /* Read and discard any pending scancodes from the controller */
   while (inb(0x64) & 0x01) {  // Check if output buffer has data
       inb(0x60);  // Read and discard the scancode
       /* Small delay to allow the controller to update status */
       /* Writing to port 0x80 is a common delay technique (does nothing, just waits) */
       outb(0x80, 0);
       outb(0x80, 0);
   }
   
   /* Activate the keyboard interface (enable keyboard) */
   /* Command 0xAE = Enable keyboard (0xAD would disable it) */
   outb(0x64, 0xAE);

   /* Explicitly unmask IRQ1 (Keyboard) on the PIC Master data port (0x21) */
   /* Get current interrupt mask */
   uint8_t mask = inb(0x21);
   /* Clear bit 1 (IRQ1) to enable keyboard interrupts */
   mask &= ~(1 << 1);
   /* Write back the updated mask */
   outb(0x21, mask);
}
