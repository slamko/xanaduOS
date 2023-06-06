section .data
    syscall_msg db "In kernel", 10, 0
    legacy_msg db "Using legacy int", 10, 0
    
section .text
extern fb_print_num
usermode_bootstrap:
    mov eax, cs
    push eax

    and eax, 0x3
    cmp eax, 0x3
    jnz loop
    call usermode
    
loop:
    jmp loop
    

SYSENTER_CS  equ 0x174
SYSENTER_ESP equ 0x175
SYSENTER_EIP equ 0x176
EFLAGS_ID    equ 0x200000


scall_handler:
    push syscall_msg
    call fb_print_black
    sysexit

global jump_usermode
extern usermode
extern fb_print_black    
extern fb_print_num    
jump_usermode:
    ;; pushfd
    ;; or dword [esp], EFLAGS_ID
    ;; mov eax, [esp]
    ;; shr eax, 21
    ;; push eax
    ;; call fb_print_num
    ;; mov eax, [esp]
    mov ecx, SYSENTER_CS
    mov eax, 0x8
    wrmsr

    mov ecx, SYSENTER_ESP
    mov eax, kernel_int_stack_end
    wrmsr

    mov ecx, SYSENTER_EIP
    mov eax, scall_handler
    wrmsr   

    cli
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov eax, esp
    push dword 0x23
    push eax
    pushfd
    push 0x1B
    sti
    push usermode_bootstrap
    iret
extern kernel_int_stack_end
extern syscall_handler
global syscall
syscall:
real:   
    mov eax, cs
    cmp eax, 0x18 | 3
    jz gcall
    push eax
    call fb_print_num

gcall:  
    pushfd
    or dword [esp], EFLAGS_ID
    popfd
    pushfd
    mov eax, [esp]
    shr eax, 21
    and eax, 0x1
    jz legacy
    popfd

    mov eax, 1
    cpuid
    shr edx, 11
    and edx, 1
    jz legacy

enter:  
    mov ecx, esp
    mov edx, after
    sysenter

    jmp after

legacy:
    ;; push legacy_msg
    ;; call fb_print_black
    ;; int 0x80
    
after:
    jmp after


global ltr
ltr:
    mov ax, 0x28 | 3
    ltr ax
    ret
