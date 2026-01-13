global idt_load

idt_load:
    mov eax, [esp+4]
    lidt [eax]
    ret

; Mark stack as non-executable (silences ld warning about missing .note.GNU-stack)
section .note.GNU-stack noalloc noexec nowrite progbits
