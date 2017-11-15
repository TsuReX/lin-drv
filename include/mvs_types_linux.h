#ifndef _MVS_TYPES_LINUX_H
#define _MVS_TYPES_LINUX_H

#ifndef _MVS_TYPES_H
#	error You should include "mvs_types.h", not this file.
#endif

#include <linux/types.h>

#define MVS_IOCTL_MAGIC			0xFE

#define MVS_IOCTL_READIO		_IOR(MVS_IOCTL_MAGIC,  1, unsigned long)
#define MVS_IOCTL_WRITEIO		_IOW(MVS_IOCTL_MAGIC,  2, unsigned long)
#define MVS_IOCTL_READMEM		_IOR(MVS_IOCTL_MAGIC,  3, unsigned long)
#define MVS_IOCTL_WRITEMEM		_IOW(MVS_IOCTL_MAGIC,  4, unsigned long)
#define MVS_IOCTL_READMEM_BLK		_IOR(MVS_IOCTL_MAGIC,  5, unsigned long)
#define MVS_IOCTL_WRITEMEM_BLK		_IOW(MVS_IOCTL_MAGIC,  6, unsigned long)
#define MVS_IOCTL_BENCH			_IOR(MVS_IOCTL_MAGIC,  7, unsigned long)
#define MVS_IOCTL_VERSION		_IOR(MVS_IOCTL_MAGIC,  8, unsigned long)
#define MVS_IOCTL_READBAR3		_IOR(MVS_IOCTL_MAGIC,  9, unsigned long)
#define MVS_IOCTL_WRITEBAR3		_IOR(MVS_IOCTL_MAGIC, 10, unsigned long)
#define MVS_IOCTL_GET_INFO		_IOR(MVS_IOCTL_MAGIC, 11, struct mvs_info)
#define MVS_IOCTL_READMEM_32X32		_IOR(MVS_IOCTL_MAGIC, 12, unsigned long)
#define MVS_IOCTL_WRITEMEM_32X32	_IOR(MVS_IOCTL_MAGIC, 13, unsigned long)

struct mvs_cmd_tiny {
	unsigned short  reg;
	unsigned int    val;
};

struct mvs_cmd {
	unsigned int   reg;
	unsigned long  val;
	unsigned int   bm_id;
	unsigned int   code;
	unsigned int   dmc_id;
	unsigned int   addr;
	unsigned int   step;
	unsigned char  cfg_dmc;
};

struct mvs_info {
	/** Полная версия. */
	uint64_t  changeset;
	/** Короткая версия. */
	unsigned long  revision;
};

#endif /* _MVS_TYPES_LINUX_H */

