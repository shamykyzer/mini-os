# Makefile

# Tools
AS    = nasm
CC    = gcc
LD    = ld
QEMU  = qemu-system-i386

# Monitor port for QEMU telnet monitor (override with `make run-curses MON_PORT=45555`)
MON_PORT ?= 45454

CFLAGS  = -m32 -ffreestanding -O2 -Wall -Wextra \
          -nostdlib -nostdinc -fno-builtin -fno-stack-protector -c \
          -I$(DRV_DIR) -I$(SRC_DIR) -I$(BUILD_DIR)
ASFLAGS = -f elf
LDFLAGS = -T $(DRV_DIR)/link.ld -melf_i386

# Paths and files
BUILD_DIR = build
SRC_DIR   = source
DRV_DIR   = drivers
ISO_DIR   = iso

KERNEL = kernel.elf
ISO    = os.iso
VERSION_H = $(BUILD_DIR)/version.h

OBJS = $(BUILD_DIR)/loader.o \
       $(BUILD_DIR)/idt_load.o \
       $(BUILD_DIR)/interrupts.o \
       $(BUILD_DIR)/idt.o \
       $(BUILD_DIR)/isr.o \
       $(BUILD_DIR)/kernel.o  \
       $(BUILD_DIR)/menu.o  \
       $(BUILD_DIR)/calc.o  \
       $(BUILD_DIR)/tictactoe.o  \
       $(BUILD_DIR)/framebuffer.o \
       $(BUILD_DIR)/io.o \
       $(BUILD_DIR)/pic.o \
       $(BUILD_DIR)/keyboard.o

.PHONY: all run run_log clean

# Build everything: kernel + ISO
all: $(ISO)

.PHONY: FORCE
FORCE:

# Create build dir
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Generate a small header containing the git commit count.
# This lets the kernel print a version like v<hundreds>.<tens>.<ones> where the digits are derived
# from the total number of commits (e.g., 41 commits => v0.4.1; 137 commits => v1.3.7).
# The rule runs on every `make`, but only updates the file if the content changed.
$(VERSION_H): FORCE | $(BUILD_DIR)
	@count=$$(git rev-list --count HEAD 2>/dev/null || echo 0); \
	tmp="$(BUILD_DIR)/version.h.tmp"; \
	printf "%s\n" "#ifndef VERSION_H" > $$tmp; \
	printf "%s\n" "#define VERSION_H" >> $$tmp; \
	printf "%s\n" "#define OS_GIT_COMMIT_COUNT $$count" >> $$tmp; \
	printf "%s\n" "#endif" >> $$tmp; \
	if [ ! -f $@ ] || ! cmp -s $$tmp $@; then mv $$tmp $@; else rm $$tmp; fi

# Link kernel
$(KERNEL): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $(KERNEL)

# Build ISO using menu.lst + stage2_eltorito (lab style)
$(ISO): $(KERNEL)
	# ensure latest kernel is on the ISO
	cp $(KERNEL) $(ISO_DIR)/boot/kernel.elf
	genisoimage -R \
		-b boot/grub/stage2_eltorito \
		-no-emul-boot \
		-boot-load-size 4 \
		-A os \
		-input-charset utf8 \
		-quiet \
		-boot-info-table \
		-o $(ISO) \
		$(ISO_DIR)

# Assemble loader.asm
$(BUILD_DIR)/loader.o: $(DRV_DIR)/loader.asm | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

# Compile kernel.c
$(BUILD_DIR)/kernel.o: $(SRC_DIR)/kernel.c $(SRC_DIR)/menu.h $(VERSION_H) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

# Compile menu.c
$(BUILD_DIR)/menu.o: $(SRC_DIR)/menu.c $(SRC_DIR)/menu.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

# Compile calc.c
$(BUILD_DIR)/calc.o: $(SRC_DIR)/calc.c $(SRC_DIR)/menu.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

# Compile tictactoe.c
$(BUILD_DIR)/tictactoe.o: $(SRC_DIR)/tictactoe.c $(SRC_DIR)/menu.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

# Compile framebuffer.c
$(BUILD_DIR)/framebuffer.o: drivers/framebuffer.c drivers/framebuffer.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) drivers/framebuffer.c -o $@

# Assemble io.s
$(BUILD_DIR)/io.o: $(DRV_DIR)/io.s | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

# Compile pic.c
$(BUILD_DIR)/pic.o: drivers/pic.c drivers/pic.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) drivers/pic.c -o $@

# Compile keyboard.c
$(BUILD_DIR)/keyboard.o: drivers/keyboard.c drivers/keyboard.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) drivers/keyboard.c -o $@

# Compile idt.c
$(BUILD_DIR)/idt.o: $(DRV_DIR)/idt.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

# Compile isr.c
$(BUILD_DIR)/isr.o: $(DRV_DIR)/isr.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

# Assemble gdt_flush.asm
# $(BUILD_DIR)/gdt_flush.o: $(SRC_DIR)/gdt_flush.asm | $(BUILD_DIR)
# 	$(AS) $(ASFLAGS) $< -o $@

# Assemble idt_load.asm
$(BUILD_DIR)/idt_load.o: $(DRV_DIR)/idt_load.asm | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

# Assemble interrupts.asm
$(BUILD_DIR)/interrupts.o: $(DRV_DIR)/interrupts.asm | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

# Run in QEMU with curses display (per module leader)
run: $(ISO)
	$(QEMU) -display curses \
		-serial mon:stdio \
		-device isa-debug-exit,iobase=0xf4,iosize=0x04 \
		-boot d -cdrom $(ISO) \
		-m 32 -d cpu -D logQ.txt

run-curses: $(ISO)
	$(QEMU) -display curses \
		-monitor telnet::$(MON_PORT),server,nowait \
	-device isa-debug-exit,iobase=0xf4,iosize=0x04 \
	-chardev stdio,id=char0 \
	-serial chardev:char0 \
	-boot d \
	-cdrom $(ISO) \
	-m 32 \
	-d cpu \
	-no-reboot \
	-D logQ.txt

# Run headless and log CPU state (for CAFEBABE / Task 1)
run_log: $(ISO)
	$(QEMU) -nographic \
		-device isa-debug-exit,iobase=0xf4,iosize=0x04 \
		-boot d -cdrom $(ISO) \
		-m 32 -d cpu -D logQ.txt

# Clean build
clean:
	rm -f $(BUILD_DIR)/*.o $(KERNEL) $(ISO) logQ.txt
