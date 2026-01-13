#include "isr.h"
#include "idt.h"
#include "framebuffer.h"
#include "pic.h"

// Lookup table for registered C interrupt handlers (indexed by vector 0-255).
static isr_t interrupt_handlers[256];

void register_interrupt_handler(uint8_t n, isr_t handler) {
    interrupt_handlers[n] = handler;
}

static void dispatch_interrupt(registers_t *regs) {
    if (regs == 0) return;
    uint32_t n = regs->int_no;
    if (n >= 256) return;
    isr_t handler = interrupt_handlers[n];
    if (handler != 0) handler(regs);
}

// CPU exceptions (vectors 0-31).
void isr_handler(registers_t *regs) {
    if (regs == 0 || regs->int_no >= 256 || interrupt_handlers[regs->int_no] == 0) {
        write_str("Unhandled Interrupt: ");
        if (regs != 0) write_dec(regs->int_no);
        put_char('\n');
        return;
    }
    dispatch_interrupt(regs);
}

// Hardware IRQs (vectors 32-47 after PIC remap).
void irq_handler(registers_t *regs) {
    if (regs == 0 || regs->int_no >= 256) return;

    // EOI (End Of Interrupt): if from slave, ACK slave then master; otherwise just master.
    if (regs->int_no >= PIC_2_OFFSET) outb(PIC_2_COMMAND, PIC_ACKNOWLEDGE);
    outb(PIC_1_COMMAND, PIC_ACKNOWLEDGE);

    dispatch_interrupt(regs);
}

typedef void (*stub_t)(void);

// Assembly stubs (defined in drivers/interrupts.asm).
extern void isr0(void);  extern void isr1(void);  extern void isr2(void);  extern void isr3(void);
extern void isr4(void);  extern void isr5(void);  extern void isr6(void);  extern void isr7(void);
extern void isr8(void);  extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void); extern void isr15(void);
extern void isr16(void); extern void isr17(void); extern void isr18(void); extern void isr19(void);
extern void isr20(void); extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void); extern void isr27(void);
extern void isr28(void); extern void isr29(void); extern void isr30(void); extern void isr31(void);

extern void irq0(void);  extern void irq1(void);  extern void irq2(void);  extern void irq3(void);
extern void irq4(void);  extern void irq5(void);  extern void irq6(void);  extern void irq7(void);
extern void irq8(void);  extern void irq9(void);  extern void irq10(void); extern void irq11(void);
extern void irq12(void); extern void irq13(void); extern void irq14(void); extern void irq15(void);

void init_interrupt_gates(void) {
    // Install CPU exception handlers.
    static const stub_t isr_stubs[32] = {
        isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7,
        isr8,  isr9,  isr10, isr11, isr12, isr13, isr14, isr15,
        isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
        isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
    };

    const uint16_t KERNEL_CS = 0x08;
    const uint8_t  INT_GATE  = 0x8E;

    for (uint8_t i = 0; i < 32; i++) {
        idt_set_gate(i, (uint32_t)isr_stubs[i], KERNEL_CS, INT_GATE);
    }

    // Remap PIC so IRQs start at vectors 0x20 (32) and 0x28 (40).
    pic_remap(PIC_1_OFFSET, PIC_2_OFFSET);

    // Install hardware IRQ handlers.
    static const stub_t irq_stubs[16] = {
        irq0,  irq1,  irq2,  irq3,  irq4,  irq5,  irq6,  irq7,
        irq8,  irq9,  irq10, irq11, irq12, irq13, irq14, irq15
    };

    for (uint8_t i = 0; i < 16; i++) {
        idt_set_gate(IRQ(i), (uint32_t)irq_stubs[i], KERNEL_CS, INT_GATE);
    }
}
