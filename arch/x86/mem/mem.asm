section .text
global enable_paging
global load_page_dir

load_page_dir:
    mov eax, [esp + 4]
    mov cr3, eax

enable_paging:
    mov ebx, cr4        ; read current cr4
    or  ebx, 0x0010 ; set PSE
    mov cr4, ebx        ; update cr4

    mov ebx, cr0
    mov ebx, 0x80000000
    mov cr0, ebx
    ret
