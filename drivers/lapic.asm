CPUID_FEAT_APIC equ 0x200
CPUID_ENABLED   equ 0x200000
;; section .data

section .text

extern fb_print_hex
global apic_available
apic_available:
    
    mov eax, 1
    pushfd
    mov ecx, [esp] 
    xor dword [esp], CPUID_ENABLED
    popfd
    pushfd
    cmp ecx, [esp]
    je no_cpuid
    popfd

    cpuid
    
    ;; pop ecx
    shr edx, 9
    and edx, 0x1
    mov eax, edx
    ;; call fb_print_hex
    ;; call fb_print_hex
    ret

no_cpuid:
    popfd
    mov eax, -1
    ret
    
    
