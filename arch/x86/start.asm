MAGIC_NUMBER equ 0x1BADB002
FLAGS equ 0x0
CHECKSUM equ -MAGIC_NUMBER
KERNEL_STACK_SIZE equ 0x4000

extern kernel_main
global _start
global kernel_stack_end

section .bss
align 16
kernel_stack_start:
    resb KERNEL_STACK_SIZE 
kernel_stack_end:   

section .text
align 4
    dd MAGIC_NUMBER
    dd FLAGS
    dd CHECKSUM

_start:
    ;; mov eax, 0xCAFEBABE
    mov esp, kernel_stack_end
    call kernel_main

.loop:
    cli
    hlt
    jmp .loop
   
