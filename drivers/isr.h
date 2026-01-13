#ifndef INCLUDE_ISR_H
#define INCLUDE_ISR_H

#include "types.h"

// PIC IRQs are mapped to IDT vectors starting at 32 (0x20) after PIC remap.
#define IRQ_BASE 32
#define IRQ(n)   (IRQ_BASE + (n))

#define IRQ0  IRQ(0)
#define IRQ1  IRQ(1)
#define IRQ2  IRQ(2)
#define IRQ3  IRQ(3)
#define IRQ4  IRQ(4)
#define IRQ5  IRQ(5)
#define IRQ6  IRQ(6)
#define IRQ7  IRQ(7)
#define IRQ8  IRQ(8)
#define IRQ9  IRQ(9)
#define IRQ10 IRQ(10)
#define IRQ11 IRQ(11)
#define IRQ12 IRQ(12)
#define IRQ13 IRQ(13)
#define IRQ14 IRQ(14)
#define IRQ15 IRQ(15)

// CPU register snapshot pushed by the ASM stubs.
typedef struct registers {
    uint32_t ds;                  // Data segment selector
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
    uint32_t int_no, err_code;    // Interrupt number and error code (if applicable)
    uint32_t eip, cs, eflags, useresp, ss; // Pushed by the processor automatically.
} registers_t;

// C-level interrupt callback (CPU exception or IRQ).
typedef void (*isr_t)(registers_t *regs);
void register_interrupt_handler(uint8_t n, isr_t handler);
void init_interrupt_gates(void);

#endif
