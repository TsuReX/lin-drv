#ifndef DRIVER_MVS_PCI_OPS_H_
#define DRIVER_MVS_PCI_OPS_H_

#include "mvs_general.h"

int mvs_init_one(struct pci_dev *pdev, const struct pci_device_id *ent);
void mvs_remove_one(struct pci_dev *pdev);
void prepare_dmc_32x32(const struct mvs_dev *mvsdev, uint32_t write, uint32_t code, uint32_t dmc_id, uint32_t addr, uint32_t step, uint32_t count);
void prepare_dmc_common(const struct mvs_dev *mvsdev, uint32_t write, uint32_t code, uint32_t dmc_id, uint32_t addr, uint32_t step, uint32_t count);

#endif /* DRIVER_MVS_PCI_OPS_H_ */
