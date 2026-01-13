global loader
extern kmain
extern sum_of_three

MAGIC_NUMBER equ 0x1BADB002
FLAGS equ 0
CHECKSUM equ -(MAGIC_NUMBER + FLAGS)

section .text
align 4

dd MAGIC_NUMBER
dd FLAGS
dd CHECKSUM

loader:
    mov esp, stack_top

    push dword 3
    push dword 2
    push dword 1
    
    call sum_of_three
    
    add esp, 12

    call kmain

.hang:
    jmp .hang

section .bss
align 4
stack_bottom:
    resb 4096
stack_top:

; Mark stack as non-executable (silences ld warning about missing .note.GNU-stack)
section .note.GNU-stack noalloc noexec nowrite progbits
