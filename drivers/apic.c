#include "lib/kernel.h"

int apic_available(void);

void apic_init(void) {
    int apic = apic_available();
    klog("Apic available: %d\n", apic);
}
