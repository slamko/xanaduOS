global enable_paging

enable_paging:
    mov cr3, eax

    mov ebx, cr0
    mov ebx, 0x80000000
    mov cr0, ebx
    ret
