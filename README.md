# SnowOS — Bare-Metal x86-32 Kernel

A freestanding 32-bit x86 kernel targeting the IA-32 architecture. Boots via GRUB/Multiboot, provides VGA text-mode console I/O, interrupt-driven keyboard input, and an interactive shell with sub-applications. All claims are quantified and verifiable via built-in instrumentation (`sysinfo`, `bench`).

---

## System Specifications

| Parameter | Value | Source / Verification |
|---|---|---|
| ISA | IA-32 (x86-32) | `gcc -m32`, `ld -melf_i386` |
| Boot protocol | Multiboot 1 | Magic `0x1BADB002` in `loader.asm` |
| Kernel load address | `0x00100000` (1 MiB) | Linker script `link.ld` |
| Binary size | 24,738 B `.text` / 364 B `.data` / 7,616 B `.bss` | `make size` |
| Display | VGA text mode: 80 x 25 cells, 16 colors | `FRAMEBUFFER_ADDRESS = 0xB8000` |
| Input | PS/2 keyboard via IRQ1 (port `0x60`) | Scancode table in `keyboard.c` |
| Interrupt controller | Dual 8259A PIC | Remapped to vectors `0x20`-`0x2F` |
| IDT gates installed | 48 | 32 CPU exceptions + 16 hardware IRQs |
| Kernel stack | 4,096 bytes | BSS-allocated in `loader.asm` |
| Input buffer | 256 bytes (circular ring) | `keyboard.c` |
| Command buffer | 128 bytes | `kernel.c` shell loop |
| Heap / dynamic memory | 0 bytes | All storage is static or BSS |
| Scheduling model | Single-threaded, blocking I/O | `hlt` in keyboard wait loop |
| C runtime | None | `-ffreestanding -nostdlib -nostdinc` |
| Emulation target | QEMU `qemu-system-i386` | `-m 32` (32 MiB guest RAM) |

---

## Repository Structure

```
source/
  kernel.c           Main kernel entry, shell loop, bench/sysinfo commands
  menu.c / menu.h    Help menu, calculator/tictactoe entry, Task 2 helpers
  calc.c             Calculator sub-shell
  tictactoe.c        TicTacToe game

drivers/
  loader.asm         Multiboot header, stack setup, call to kmain
  link.ld            Linker script (kernel @ 1 MiB, section boundary symbols)
  types.h            Fixed-width integer types (uint8_t - uint64_t)
  idt.c / idt.h      Interrupt Descriptor Table setup + LIDT
  idt_load.asm       Assembly wrapper for LIDT instruction
  isr.c / isr.h      ISR/IRQ dispatch, IDT gate installation, IRQ counters
  interrupts.asm     ISR/IRQ assembly stubs (32 exceptions + 16 IRQs)
  io.s / io.h        I/O port wrappers (inb / outb)
  framebuffer.c / .h VGA text-mode driver: cursor, colors, scroll, hex output
  keyboard.c / .h    PS/2 keyboard driver: IRQ1 handler, circular buffer
  pic.c / pic.h      8259A PIC remapping and EOI

iso/boot/grub/
  menu.lst           GRUB boot menu (entry: "SnowOS")
  stage2_eltorito    GRUB stage2 bootloader binary

build/               (generated)
  *.o                Object files
  version.h          Git commit count (#define OS_GIT_COMMIT_COUNT)

kernel.elf           (generated) Linked kernel ELF binary
os.iso               (generated) Bootable ISO image
```

---

## Toolchain and Build

### Required Tools

| Tool | Purpose |
|---|---|
| `nasm` | Assembler: Multiboot loader, IDT load, ISR/IRQ stubs, I/O ports |
| `gcc` (with `-m32`) | C compiler: all kernel and driver C sources |
| `ld` | GNU linker: ELF i386 target with custom linker script |
| `genisoimage` | ISO 9660 image creation for GRUB boot |
| `qemu-system-i386` | IA-32 system emulator |

Install on Debian/Ubuntu:
```sh
sudo apt install make gcc gcc-multilib binutils nasm genisoimage qemu-system-x86
```

### Compiler Flags

