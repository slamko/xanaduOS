#ifndef RTL8139_H
#define RTL8139_H

#include <stdint.h>
#include "ipc/pci.h"

int rtl8139_init(struct pci_dev_data *data);

#endif
