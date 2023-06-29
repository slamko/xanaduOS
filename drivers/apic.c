#include "drivers/fb.h"
#include "lib/kernel.h"

__attribute__((cdecl)) int apic_available(void);

void apic_detect(void) {
    unsigned int apic = 0;
    asm volatile("push %eax;"
                 "push %edx");

    asm volatile("mov $1, %eax");
    asm volatile("cpuid");
    asm volatile("mov %%edx, %0" : "=r"(apic));

    asm volatile("pop %edx;"
                 "pop %eax");
    apic = (apic >> 9) & 0x1;
    
    if (apic) {
        klog("APIC available\n");
    }
 
}

void apic_init(void) {
    apic_detect();
}
