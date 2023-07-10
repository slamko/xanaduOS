section .text
global enable_paging
global load_page_dir

PGE equ 0x80

%include "mem/mem.inc"
    
extern fb_print_num
global print_cr0

global flush_tlb
flush_tlb:
    push ebp
    mov ecx, cr3
    mov cr3, ecx
    pop ebp
    ret

global flush_page
flush_page:
    push ebp
    mov ecx, [esp-4]
    invlpg [ecx]
    pop ebp
    ret

global disable_paging
disable_paging: 
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

    mov ebx, cr0
    and ebx, ~CR_PG
    mov cr0, ebx

    mov ecx, 0
_copy:  
    mov ebx, [esi + ecx*4]
    mov [edi + ecx*4], ebx
    inc ecx

    cmp ecx, 1024
    jl _copy - 0xC0000000

    mov ebx, cr0
    or ebx, CR_PG | CR_WP | 1
    mov cr0, ebx

    mov ecx, cr4
    or ecx, PGE
    mov cr4, ecx

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

    mov ecx, cr4
    or ecx, PGE
    mov cr4, ecx

    pop ebx
    ret

global switch_page_dir_asm
switch_page_dir_asm:
    push ebp
    mov ebp,  esp

    push ebx
    pushfd

    cli

    ;; mov ecx, cr3
    mov ecx, [ebp + 8]
    mov cr3, ecx

    ;; mov ebx, cr0
    ;; mov ebx, CR_PG | CR_WP | 3
    ;; mov cr0, ebx

    popfd
    pop ebx
    pop ebp
    ret
    
