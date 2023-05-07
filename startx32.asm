MAGIC_NUMBER equ 0x1BADB002
FLAGS equ 0x0
CHECKSUM equ -MAGIC_NUMBER
KERNEL_STACK_SIZE equ 0x1000

extern kernel_main
    
global loader

section .text
align 8
    dd MAGIC_NUMBER
    dd FLAGS
    dd CHECKSUM

loader:
    mov rax, 0xCAFEBABE
    mov rsp, kernel_stack + KERNEL_STACK_SIZE
    call kernel_main

section .bss
align 8
kernel_stack:
    resb KERNEL_STACK_SIZE 
    
