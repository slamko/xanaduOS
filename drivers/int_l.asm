extern isr_x86
    
%macro isr_no_error_code 1
global isr_%1
isr_%1:
    push dword 0
    push dword %1
    jmp common_isr    
%endmacro

%macro isr_error_code 1
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

isr_no_error_code 0
isr_no_error_code 1
isr_no_error_code 2
isr_no_error_code 3
isr_no_error_code 4
isr_no_error_code 5
isr_no_error_code 6
isr_no_error_code 7
isr_error_code    8
isr_no_error_code 9
isr_error_code    10
isr_error_code    11
isr_error_code    12
isr_error_code    13
isr_error_code    14
isr_no_error_code 15
isr_no_error_code 16
isr_error_code    17
isr_no_error_code 18
isr_no_error_code 19
isr_no_error_code 20
isr_no_error_code 21
isr_no_error_code 22
isr_no_error_code 23
isr_no_error_code 24
isr_no_error_code 25
isr_no_error_code 26
isr_no_error_code 27
isr_no_error_code 28
isr_no_error_code 29
isr_error_code    30
isr_no_error_code 31
    

global isr_table
isr_table:
%assign i 0 
%rep 32 
    dd isr_%+i ; use DQ instead if targeting 64-bit
%assign i i+1 
%endrep
