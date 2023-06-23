section .text
global enable_paging
global load_page_dir

%include "mem/mem.inc"
    
extern fb_print_num
global print_cr0

global disable_paging
disable_paging
    push ebx
    mov ebx, cr0
    and ebx, ~CR_PG
    mov cr0, ebx
    pop ebx
    ret

global copy_page_data
copy_page_data:
    push ebp
    mov ebp, esp

    push ebx
    pushfd
    cli

    mov esi, [ebp - 8]
    mov edi, [ebp - 12]
    call disable_paging

_copy
    mov ecx, 0
    mov ebx, [esi + ecx*4]
    mov [edi + ecx*4], ebx
    inc ecx

    cmp ecx, 1024
    jl _copy

    call enable_paging

    popfd
    pop ebx
    pop ebp
    ret

print_cr0:
    mov eax, cr0
    push eax
    call fb_print_num
    pop eax
    ret
    
load_page_dir:
    mov eax, [esp + 4]
    mov cr3, eax
    ret

extern _kernel_end
extern fb_print_hex
enable_paging:
    push ebx
    
    mov ebx, cr0
    or ebx, CR_PG | CR_WP | 1
    mov cr0, ebx

    pop ebx
    ret

global switch_page_dir_asm
switch_page_dir_asm:
    push ebx

    mov eax, [esp + 4]

    mov ebx, 0
    ;; and ebx, 0
    mov cr0, ebx

    mov cr3, eax

    mov ebx, cr0
    or ebx, CR_PG | CR_WP | 3
    mov cr0, ebx

    pop ebx
    ret
    