| Flag | Effect |
|---|---|
| `-m32` | Generate 32-bit (IA-32) code |
| `-ffreestanding` | No hosted environment assumed |
| `-O2` | Optimization level 2 |
| `-nostdlib` | No standard library linking |
| `-nostdinc` | No standard include paths |
| `-fno-builtin` | Disable GCC built-in function replacement |
| `-fno-stack-protector` | No stack canary (no runtime support available) |
| `-Wall -Wextra` | Enable comprehensive warnings |

### Linker Flags

| Flag | Effect |
|---|---|
| `-T drivers/link.ld` | Custom linker script: load at 1 MiB, 4 KiB-aligned sections |
| `-melf_i386` | 32-bit ELF output format |

### Build Commands

| Command | Action |
|---|---|
| `make` | Build `kernel.elf` and `os.iso` |
| `make run` | Boot in QEMU (curses display, serial monitor on stdio) |
| `make run-curses` | Boot in QEMU (curses display, telnet monitor on port 45454) |
| `make run_log` | Boot headless, CPU state logged to `logQ.txt` |
| `make size` | Report `.text`, `.data`, `.bss` sizes via `size(1)` |
| `make clean` | Remove all build artifacts |

### Versioning

`make` generates `build/version.h` containing `OS_GIT_COMMIT_COUNT` from `git rev-list --count HEAD`. The shell `version` command formats this as `SnowOS v<hundreds>.<tens>.<ones> (alpha)` (e.g., 41 commits = `v0.4.1`). Deterministic, no RTC or filesystem required.

---

## Memory Map

### Physical Address Space

| Address Range | Size | Contents |
|---|---|---|
| `0x00000000` - `0x000003FF` | 1,024 B | Real-mode Interrupt Vector Table (BIOS) |
| `0x000B8000` - `0x000B8F9F` | 4,000 B | VGA text buffer (80 x 25 x 2 bytes/cell) |
| `0x00100000` - `0x00100000 + .text` | ~24 KiB | Kernel code (`.text` section) |
| ... | ~1 KiB | Read-only data (`.rodata` section) |
| ... | ~1 KiB | Initialized data (`.data` section) |
| ... | ~8 KiB | BSS: kernel stack, IDT, handler table, buffers |

Run `sysinfo` at the shell prompt to see exact addresses and byte counts for the current build.

### Kernel Section Layout

Sections are ordered and 4 KiB-aligned per `link.ld`. Boundary symbols (`_text_start`, `_text_end`, etc.) are exported for runtime introspection.

| Section | Contents | Alignment |
|---|---|---|
| `.text` | Executable code (C + ASM) | 4,096 B |
| `.rodata` | String literals, lookup tables, constants | 4,096 B |
| `.data` | Initialized global/static variables | 4,096 B |
| `.bss` | Kernel stack (4 KiB), IDT (2 KiB), handler table (1 KiB), keyboard buffer (256 B) | 4,096 B |

### VGA Cell Format

Each of the 2,000 display cells occupies 2 bytes at `0xB8000`:

| Byte | Bits | Field |
|---|---|---|
| 0 | 7:0 | ASCII character code |
| 1 | 3:0 | Foreground color (4-bit, 16 values) |
| 1 | 7:4 | Background color (4-bit, 16 values) |

Total framebuffer size: 80 x 25 x 2 = **4,000 bytes**.

---

## Architecture

### Boot Sequence

| Step | Component | Action | Verifiable by |
|---|---|---|---|
| 1 | BIOS | POST, loads GRUB from ISO | QEMU boot |
| 2 | GRUB | Validates Multiboot header (`0x1BADB002`) | Boot succeeds or fails |
| 3 | GRUB | Loads `kernel.elf` to `0x00100000` per ELF headers | `sysinfo` shows `_text_start = 0x00100000` |
| 4 | GRUB | Jumps to `loader` entry point | — |
| 5 | `loader.asm` | Sets ESP to 4 KiB stack top (BSS) | — |
| 6 | `loader.asm` | Calls `sum_of_three(1,2,3)` via cdecl ABI | `task2` returns sum=6 |
| 7 | `loader.asm` | Calls `kmain()` | Shell prompt appears |
| 8 | `kmain` | `init_framebuffer()` — clears 2,000 VGA cells | Screen clears |
| 9 | `kmain` | `init_idt()` — loads 2,048-byte IDT via LIDT | — |
| 10 | `kmain` | `init_interrupt_gates()` — installs 48 gates, remaps PIC | — |
| 11 | `kmain` | `init_keyboard()` — registers IRQ1, unmasks on PIC | — |
| 12 | `kmain` | `sti` — enables maskable interrupts | `Status: IRQ on` printed |
| 13 | `kmain` | Enters shell loop | `snowos>` prompt appears |

