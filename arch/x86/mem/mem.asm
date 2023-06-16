section .text
global enable_paging
global load_page_dir

CR_PG equ 0x80000000

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
    mov eax, 1
    shl eax, 31
    or ebx, eax
    mov cr0, ebx
    ret
