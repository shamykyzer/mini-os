#ifndef INCLUDE_KEYBOARD_H
#define INCLUDE_KEYBOARD_H

#include "types.h"

void init_keyboard(void);
char kbd_getc(void);
void kbd_readline(char* buffer, uint32_t max_len);

#endif