### Interrupt Subsystem

**IDT Configuration:**

| Property | Value |
|---|---|
| Table size | 256 entries x 8 bytes = 2,048 bytes |
| Populated gates | 48 (vectors 0-31 + 32-47) |
| Gate type | `0x8E` — 32-bit interrupt gate, ring 0, present |
| Segment selector | `0x08` — kernel code segment |

**PIC Remapping:**

| PIC | Default Vectors | Remapped Vectors | Command Port | Data Port |
|---|---|---|---|---|
| Master (8259A #1) | `0x08`-`0x0F` | `0x20`-`0x27` | `0x20` | `0x21` |
| Slave (8259A #2) | `0x70`-`0x77` | `0x28`-`0x2F` | `0xA0` | `0xA1` |

Remapping is required because default IRQ0-7 vectors (`0x08`-`0x0F`) overlap CPU exception vectors.

**IRQ Dispatch Path (measured per-IRQ via counters):**

1. Hardware asserts IRQ line -> PIC delivers interrupt vector to CPU
2. CPU indexes IDT entry, pushes EFLAGS/CS/EIP, jumps to ASM stub
3. ASM stub: `PUSHA`, saves DS, sets kernel data segment, calls C `irq_handler()`
4. `irq_handler()`: increments per-IRQ counter, sends EOI to PIC, dispatches to registered callback
5. ASM stub: restores DS, `POPA`, `IRET` to interrupted code

Run `sysinfo` to see cumulative per-IRQ event counts since boot.

### Framebuffer Driver

| Property | Value |
|---|---|
| Base address | `0x000B8000` |
| Resolution | 80 columns x 25 rows = 2,000 cells |
| Cell size | 2 bytes (character + attribute) |
| Buffer size | 4,000 bytes |
| Color depth | 4-bit foreground x 4-bit background (256 combinations) |
| Cursor control | VGA I/O ports `0x3D4` (command), `0x3D5` (data) |
| Scroll method | Byte-by-byte row copy upward, clear last row |

Run `bench` to measure cycles per `clear_screen()`, `write_str()`, and `put_char()`.

### Keyboard Driver

| Property | Value |
|---|---|
| Interface | PS/2: port `0x60` (data), `0x64` (status/command) |
| IRQ | IRQ1 (vector 33 after PIC remap) |
| Scancode set | Set 1 (128-entry US QWERTY lookup table) |
| Buffer | 256-byte circular ring buffer |
| Read model | Blocking: CPU executes `hlt` until next interrupt |
| Key filtering | Key-down only (bit 7 of scancode = release, ignored) |
| Features | ASCII translation, echo to VGA, backspace handling |

---

## Performance Characterization

### Methodology

All measurements use the x86 `RDTSC` (Read Time-Stamp Counter) instruction, returning CPU cycle counts. The `bench` shell command executes each operation, captures start/end TSC values, and reports elapsed cycles. Results vary by host CPU frequency and QEMU TCG/KVM configuration.

### Benchmark Suite (`bench` command)

| # | Test | Operation | Metric |
|---|---|---|---|
| 1 | Screen clear | `clear_screen()`: write 2,000 VGA cells | Total cycles, cycles/cell |
| 2 | String write | `write_str()`: 80-character string (1 row) | Total cycles, cycles/char |
| 3 | Char write | `put_char()`: 2,000 individual calls (1 screen) | Total cycles, cycles/char |
| 4 | Int format | `write_dec()`: 100 integer-to-decimal conversions | Total cycles, cycles/call |

### System Instrumentation (`sysinfo` command)

Reports at runtime:

| Category | Data |
|---|---|
| Memory layout | `.text`, `.rodata`, `.data`, `.bss` start/end addresses and sizes (bytes) |
| Kernel footprint | Total bytes from `_kernel_start` to `_kernel_end` |
| System parameters | Framebuffer geometry, IDT gate count, PIC vector range, stack/buffer sizes |
| IRQ counters | Per-IRQ line event count and total since boot |

---

## Shell Command Reference

| Command | Arguments | Description |
|---|---|---|
| `help` | — | Display boxed command list |
| `clear` | — | Clear screen (fill 2,000 cells) |
| `echo` | `<text>` | Print text to console |
| `version` | — | Print version from git commit count |
| `task1` | — | VGA demo: 16 foreground colors x 4 backgrounds, cursor positioning, scroll |
| `task2` | `[a b c]` | Compute sum, max, product of 3 integers (default: 1, 2, 3) |
| `calc` | — | Enter calculator sub-shell (`calc>`) |
| `tictactoe` | — | Enter TicTacToe game (`ttt>`) |
| `sysinfo` | — | Print memory layout, IRQ counters, system parameters |
| `bench` | — | Run RDTSC-based performance benchmark suite |
| `pink` | — | Toggle color theme (cyan / pink) |
| `shutdown` | — | Halt: write `0x2000` to port `0x604`, fallback `cli; hlt` |

### Calculator Sub-Shell (`calc>`)

| Command | Computation | Error Handling |
|---|---|---|
| `add a b` | a + b | Rejects non-integer or wrong argument count |
| `sub a b` | a - b | " |
| `mul a b` | a * b | " |
| `div a b` | a / b (integer, truncates toward 0) | Division by zero: explicit check |
| `mod a b` | a mod b | Division by zero: explicit check |
| `pow a b` | a^b (integer) | Negative exponent: explicit check |
| `min a b` | min(a, b) | Rejects non-integer |
| `max a b` | max(a, b) | " |
| `mean a b` | (a + b) / 2 (truncated) | " |
| `quit` | — | Return to main shell |

Integer range: 32-bit signed (-2,147,483,648 to 2,147,483,647).

### TicTacToe (`ttt>`)

| Input | Action |
|---|---|
| `1`-`9` | Place mark at board position (1=top-left, 9=bottom-right) |
| `restart [x\|o]` | Reset board; optionally set starting player |
| `clear` | Redraw current board |
| `help` | Print command list |
| `quit` | Return to main shell |

**Game parameters:**

| Property | Value |
|---|---|
| Board representation | `uint8_t[9]`, values: 0=empty, 1=X, 2=O |
| Win detection | 8 lines: 3 rows + 3 columns + 2 diagonals (table-driven) |
| Win check complexity | O(8) per move (constant) |
| Draw detection | Linear scan of 9 cells for any empty |
| Colors | X = light red (12), O = light green (10) |

---

## Input Parsing

All parsing is freestanding (no libc). Helpers are in `drivers/framebuffer.c`.

| Function | Behavior | Complexity |
|---|---|---|
| `k_skip_ws(s)` | Advance past spaces and tabs | O(n) |
| `k_match_cmd(line, cmd, &rest)` | Token match with boundary check; `"add"` does not match `"addd"` | O(len(cmd)) |
| `k_parse_int(s, &val, &end)` | Signed decimal parse; `+`/`-` prefix supported | O(digits) |
| `k_parse_two_ints(s, &a, &b)` | Parse exactly 2 integers; reject trailing non-whitespace | O(n) |
| `k_parse_three_ints(s, &a, &b, &c)` | Parse exactly 3 integers; reject trailing non-whitespace | O(n) |

---

## Verification Protocol

### Build Verification

| # | Test | Command | Expected Result |
|---|---|---|---|
| 1 | Compilation | `make` | Exit 0; `kernel.elf` and `os.iso` produced |
| 2 | Zero warnings | `make 2>&1 \| grep warning` | No output |
| 3 | Section sizes | `make size` | Non-zero `.text`; total ~32 KiB |
| 4 | Clean rebuild | `make clean && make` | Identical output to test 1 |

### Boot Verification

| # | Test | Expected Output |
|---|---|---|
| 5 | Kernel boots | `SnowOS vX.Y.Z (alpha)` banner displayed |
| 6 | IRQ enabled | `Status: IRQ on \| keyboard ready` printed |
| 7 | Shell prompt | `snowos>` appears; keyboard input accepted |

### Functional Verification

| # | Input | Expected Output | Tests |
|---|---|---|---|
| 8 | `help` | Boxed command list, 14 entries | Help rendering |
| 9 | `echo hello` | `hello` | String echo |
| 10 | `version` | `SnowOS vX.Y.Z (alpha)` | Version formatting |
| 11 | `task2` | sum=6, max=3, product=6 | Default args (1,2,3) |
| 12 | `task2 7 4 5` | sum=16, max=7, product=140 | Custom args |
| 13 | `task2 -1 2 -3` | sum=-2, max=2, product=6 | Signed args |
| 14 | `task2 1 2` | `Usage: task2 <a> <b> <c>` | Arg count validation |
| 15 | `task2 1 2 3 4` | `Usage: task2 <a> <b> <c>` | Extra arg rejection |
| 16 | `sysinfo` | Section addresses + sizes, IRQ counters | Runtime introspection |
| 17 | `bench` | 4 cycle measurements with per-unit costs | RDTSC instrumentation |

### Calculator Verification

| # | Input (in `calc>`) | Expected | Tests |
|---|---|---|---|
| 18 | `add 2 3` | `5` | Basic arithmetic |
| 19 | `add -2 3` | `1` | Signed operands |
| 20 | `div 10 3` | `3` | Integer truncation |
| 21 | `div 5 0` | `Error: divide by zero` | Zero divisor check |
| 22 | `pow 2 10` | `1024` | Exponentiation |
| 23 | `pow 2 -1` | `Error: exp must be >= 0` | Negative exponent check |
| 24 | `addd 1 2` | `Unknown calculator command` | Token boundary safety |
| 25 | `add 1 2x` | `Usage: add <a> <b>` | Trailing garbage rejection |
| 26 | `quit` | Returns to `snowos>` | Exit behavior |

### TicTacToe Verification

| # | Input Sequence (in `ttt>`) | Expected | Tests |
|---|---|---|---|
| 27 | `1`, `4`, `2`, `5`, `3` | `Winner: X` | Row win (top) |
| 28 | `1`, `2`, `5`, `3`, `9` | `Winner: X` | Diagonal win |
| 29 | `1` then `1` | `That square is taken` | Occupied cell rejection |
| 30 | `restart o`, then move | O goes first | Restart with player choice |

### Whitespace / Edge Cases

| # | Input | Expected | Tests |
|---|---|---|---|
| 31 | `   task2    1   2   3` | sum=6, max=3, product=6 | Leading + repeated whitespace |
| 32 | (empty, just Enter) | New prompt, no output | Empty input |
| 33 | `task2x 1 2 3` | `Unknown command: task2x 1 2 3` | Token boundary at shell level |

---

## Known Constraints

| Constraint | Quantification | Impact |
|---|---|---|
| No heap allocator | 0 bytes dynamic memory | All buffers fixed at compile time |
| No filesystem | 0 file operations | No persistent storage |
| Display fixed | 80 x 25 = 2,000 cells (4,000 B) | No graphics mode, no resize |
| Keyboard layout | US QWERTY, 128-entry table | No international layouts, no modifier keys |
| Input buffer | 256 bytes; overflow drops silently | Fast typing during slow operations may lose chars |
| Command buffer | 128 bytes; longer input truncated | Max command length: 127 characters + NUL |
| Integer range | 32-bit signed | -2,147,483,648 to 2,147,483,647 |
| Stack depth | 4,096 bytes; no overflow detection | Deep recursion or large locals will corrupt memory |
| Concurrency | Single-threaded; `hlt` blocks CPU | No background tasks possible |
| Timer | No PIT/HPET/RTC driver | No wall-clock time; cycle counts via RDTSC only |

---

## References

1. E. Helin and A. Renberg, *The Little Book of OS Development*, 2015.
2. Intel Corporation, *Intel 64 and IA-32 Architectures Software Developer's Manual*, Vol. 3A, Ch. 6: Interrupt and Exception Handling.
3. Intel Corporation, *Intel 64 and IA-32 Architectures Software Developer's Manual*, Vol. 2B: RDTSC instruction reference.
4. OSDev Wiki, "8259 PIC," https://wiki.osdev.org/PIC.
5. OSDev Wiki, "Interrupt Descriptor Table," https://wiki.osdev.org/IDT.
6. GNU Multiboot Specification, version 0.6.96.
7. QEMU Documentation, https://www.qemu.org/docs/master/.

---

## License

MIT. See [LICENSE](LICENSE).
