MAGIC_NUMBER equ 0x1BADB002
FLAGS equ 0x0
CHECKSUM equ -MAGIC_NUMBER
KERNEL_STACK_SIZE equ 0x4000
KERNEL_INT_STACK_SIZE equ 0x1000

%include "mem/mem.inc"

extern kernel_main
global _start
global kernel_stack_end
global kernel_stack_start
global kernel_int_stack_end

    
section .multiboot.data
align 4
    dd MAGIC_NUMBER
    dd FLAGS
    dd CHECKSUM

section .multiboot.bss
boot_page_dir:
    times 1024 dd 0 
lh_page_table:
    times 1024 dd 0
hh_page_table:
    times 1024 dd 0
    
section .multiboot.text

extern _rodata_start
extern _rodata_end    

_start:
    xor ecx, ecx

_map_lh_pt: 
    mov eax, 0x1000
    mul ecx

    cmp eax, _rodata_start
    jl _rw_page

    cmp eax, _rodata_end
    jl _ro_page
    
_rw_page:   
    or eax, 0x3
    jmp _load_page

_ro_page:
    or eax, 0x1

_load_page
    mov dword [lh_page_table + ecx*4], eax 
    inc ecx
    cmp ecx, 1024
    jne _map_lh_pt

    mov esi, lh_page_table
    or esi, 0x3
    mov [boot_page_dir], esi
    mov [boot_page_dir + 768*4], esi

    mov ecx, boot_page_dir
    mov cr3, ecx

    mov ecx, cr0
    or ecx, CR_PG | CR_WP
    mov cr0, ecx

    jmp _hh_start
    
    
section .bss
align 16

kernel_int_stack_start: 
    resb KERNEL_INT_STACK_SIZE
kernel_int_stack_end:   

kernel_stack_start:
    resb KERNEL_STACK_SIZE 
kernel_stack_end:   

section .text

mmain:
    ret

_hh_start:
    mov esp, kernel_stack_end
    
    call kernel_main

.loop:
    cli
    hlt
    jmp .loop
   
