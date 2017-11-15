#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>	/* printk(), ARRAY_SIZE(), IS_ALIGNED() */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/pci.h>
#include <linux/miscdevice.h>
#include <linux/pagemap.h>
#include <linux/sched.h>	/* struct mm_struct */
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/scatterlist.h>
#include <linux/err.h>
#include <linux/fs.h>

#include <asm/current.h>
#include <asm-generic/ioctl.h> /* _IOR */
#include <asm-generic/fcntl.h> /* F_GETLK */
#include <asm-generic/param.h> /* HZ */
#include <asm-generic/errno.h>
#include <linux/compiler.h>

#include "mvs_types.h"
#include "mvs_version.h"

#define DRIVER_NAME	"MVS"
#define DEVICE_NAME	"MVSDEV"
#define DRIVER_VERSION	"2.1.0, changeset: " __MODULE_STRING(MVS_MERCURIAL_LONG_CHANGESET) ", revision: " __MODULE_STRING(MVS_MERCURIAL_SHORT_CHANGESET)

#define SGDRV_RD_EXEC	(1U << 31)
#define SGDRV_WR_EXEC	(1U << 30)
#define SGDRV_CMD_FAIL	(1U << 29)
#define SGDRV_CMD_WRITE	(1U << 31)
#define SGDRV_CMD_READ	(0U)

#define DMABUFFSIZE	(64 * 1024)	/* 64K 64-bit pointers H/W table */
#define TIMEOUT		(5 * HZ)	/* jiffies */

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

static void prepare_dmc_32x32(const struct mvs_dev *mvsdev, uint32_t write, uint32_t code, uint32_t dmc_id, uint32_t addr, uint32_t step, uint32_t count)
{
	iowrite32(4, mvsdev->bar_addr[1] + 2);

	iowrite32((dmc_id << 24) | (write << 15) | (1 << 5) | code, mvsdev->bar_addr[1] + 3);

	iowrite32(addr, mvsdev->bar_addr[1] + 3);

	iowrite32(step, mvsdev->bar_addr[1] + 3);

	iowrite32(count, mvsdev->bar_addr[1] + 3);
}

static void prepare_dmc_common(const struct mvs_dev *mvsdev, uint32_t write, uint32_t code, uint32_t dmc_id, uint32_t addr, uint32_t step, uint32_t count)
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

