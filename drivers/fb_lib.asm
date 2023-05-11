global outfb

outfb:
    mov al, 0xFF
    mov dx, 0xFF
    out dx, al
    ret
