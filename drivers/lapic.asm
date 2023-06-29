CPUID_FEAT_APIC equ 0x200
CPUID_ENABLED   equ 0x200000
;; section .data

section .text

extern fb_print_hex
global apic_available
apic_available:
    mov eax, 1
    pushfd
    mov ecx, [esp + 4]
    xor dword [esp + 4], CPUID_ENABLED
    popfd
    pushfd
    cmp ecx, [esp+4]
    je no_cpuid

    cpuid
    ;; pop ecx
    shr edx, 9
    and edx, 0x1
    mov eax, edx
    push eax
    call fb_print_hex
    pop eax
    ret

no_cpuid:
    mov eax, -1
    ret
    
    