static int prep(struct mvs_dev *mvsdev, unsigned int cmd, const struct mvs_cmd *usrcmd)
{
	uint32_t             is_write_cmd, cnt_int, cnt_rem;
	int                  retval, direction, index, pinned_pages, sg_buffers;
	struct scatterlist  *sg;
//	printk(KERN_EMERG "MVS: prep 1, mvsdev %p\n", mvsdev);
	if (unlikely(usrcmd->reg == 0))
		return 0;

	if (unlikely(!IS_ALIGNED(usrcmd->val, PAGE_SIZE)))
		return -EINVAL;
//	printk(KERN_EMERG "MVS: prep 2\n");
	/* usrcmd->reg is the number of 32-bit words. */
	cnt_rem = usrcmd->reg % WORDS32_PER_PAGE;
	cnt_int = usrcmd->reg / WORDS32_PER_PAGE + !!cnt_rem;
//	printk(KERN_EMERG "MVS: prep 3\n");
	DBG(mvsdev->pdev, "pages_int = %d, pages_rem = %d\n", cnt_int, cnt_rem);

	if (unlikely(cnt_int > ARRAY_SIZE(mvsdev->user_pages)))
		return -ERANGE;

	is_write_cmd = (cmd == MVS_IOCTL_WRITEMEM || cmd == MVS_IOCTL_WRITEMEM_32X32);

	/*
	 * Pin user pages in memory. VMAs will only remain valid while mmap_sem is held.
	 * Thus, get_user_pages() must be called with mmap_sem held for read or write. 
	 */
	down_read(&current->mm->mmap_sem);
//	printk(KERN_EMERG "MVS: prep 4\n");
	/* is_write_cmd == false means read from drive, write into memory area. */
	pinned_pages = get_user_pages(current, current->mm, usrcmd->val, cnt_int, !is_write_cmd, 0, mvsdev->user_pages, NULL);
	//pinned_pages = get_user_pages(usrcmd->val, cnt_int, !(is_write_cmd & 0x1), mvsdev->user_pages, NULL);
//	printk(KERN_EMERG "MVS: prep 5\n");
	up_read(&current->mm->mmap_sem);

	DBG(mvsdev->pdev, "get_user_pages() returned %d pinned pages\n", pinned_pages);

	if (unlikely(pinned_pages < 0))
		return pinned_pages;
//	printk(KERN_EMERG "MVS: prep 6\n");
	direction = is_write_cmd ? PCI_DMA_TODEVICE : PCI_DMA_FROMDEVICE;

	/* Initialize the SG table. */
//	printk(KERN_EMERG "MVS: prep 7\n");
	sg_init_table(mvsdev->sglist, pinned_pages);
//	printk(KERN_EMERG "MVS: prep 8\n");
	for (index = 0, sg = mvsdev->sglist; index < pinned_pages; ++index, sg = sg_next(sg))
		sg_set_page(sg, mvsdev->user_pages[index], PAGE_SIZE, 0);

	retval = -ENOMEM;
//	printk(KERN_EMERG "MVS: prep 9\n");
	/*
	 * Get the number of physical segments mapped (this may be shorter than
	 * <pinned_pages> passed in if some elements of the scatter/gather list
	 * are physically or virtually adjacent and an IOMMU maps them with a
	 * single entry).
	 */
	sg_buffers = pci_map_sg(mvsdev->pdev, mvsdev->sglist, pinned_pages, direction);
//	printk(KERN_EMERG "MVS: prep 10\n");
	DBG(mvsdev->pdev, "pci_map_sg() returned %d mapped SG buffers\n", sg_buffers);

	if (unlikely(sg_buffers == 0))
		goto out;
//	printk(KERN_EMERG "MVS: prep 11\n");
	for_each_sg(mvsdev->sglist, sg, sg_buffers, index)
		mvsdev->dma_kvirt_addr[index] = sg_dma_address(sg);
//	printk(KERN_EMERG "MVS: prep 12\n");
	if (is_write_cmd && usrcmd->cfg_dmc) {
		if (cmd == MVS_IOCTL_WRITEMEM_32X32)
			prepare_dmc_32x32(mvsdev, 1, usrcmd->code, usrcmd->dmc_id, usrcmd->addr, usrcmd->step, usrcmd->reg);
		else
			prepare_dmc_common(mvsdev, 1, usrcmd->code, usrcmd->dmc_id, usrcmd->addr, usrcmd->step, usrcmd->reg);
	}
//	printk(KERN_EMERG "MVS: prep 13\n");
	/* Reinitialize a completion structure. */
//	INIT_COMPLETION(mvsdev->completion);
	reinit_completion(&mvsdev->completion);
//	printk(KERN_EMERG "MVS: prep 14\n");
	/* Start DMA transfer. */
	iowrite32((is_write_cmd ? SGDRV_CMD_WRITE : SGDRV_CMD_READ) | (cnt_int * WORDS32_PER_PAGE) | cnt_rem, mvsdev->bar_addr[0] + 2);
//	printk(KERN_EMERG "MVS: prep 15\n");
	if (!is_write_cmd) {
		if (usrcmd->cfg_dmc) {
			if (cmd == MVS_IOCTL_READMEM_32X32)
				prepare_dmc_32x32(mvsdev, 0, usrcmd->code, usrcmd->dmc_id, usrcmd->addr, usrcmd->step, usrcmd->reg);
			else
				prepare_dmc_common(mvsdev, 0, usrcmd->code, usrcmd->dmc_id, usrcmd->addr, usrcmd->step, usrcmd->reg);
		}

		iowrite32(1, mvsdev->bar_addr[1] + 2);
	}
//	printk(KERN_EMERG "MVS: prep 16 , &mvsdev->completion %p\n", &mvsdev->completion);
	/* Wait for an interrupt. */
	retval = wait_for_completion_timeout(&mvsdev->completion, TIMEOUT) ? 0 : -ETIMEDOUT;
//	printk(KERN_EMERG "MVS: prep 17\n");
	DBG(mvsdev->pdev, "wait_for_completion_timeout() returned %d\n", retval);

	/*
	 * Unmap the previously mapped scatter/gather list. All the parameters must
	 * be the same as those and passed in to the scatter/gather mapping API.
	 */
	pci_unmap_sg(mvsdev->pdev, mvsdev->sglist, pinned_pages, direction);
//	printk(KERN_EMERG "MVS: prep 18\n");
out:
	for (index = 0; index < pinned_pages; ++index) {
		/*
		 * If the page is written to, set_page_dirty (or set_page_dirty_lock, as appropriate)
		 * must be called after the page is finished with, and before put_page is called.
		 */
		if (!is_write_cmd)
			set_page_dirty_lock(mvsdev->user_pages[index]);

		/* Free from page cache. */
		page_cache_release(mvsdev->user_pages[index]);
	}
//	printk(KERN_EMERG "MVS: prep 19\n");
	return retval;
}

