section .text
global enable_paging
global load_page_dir

%include "mem/mem.inc"
    
extern fb_print_num
global print_cr0

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
enable_paging:
    mov ebx, cr0
    or ebx, CR_PG | CR_WP
    mov cr0, ebx
    ret
