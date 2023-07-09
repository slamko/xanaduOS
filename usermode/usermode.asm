SYSENTER_CS  equ 0x174
SYSENTER_ESP equ 0x175
SYSENTER_EIP equ 0x176
EFLAGS_ID    equ 0x200000

section .data
    syscall_msg db "In kernel", 10, 0
    usermsg db "User mode msg", 10, 0

section .text

global usr_sysenter
global usr_intx80

extern sysenter
extern sys_write
   
extern SYSCALL_MAX_ARGS_NUM
extern syscall_handler

extern main
extern userloop
   
_after:

    ;; add esp, 4
    ;; jmp userloop
    pop edx
    pop ecx
    pop edi
    pop esi
    pop ebp
    pop esp
    pop ebx
    ret

usr_intx80:
    push ebx
    push esp
    push ebp
    push esi
    push edi
    push ecx
    push edx

    ;; push dword 42
    mov ecx, esp
    mov edx, _after 
    int 0x80

    jmp _after

usr_sysenter:
    push ebx
    push esp
    push ebp
    push esi
    push edi
    push ecx
    push edx

    mov ecx, esp
    mov edx, _after 
    sysenter

    jmp _after

