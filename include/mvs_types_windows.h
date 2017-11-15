#ifndef _MVS_TYPES_WINDOWS_H
#define _MVS_TYPES_WINDOWS_H

#ifndef _MVS_TYPES_H
#	error You should include "mvs_types.h", not this file.
#endif

#define BM_TYPE	(40000UL)

#define IOCTL_BM_GET_INFORMATION \
	CTL_CODE(BM_TYPE, 0x2054, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

/* Memory read */
#define IOCTL_BM_READ_BUFFERED \
	CTL_CODE(BM_TYPE, 0x2048, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_BM_READ_CYCLED \
	CTL_CODE(BM_TYPE, 0x2050, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

/* Memory write */
#define IOCTL_BM_WRITE_BUFFERED \
	CTL_CODE(BM_TYPE, 0x2049, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_BM_WRITE_CYCLED \
	CTL_CODE(BM_TYPE, 0x2051, METHOD_IN_DIRECT, FILE_ANY_ACCESS)

/* Ports */
#define IOCTL_BM_READ_PORT \
	CTL_CODE(BM_TYPE, 0x2052, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_BM_WRITE_PORT \
	CTL_CODE(BM_TYPE, 0x2053, METHOD_IN_DIRECT, FILE_ANY_ACCESS)

/* DMC read/write */
#define IOCTL_BM_READWRITE_DMC \
	CTL_CODE(BM_TYPE, 0x2055, METHOD_NEITHER, FILE_ANY_ACCESS)

/* BAR3 */
#define IOCTL_BM_READ_BAR3 \
	CTL_CODE(BM_TYPE, 0x2057, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_BM_WRITE_BAR3 \
	CTL_CODE(BM_TYPE, 0x2058, METHOD_IN_DIRECT, FILE_ANY_ACCESS)

struct mvs_rw_port {
	DWORD  reg_code;
	DWORD  length;
};

struct mvs_write_bar3 {
	DWORD  index;
	DWORD  data;
};

struct mvs_rw_dmc {
	DWORD  seg;
	DWORD  dmc_id;
	DWORD  addr;
	DWORD  step;
	DWORD  mode;
	DWORD  length;
	DWORD  is_reading;
};

struct mvs_driver_info {
	UINT       id;
	ULONGLONG  changeset;
	ULONG      revision;
};

#endif /* _MVS_TYPES_WINDOWS_H */

