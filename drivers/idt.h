#ifndef INCLUDE_IDT_H
#define INCLUDE_IDT_H

#include "types.h"

// A struct describing an interrupt gate.
struct idt_entry_struct {
    uint16_t base_lo;             // The lower 16 bits of the address to jump to when this interrupt fires.
    uint16_t sel;                 // Kernel segment selector.
    uint8_t  always0;             // This must always be zero.
    uint8_t  flags;               // More flags. See documentation.
    uint16_t base_hi;             // The upper 16 bits of the address to jump to.
} __attribute__((packed));

typedef struct idt_entry_struct idt_entry_t;

// A struct describing a pointer to an array of interrupt handlers.
// This is in a format suitable for giving to 'lidt'.
struct idt_ptr_struct {
    uint16_t limit;               // Size of the IDT table - 1
    uint32_t base;                // The address of the first element in our idt_entry_t array.
} __attribute__((packed));

typedef struct idt_ptr_struct idt_ptr_t;

// Defined in idt_load.asm
extern void idt_load(uint32_t);

void init_idt(void);
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);

#endif
