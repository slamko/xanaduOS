ENTRY(_start)

SECTIONS {
    . = 0x00100000;

    .multiboot.data ALIGN (0x1000) :
    {
        *(.multiboot.data)
    }

    .multiboot.text ALIGN (0x1000) :
    {
        *(.multiboot.text)
    }

    .multiboot.bss ALIGN (0x1000) :
    {
        *(.multiboot.bss)
    }

    . += 0x4000000;

    .usermode.text ALIGN (0x1000) :
    {
        _usermode_text_start = .;
        *(.usermode.text)
    }
    .usermode.data ALIGN (0x1000) :
    {
        *(.usermode.data)
        _usermode_text_end = .;
    }

    _virt_kernel_addr = 0xC0000000;
    . += 0xBC000000;
    .text ALIGN (0x1000) : AT(ADDR(.text) - 0xC0000000)
    {
        _rodata_start = .;
        *(.text)
    }
     .rodata ALIGN (0x1000) : AT(ADDR(.rodata) - 0xC0000000)
    {
        *(.rodata)
        _rodata_end = .;
    }
     .data ALIGN (0x1000) : AT(ADDR(.data) - 0xC0000000)
    {
        *(.data)
    }
     .bss ALIGN (0x1000) : AT(ADDR(.bss) - 0xC0000000)
    {
        *(COMMON)
        *(.bss)
    }

    . = ALIGN(0x1000);
    _kernel_end = .;
}
