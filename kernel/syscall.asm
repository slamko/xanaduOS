
section .bss
    _sysenter_avl resb 1

section .text

extern kernel_int_stack_end

usermode_bootstrap:
    mov eax, cs
    and eax, 0x3
    jz loop

loop:
    jmp loop

SYSENTER_CS  equ 0x174
SYSENTER_ESP equ 0x175
SYSENTER_EIP equ 0x176
EFLAGS_ID    equ 0x200000

extern syscall_exec

get_eip:
    mov eax, [esp]
    ret

scall_wrapper:
    push ecx
    push edx

    lea ebp, [ecx + 5*4 + 4]
    mov edi, [ebp + 4]              ; store number of arguments

_push_args:
    push dword [ebp + edi*4 + 4]
    dec edi
    jnz _push_args              ; push variadic args

    push dword [ebp]              ; syscall number
    call syscall_exec
    add esp, 4
    push eax

    mov eax, 4
    mul dword [ebp + 4]         ; number of args
    add esp, eax

    pop eax
    pop edx
    pop ecx
    sysexit

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
    mov eax, scall_wrapper
    wrmsr   

    ret

_legacy_setup:
    ret

global jump_usermode
extern usermode
extern fb_print_black    
extern fb_print_num    
extern fb_print_hex    
extern proc_esp

jump_usermode:
    mov ebp, esp
    
    cli
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push dword 0x23
    push dword [ebp + 8]
    ;; call fb_print_hex
    pushfd
    push 0x1B
    sti
    push dword [ebp + 4]
    iret

global ltr
ltr:
    mov ax, 0x28 | 3
    ltr ax
    ret
