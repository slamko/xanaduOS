extern isr_x86
    
%macro no_error_code_isr 1
global isr_%1
isr_%1:
    push dword 0
    push dword %1
    jmp common_isr    
%endmacro

%macro error_code_isr 1
global isr_%1
isr_%1:
    push dword %1
    jmp common_isr
%endmacro

common_isr:
    push eax
    push ebx
    push ecx
    push edx
    push esi
    push edi
    push ebp

    call isr_x86

    pop ebp
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    pop eax

    add esp, 8

    iret
    

no_error_code_isr 1
no_error_code_isr 2
    
