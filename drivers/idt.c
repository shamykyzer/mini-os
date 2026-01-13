#include "idt.h"

// Global IDT (Interrupt Descriptor Table) array and pointer structure
// The IDT contains 256 entries, one for each possible interrupt/exception
idt_entry_t idt_entries[256];
idt_ptr_t   idt_ptr;

// Set an entry in the IDT (Interrupt Descriptor Table)
// num: Interrupt number (0-255)
// base: 32-bit address of the ISR handler function
// sel: Kernel code segment selector (0x08)
// flags: Access flags (0x8E = present, ring 0, interrupt gate)
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    // Split 32-bit base address into low and high 16-bit parts
    idt_entries[num].base_lo = base & 0xFFFF;
    idt_entries[num].base_hi = (base >> 16) & 0xFFFF;

    idt_entries[num].sel     = sel;
    idt_entries[num].always0 = 0;  // Reserved field, must be 0
    /* Note: To enable user-mode interrupts, uncomment the OR below.
     * It sets the interrupt gate's privilege level to 3 (user mode). */
    idt_entries[num].flags   = flags /* | 0x60 */;
}

// Initialize the IDT pointer structure and load it into the CPU using LIDT instruction
// This sets up the IDT but doesn't populate entries (that's done in init_interrupt_gates)
void init_idt(void) {
    // Set limit to size of IDT minus 1 (CPU requirement)
    idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
    // Set base address to the start of our IDT entries array
    idt_ptr.base  = (uint32_t)&idt_entries;

    // Memory is already zeroed by BSS section, so we don't need to memset here

    // Load the IDT pointer into the CPU using the LIDT instruction
    idt_load((uint32_t)&idt_ptr);
}
