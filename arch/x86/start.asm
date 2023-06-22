MAGIC_NUMBER equ 0x1BADB002
FLAGS equ 0x0
CHECKSUM equ -MAGIC_NUMBER
KERNEL_STACK_SIZE equ 0x4000
KERNEL_INT_STACK_SIZE equ 0x1000

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

_start:
    xor ecx, ecx
_map_lh_pt: 
    mov eax, 0x1000
    mul ecx
    or eax, 0x3
    mov dword [lh_page_table + ecx*4], eax 
    inc ecx
    cmp ecx, 1024
    jne _map_lh_pt

    ;; xor ecx, ecx
;; _map_hh_pt:
    ;; mov eax, 0x1000
    ;; mul ecx
    ;; or eax, 0x3
    ;; mov dword [hh_page_table + ecx*4], eax
    ;; inc ecx
    ;; cmp ecx, 1024
    ;; jl _map_hh_pt

    mov esi, lh_page_table
    or esi, 0x3
    mov [boot_page_dir], esi

    ;; mov esi, hh_page_table
    ;; or esi, 0x3
    mov [boot_page_dir + 768*4], esi

    mov ecx, boot_page_dir
    mov cr3, ecx

    mov ecx, cr0
    or ecx, 0x80000000
    mov cr0, ecx

    ;; mov ecx, _start
    ;; jmp [ecx]
    jmp _hh_start
;; lp:
    ;; jmp lp
    
    
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
    ;; mov eax, 0xCAFEBABE
    mov esp, kernel_stack_end
    ;; mov eax, 3
    
;; call [mmain - 0xC0000000]
    call kernel_main

.loop:
    cli
    hlt
    jmp .loop
   
