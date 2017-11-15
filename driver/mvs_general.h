#ifndef DRIVER_MVS_GENERAL_H_
#define DRIVER_MVS_GENERAL_H_

#include <linux/types.h>
#include <asm-generic/param.h>

#define DRIVER_NAME	"MVS"
#define DEVICE_NAME	"MVSDEV"
#define DRIVER_VERSION	"2.1.0, changeset: " __MODULE_STRING(MVS_MERCURIAL_LONG_CHANGESET) ", revision: " __MODULE_STRING(MVS_MERCURIAL_SHORT_CHANGESET)

#define SGDRV_RD_EXEC	(1U << 31)
#define SGDRV_WR_EXEC	(1U << 30)
#define SGDRV_CMD_FAIL	(1U << 29)
#define SGDRV_CMD_WRITE	(1U << 31)
#define SGDRV_CMD_READ	(0U)

#define TIMEOUT		(5 * HZ)	/* jiffies */
#define DMABUFFSIZE	(64 * 1024)	/* 64K 64-bit pointers H/W table */

/* The number of 32-bit words per page. */
#define WORDS32_PER_PAGE	(PAGE_SIZE / sizeof(uint32_t))

#define DBG(d, fmt, args...)	dev_dbg(&(d)->dev, fmt, ## args)
#define ERROR(d, fmt, args...)	dev_err(&(d)->dev, fmt, ## args)
#define INFO(d, fmt, args...)	dev_info(&(d)->dev, fmt, ## args)



static struct mvs_dev {
	struct pci_dev      *pdev;
	struct miscdevice    misc;
	uint32_t __iomem    *bar_addr[3];
	uint64_t            *dma_kvirt_addr;
	dma_addr_t           dma_bus_addr;
	struct mutex         mutex;
	struct completion    completion;
	struct page         *user_pages[DMABUFFSIZE];
	struct scatterlist   sglist[DMABUFFSIZE];
} dev;

#endif /* DRIVER_MVS_GENERAL_H_ */
