extern fb_put_pixel

section .text
    global some

some:
    push eax
    push ebx
    
    xor eax, eax
    mov eax, 15
    shl eax, 0x10
    mov ebx, 3
    shl ebx, 24
    or eax, ebx
    or eax, 97
    push eax
    call fb_put_pixel
    pop eax
    
    pop ebx
    pop eax
    ret
    