static int mvs_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &dev;

	return 0;
}

static long mvs_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	unsigned int         i;
	uint32_t __user     *ptr;
	uint32_t             value;
	struct mvs_dev      *mvsdev;
	struct mvs_cmd       usrcmd;
	struct mvs_cmd_tiny  usrcmd_tiny;
	struct mvs_info      info;

	mvsdev = filp->private_data;

	switch (cmd) {
		case MVS_IOCTL_READIO:
			if (copy_from_user(&usrcmd, (struct mvs_cmd __user *)arg, sizeof(usrcmd)))
				return -EFAULT;

			for (i = 0, ptr = (uint32_t __user *)usrcmd.val; i < usrcmd.step; ++i, ++ptr) {
				value = ioread32(mvsdev->bar_addr[1] + usrcmd.reg);

				if (put_user(value, ptr))
					return -EFAULT;
			}
			return 0;

		case MVS_IOCTL_WRITEIO:
//			printk(KERN_EMERG "MVS: mvs_ioctl WRITEIO before copy_from_user\n");
			
			if (copy_from_user(&usrcmd, (struct mvs_cmd __user *)arg, sizeof(usrcmd)))
				return -EFAULT;
			
//			printk(KERN_EMERG "MVS: mvs_ioctl WRITEIO after copy_from_user\n");
			
			for (i = 0, ptr = (uint32_t __user *)usrcmd.val; i < usrcmd.step; ++i, ++ptr) {
				if (get_user(value, ptr))
					return -EFAULT;

				iowrite32(value, mvsdev->bar_addr[1] + usrcmd.reg);
			}

			return 0;

		case MVS_IOCTL_READMEM:
		case MVS_IOCTL_WRITEMEM:
		case MVS_IOCTL_READMEM_32X32:
		case MVS_IOCTL_WRITEMEM_32X32:
//			printk(KERN_EMERG "MVS: mvs_ioctl XXXXMEM before copy_from_user\n");

			if (copy_from_user(&usrcmd, (struct mvs_cmd __user *)arg, sizeof(usrcmd)))
				return -EFAULT;
//			printk(KERN_EMERG "MVS: mvs_ioctl XXXXMEM after copy_from_user\n");

//			printk(KERN_EMERG "MVS: mvs_ioctl XXXXMEM reg %u, val 0x%lX, bm_id %d, code %d, dmc_id %d, addr 0x%X, step %d, cfg_dmc 0x%X", 
//			usrcmd.reg, usrcmd.val, usrcmd.bm_id, usrcmd.code, usrcmd.dmc_id, usrcmd.addr, usrcmd.step, usrcmd.cfg_dmc);
			
			return prep(mvsdev, cmd, &usrcmd);

		case MVS_IOCTL_READBAR3:
			if (copy_from_user(&usrcmd_tiny, (struct mvs_cmd_tiny __user *)arg, sizeof(usrcmd_tiny)))
				return -EFAULT;

			usrcmd_tiny.val = ioread32(mvsdev->bar_addr[2] + usrcmd_tiny.reg);

			if (copy_to_user((struct mvs_cmd_tiny __user *)arg, &usrcmd_tiny, sizeof(usrcmd_tiny)))
				return -EFAULT;

			return 0;

		case MVS_IOCTL_WRITEBAR3:
			if (copy_from_user(&usrcmd_tiny, (struct mvs_cmd_tiny __user *)arg, sizeof(usrcmd_tiny)))
				return -EFAULT;

			iowrite16(usrcmd_tiny.val, (uint16_t __iomem *)mvsdev->bar_addr[2] + usrcmd_tiny.reg);

			return 0;

		case MVS_IOCTL_GET_INFO:
			info.changeset = MVS_MERCURIAL_LONG_CHANGESET;
			info.revision  = MVS_MERCURIAL_SHORT_CHANGESET;

			if (copy_to_user((struct mvs_version __user *)arg, &info, sizeof(info)))
				return -EFAULT;

			return 0;

		default:
			/* Inappropriate ioctl for device. */
			return -ENOTTY;
	}
}

