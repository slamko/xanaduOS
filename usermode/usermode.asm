SYSENTER_CS  equ 0x174
SYSENTER_ESP equ 0x175
SYSENTER_EIP equ 0x176
EFLAGS_ID    equ 0x200000

section .data
    syscall_msg db "In kernel", 10, 0
    usermsg db "User mode msg", 10, 0

section .text

global usr_sysenter
extern sysenter
extern sys_write
   
extern SYSCALL_MAX_ARGS_NUM
extern syscall_handler

usr_sysenter:
    push ebx
    push esp
    push ebp
    push esi
    push edi

    mov ecx, esp
    mov edx, _after
    sysenter

    jmp _after

_legacy:
    ;; int 0x80
    ret
    
_after:
    pop edi
    pop esi
    pop ebp
    pop esp
    pop ebx
    ret

