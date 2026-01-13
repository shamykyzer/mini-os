#include "pic.h"

/* Remap the PIC (Programmable Interrupt Controller) to new interrupt vector offsets
 * This moves the hardware IRQ interrupts (0-15) to a different range in the IDT
 * to avoid conflicts with CPU exceptions (0-31).
 * Standard remapping: offset1 = 0x20 (32) for master PIC, offset2 = 0x28 (40) for slave PIC
 * After remapping: IRQ0-7 map to interrupts 32-39, IRQ8-15 map to interrupts 40-47
 */
void pic_remap(s32int offset1, s32int offset2) {
    // Save current interrupt masks before remapping
    u8int a1 = inb(PIC_1_DATA);
    u8int a2 = inb(PIC_2_DATA);

    /* Start initialization sequence in cascade mode (ICW1) */
    /* PIC_ICW1_INIT = start initialization, PIC_ICW1_ICW4 = ICW4 needed */
    outb(PIC_1_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    outb(PIC_2_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);

    /* Set vector offsets (ICW2) - where IRQ interrupts will appear in IDT */
    outb(PIC_1_DATA, offset1);  // Master PIC: IRQ0-7 -> interrupts offset1 to offset1+7
    outb(PIC_2_DATA, offset2);  // Slave PIC: IRQ8-15 -> interrupts offset2 to offset2+7

    /* Configure cascade mode (ICW3) */
    /* Tell master PIC that slave is connected at IRQ2 (bit 2 = 4) */
    outb(PIC_1_DATA, 4);
    /* Tell slave PIC its cascade identity (it's connected as IRQ2 on master) */
    outb(PIC_2_DATA, 2);

    /* Set 8086 mode (ICW4) - use 8086/8088 mode instead of 8080 mode */
    outb(PIC_1_DATA, PIC_ICW4_8086);
    outb(PIC_2_DATA, PIC_ICW4_8086);

    /* Restore the saved interrupt masks */
    outb(PIC_1_DATA, a1);
    outb(PIC_2_DATA, a2);
}

/* Send End-of-Interrupt (EOI) command to PICs
 * This must be called at the end of an IRQ handler to allow further interrupts
 * If the interrupt came from the slave PIC (interrupts 40-47), acknowledge both
 * Otherwise, just acknowledge the master PIC
 * Note: This function exists but is not currently used - EOI is handled in isr.c
 */
void pic_acknowledge(u32int interrupt) {
    // If interrupt came from slave PIC (offset 40-47), acknowledge slave first
    if (interrupt >= PIC_2_OFFSET && interrupt <= PIC_2_END) {
        outb(PIC_2_COMMAND_PORT, PIC_ACKNOWLEDGE);
    }
    // Always acknowledge master PIC (handles interrupts 32-39 and cascades slave)
    outb(PIC_1_COMMAND_PORT, PIC_ACKNOWLEDGE);
}