static int do_getlk(struct file *filp, struct file_lock *fl)
{
	posix_test_lock(filp, fl);

	return 0;
}

static int do_unlck(struct file *filp, struct file_lock *fl)
{
	int              retval;
	struct mvs_dev  *mvsdev;

	/* Only whole-file unlocks are supported. */
	if (fl->fl_start != 0 || fl->fl_end != OFFSET_MAX)
		return -EINVAL;

	mvsdev = filp->private_data;

	/* We must determine whether or not a lock is already set. */
	fl->fl_flags |= FL_EXISTS;

//	retval = posix_lock_inode_wait(filp, fl);
	retval = locks_lock_inode_wait(file_inode(filp),fl);
	if (retval == 0)
		mutex_unlock(&mvsdev->mutex);

	return retval;
}

static int do_setlk(struct file *filp, struct file_lock *fl)
{
	int              retval;
	struct mvs_dev  *mvsdev;

	/* Only whole-file locks are supported. */
	if (fl->fl_start != 0 || fl->fl_end != OFFSET_MAX)
		return -EINVAL;

	mvsdev = filp->private_data;

	if (fl->fl_flags & FL_SLEEP)
		retval = mutex_lock_interruptible(&mvsdev->mutex);
	else
		retval = !mutex_trylock(&mvsdev->mutex) ? -EAGAIN : 0;

	if (retval == 0) {
//		retval = posix_lock_inode_wait(filp, fl);
		retval = locks_lock_inode_wait(file_inode(filp),fl);
		if (retval < 0)
			mutex_unlock(&mvsdev->mutex);
	}

	return retval;
}

static int mvs_lock(struct file *filp, int cmd, struct file_lock *fl)
{
	/* We don't support mandatory locks. */
	if (__mandatory_lock(filp->f_path.dentry->d_inode) && fl->fl_type != F_UNLCK)
		return -ENOLCK;

	if (IS_GETLK(cmd))
		return do_getlk(filp, fl);

	if (fl->fl_type == F_UNLCK)
		return do_unlck(filp, fl);

	return do_setlk(filp, fl);
}

static const struct file_operations  mvs_fops = {
	.owner		= THIS_MODULE,
	.open		= mvs_open,
	.unlocked_ioctl	= mvs_ioctl,
	.lock		= mvs_lock,
};

static int mvs_init_one(struct pci_dev *pdev, const struct pci_device_id *ent)
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

static void mvs_remove_one(struct pci_dev *pdev)
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

static const struct pci_device_id  mvs_pci_tbl[] = {
	{PCI_DEVICE(0x10ee, 0x0000)},
	{0}
};

static struct pci_driver  mvs_driver = {
	.name		= DRIVER_NAME,
	.id_table	= mvs_pci_tbl,
	.probe		= mvs_init_one,
	.remove		= mvs_remove_one,
};

static int __init mvs_init_module(void)
{
	printk("Init module\n");
	return pci_register_driver(&mvs_driver);
}

static void __exit mvs_cleanup_module(void)
{
	printk("Cleanup module\n");
	pci_unregister_driver(&mvs_driver);
}

module_init(mvs_init_module);
module_exit(mvs_cleanup_module);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION(DRIVER_VERSION);
MODULE_DEVICE_TABLE(pci, mvs_pci_tbl);

