MBALIGN equ 1 << 0
MEMINFO equ 1 << 1
MBFLAGS equ MBALIGN | MEMINFO
MAGIC equ 0x1BADB002           
CHECKSUM equ -(MAGIC + MBFLAGS)
    
extern _kernel_start
extern _kernel_end
extern kernel_main
    
section .multiboot.data
align 
4
    dd MAGIC
    dd MBFLAGS
    dd CHECKSUM

section .bss
align 16
stack_bottom:   
resb 0x4000
stack_top:  

align 4096
boot_page_directory:    
resb 4096
boot_page_table1:   
resb 4096

section .multiboot.text 
global _start
_start: 
    mov edi, [boot_page_table1 - 0xC0000000]
    mox esi, 0
    mov ecx, 1023
    
mmap:   
    cmp esi, _kernel_start
    jl page_map

    cmp esi, _kernel_end - 0xC0000000
    jge vga_map

    mov edx, esi
    or edx, 0x3
    mov [edi], edx

page_map:   
    add esi, 0x1000
    add edi, 4
    loop mmap
    
vga_map:    
    mov eax, boot_page_table1 - 0xC0000000 + 1023 * 4 
    mov ebx, [0x000B8000 | 0x3]
    mov [eax], ebx
    mov eax, boot_page_directory - 0xC0000000 + 0 
    mov ebx, [boot_page_table1 - 0xC0000000 + 0x3] 
    mov [eax], ebx
    mov eax, boot_page_directory - 0xC0000000 + 768 * 4
    mov ebx, [boot_page_table1 - 0xC0000000 + 0x3]
    mov [eax], ebx
    
    mov ecx, [boot_page_directory - 0xC0000000]
    mov cr3, ecx
    mov ecx, cr0
    or ecx, 0x80010000
    mov cr0, ecx

    lea ecx, kernel_start
    jmp ecx

section .text

kernel_start:   
    mov dword [boot_page_directory], 0
    mov ecx, cr3
    mov cr3, ecx
    mov esp, stack_top
    
    call kernel_main

    cli
halt_loop:  
    hlt
    jmp halt_loop

    

