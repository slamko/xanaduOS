#include "drivers/fb.h"
#include "lib/kernel.h"

__attribute__((cdecl)) int apic_available(void);

void apic_detect(void) {
    int apic = 0;

    asm_call(
        apic = apic_available();
    );
    
    if (apic) {
        klog("APIC available\n");
    }
}

void apic_init(void) {
    apic_detect();
}
