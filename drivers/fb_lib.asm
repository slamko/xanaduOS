global outfb

outfb:
    mov al, [esp + 8]
    mov dx, [esp + 4]
    out dx, al
    ret
