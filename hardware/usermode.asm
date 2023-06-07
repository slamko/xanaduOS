section .data
    syscall_msg db "In kernel", 10, 0
    legacy_msg db "Using legacy int", 10, 0

section .bss
    _sysenter_avl resb 1
    
section .text
extern fb_print_num
usermode_bootstrap:
    mov eax, cs
    and eax, 0x3
    jz loop
    call usermode
    
loop:
    jmp loop
    

SYSENTER_CS  equ 0x174
SYSENTER_ESP equ 0x175
SYSENTER_EIP equ 0x176
EFLAGS_ID    equ 0x200000

global syscall_setup
syscall_setup:
    mov byte [_sysenter_avl], 0

    pushfd
    or dword [esp], EFLAGS_ID
    popfd

    pushfd
    mov eax, [esp]
    shr eax, 21
    and eax, 0x1
    jz _legacy_setup
    popfd

    mov eax, 1
    cpuid
    shr edx, 11
    and edx, 1
    jz _legacy_setup

_sysenter_setup:
    mov byte [_sysenter_avl], 1
    mov ecx, SYSENTER_CS
    mov eax, 0x8
    wrmsr

    mov ecx, SYSENTER_ESP
    mov eax, kernel_int_stack_end
    wrmsr

    mov ecx, SYSENTER_EIP
    mov eax, syscall_handler
    wrmsr   

    ret

_legacy_setup:
    ret

global jump_usermode
extern usermode
extern fb_print_black    
extern fb_print_num    

jump_usermode:
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
    test byte [_sysenter_avl], 0
    jnz _legacy

    mov ecx, esp
    mov edx, _after
    sysenter

    jmp _after

_legacy:
    ;; push legacy_msg
    ;; call fb_print_black
    int 0x80
    
_after:
    ret

global ltr
ltr:
    mov ax, 0x28 | 3
    ltr ax
    ret
