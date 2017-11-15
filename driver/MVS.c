#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>	/* printk(), ARRAY_SIZE(), IS_ALIGNED() */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/pci.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>	/* struct mm_struct */
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/scatterlist.h>
#include <linux/err.h>

#include <asm/current.h>
#include <asm-generic/ioctl.h> /* _IOR */
#include <asm-generic/fcntl.h> /* F_GETLK */
#include <asm-generic/param.h> /* HZ */
#include <asm-generic/errno.h>
#include <linux/compiler.h>

#include "mvs_types.h"
#include "mvs_version.h"
#include "mvs_fops.h"
#include "mvs_general.h"
#include "mvs_pci_ops.h"

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

//	return 0;
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

