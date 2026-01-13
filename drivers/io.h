#ifndef INCLUDE_IO_H
#define INCLUDE_IO_H

// Wrapper for the assembly 'in' instruction
unsigned char inb(unsigned short port);

// Wrapper for the assembly 'out' instruction
void outb(unsigned short port, unsigned char value);

#endif
