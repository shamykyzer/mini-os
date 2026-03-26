ENTRY(loader)

SECTIONS {
    . = 0x00100000; /* Kernel loaded at 1MB */

    _kernel_start = .;

    .text ALIGN(4K) : {
        _text_start = .;
        *(.text)
        _text_end = .;
    }

    .rodata ALIGN(4K) : {
        _rodata_start = .;
        *(.rodata*)
        _rodata_end = .;
    }

    .data ALIGN(4K) : {
        _data_start = .;
        *(.data)
        _data_end = .;
    }

    .bss ALIGN(4K) : {
        _bss_start = .;
        *(COMMON)
        *(.bss)
        _bss_end = .;
    }

    _kernel_end = .;
}
