#include <linux/export.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/stat.h>
#include <asm-generic/errno-base.h>

#include "mvs_fops.h"
#include "mvs_pci_ops.h"
#include "mvs_general.h"

struct mvs_dev dev;

static const struct file_operations  mvs_fops = {
	.owner		= THIS_MODULE,
	.open		= mvs_open,
	.unlocked_ioctl	= mvs_ioctl,
	.lock		= mvs_lock,
};

void prepare_dmc_32x32(const struct mvs_dev *mvsdev, uint32_t write, uint32_t code, uint32_t dmc_id, uint32_t addr, uint32_t step, uint32_t count)
{
	iowrite32(4, mvsdev->bar_addr[1] + 2);

	iowrite32((dmc_id << 24) | (write << 15) | (1 << 5) | code, mvsdev->bar_addr[1] + 3);

	iowrite32(addr, mvsdev->bar_addr[1] + 3);

	iowrite32(step, mvsdev->bar_addr[1] + 3);

	iowrite32(count, mvsdev->bar_addr[1] + 3);
}

void prepare_dmc_common(const struct mvs_dev *mvsdev, uint32_t write, uint32_t code, uint32_t dmc_id, uint32_t addr, uint32_t step, uint32_t count)
{
	iowrite32(4, mvsdev->bar_addr[1] + 2);

	iowrite32(dmc_id << 24, mvsdev->bar_addr[1] + 3);

	iowrite32(4, mvsdev->bar_addr[1] + 2);

	iowrite32(code | (write ? 0x8000 : 0), mvsdev->bar_addr[1] + 3);

	iowrite32(0, mvsdev->bar_addr[1] + 3);

	iowrite32(addr, mvsdev->bar_addr[1] + 3);

	iowrite32(0, mvsdev->bar_addr[1] + 3);

	iowrite32(step, mvsdev->bar_addr[1] + 3);

	iowrite32(0, mvsdev->bar_addr[1] + 3);

	iowrite32(count >> 1, mvsdev->bar_addr[1] + 3);
}

static irqreturn_t mvs_irqhandler(int irq, void *dev_id)
{
	uint32_t         stat;
	struct mvs_dev  *mvsdev;
//	printk(KERN_EMERG "MVS: mvs_irqhandler begin\n");
	mvsdev = (struct mvs_dev *)dev_id;

	stat = ioread32(mvsdev->bar_addr[0]);

	if (stat & (SGDRV_RD_EXEC | SGDRV_WR_EXEC | SGDRV_CMD_FAIL)) {
		complete(&mvsdev->completion);
//		printk(KERN_EMERG "MVS: mvs_irqhandler end HANDLED\n");
		return IRQ_HANDLED;
	}
//	printk(KERN_EMERG "MVS: mvs_irqhandler end NONE\n");
	return IRQ_NONE;
}

