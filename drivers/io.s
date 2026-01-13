global inb

; inb(unsigned short port) -> unsigned char
; stack: [esp]  return address
;        [esp+4] port
inb:
    mov dx, [esp + 4]   ; port number
    in  al, dx          ; read byte from port
    ret

global outb

; outb(unsigned short port, unsigned char value)
; stack: [esp] return address
;        [esp+4] port
;        [esp+8] value
outb:
    mov dx, [esp + 4]    ; port
    mov al, [esp + 8]    ; value
    out dx, al
    ret

; Mark stack as non-executable (silences ld warning about missing .note.GNU-stack)
section .note.GNU-stack noalloc noexec nowrite progbits