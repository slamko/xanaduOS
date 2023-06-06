VM_MASK equ 0x20000
    
section .text
    global enter_v86

enter_v86:
    pushfd
    or dword [esp], VM_MASK
    popfd
    
    
    