int mvs_init_one(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	int  i;

	INFO(pdev, "Found device PCI:%s, revision %u\n", pci_name(pdev), pdev->revision);

	if (pci_enable_device(pdev)) {
		ERROR(pdev, "pci_enable_device() failed\n");
		goto pci_enable_device_failed;
	}

	/* Enable bus-mastering for the device. */
	pci_set_master(pdev);

	pci_set_drvdata(pdev, &dev);

	dev.pdev = pdev;

	/* Mark all PCI regions associated with the PCI device as being reserved. */
	if (pci_request_regions(pdev, DRIVER_NAME) < 0) {
		ERROR(pdev, "pci_request_regions() failed\n");
		goto pci_request_regions_failed;
	}

	/* Configure DMA attributes. */
	if (pci_set_dma_mask(pdev, DMA_BIT_MASK(64))) {
		ERROR(pdev, "pci_set_dma_mask() failed\n");
		goto pci_set_dma_mask_failed;
	}

	if (pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(64))) {
		ERROR(pdev, "pci_set_consistent_dma_mask() failed\n");
		goto pci_set_consistent_dma_mask_failed;
	}

	INFO(pdev, "64-bit DMA and 64-bit consistent DMA are enabled\n");

	/* Remap BAR space. */
	for (i = 0; i < ARRAY_SIZE(dev.bar_addr); ++i) {
		DBG(pdev, "BAR%d: resource = 0x%lx, size = 0x%lx, flags = 0x%lx\n",
		    i,
		    (unsigned long)pci_resource_start(pdev, i),
		    (unsigned long)pci_resource_len(pdev, i),
		    pci_resource_flags(pdev, i)
		);

		dev.bar_addr[i] = pci_ioremap_bar(pdev, i);

		if (dev.bar_addr[i] == NULL) {
			ERROR(pdev, "BAR%d pci_ioremap_bar() failed\n", i);
			goto pci_ioremap_bar_failed;
		}
	}

	/* Allocate and map consistent DMA region. */
	dev.dma_kvirt_addr = pci_alloc_consistent(pdev, DMABUFFSIZE * sizeof(*dev.dma_kvirt_addr), &dev.dma_bus_addr);

	INFO(pdev, "DMA kernel virtual address = 0x%p\n", dev.dma_kvirt_addr);
	INFO(pdev, "DMA bus address = 0x%lx\n", (unsigned long)dev.dma_bus_addr);

	if (dev.dma_kvirt_addr == NULL) {
		ERROR(pdev, "pci_alloc_consistent() failed\n");
		goto pci_alloc_consistent_failed;
	}

	init_completion(&dev.completion);

	/* Allocate an interrupt line for the managed device. */
	if (request_irq(pdev->irq, mvs_irqhandler, IRQF_SHARED, DRIVER_NAME, &dev)) {
		ERROR(pdev, "request_irq() failed\n");
		goto request_irq_failed;
	}

	mutex_init(&dev.mutex);

	/* Register a miscellaneous device. */
	dev.misc.minor = MISC_DYNAMIC_MINOR;
	dev.misc.name  = DEVICE_NAME;
	dev.misc.fops  = &mvs_fops;
	dev.misc.mode  = S_IRUGO | S_IWUGO;

	if (misc_register(&dev.misc) < 0) {
		ERROR(pdev, "misc_register() failed\n");
		goto misc_register_failed;
	}

	/* Write table address. */
	iowrite32((uint32_t)dev.dma_bus_addr, dev.bar_addr[0] + 0);
	iowrite32((uint64_t)dev.dma_bus_addr >> 32, dev.bar_addr[0] + 1);

//	unsigned int status = ioread32(dev.bar_addr[0]);
//	printk(KERN_EMERG "MVS: mvs_init_one STATUS 0x%X\n", status);

	return 0;

misc_register_failed:
	mutex_destroy(&dev.mutex);

	free_irq(pdev->irq, &dev);

request_irq_failed:
	pci_free_consistent(pdev, DMABUFFSIZE * sizeof(*dev.dma_kvirt_addr), dev.dma_kvirt_addr, dev.dma_bus_addr);

pci_alloc_consistent_failed:
pci_ioremap_bar_failed:
	for (i = 0; i < ARRAY_SIZE(dev.bar_addr); ++i) {
		if (dev.bar_addr[i] != NULL)
			pci_iounmap(pdev, dev.bar_addr[i]);
	}

pci_set_consistent_dma_mask_failed:
pci_set_dma_mask_failed:
	pci_release_regions(pdev);

pci_request_regions_failed:
	pci_set_drvdata(pdev, NULL);

	pci_disable_device(pdev);

pci_enable_device_failed:
	return -EBUSY;
}

void mvs_remove_one(struct pci_dev *pdev)
{
	int              i;
	struct mvs_dev  *mvsdev;

	mvsdev = pci_get_drvdata(pdev);

	free_irq(pdev->irq, mvsdev);

	pci_free_consistent(pdev, DMABUFFSIZE * sizeof(*mvsdev->dma_kvirt_addr), mvsdev->dma_kvirt_addr, mvsdev->dma_bus_addr);

	misc_deregister(&mvsdev->misc);

	mutex_destroy(&mvsdev->mutex);

	for (i = 0; i < ARRAY_SIZE(mvsdev->bar_addr); ++i)
		pci_iounmap(pdev, mvsdev->bar_addr[i]);

	pci_set_drvdata(pdev, NULL);

	pci_disable_device(pdev);

	/*
	 * Drivers should call pci_release_region() AFTER calling pci_disable_device().
	 * The idea is to prevent two devices colliding on the same address range.
	 */
	pci_release_regions(pdev);
}


