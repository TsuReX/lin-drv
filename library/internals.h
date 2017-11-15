#ifndef _INTERNALS_H
#define _INTERNALS_H

#include "HardwareInterface.h"

#include "mvs_version.h"
#include "mvs_types.h"

/* Platform-specific conditional compilation. */
#ifndef WIN32
#	define HI_LOCAL	__attribute__((visibility("hidden")))
#else
#	define HI_LOCAL
#endif

/* Compiler-specific definitions. */
#ifdef _MSC_VER
#	define __inline__	__inline
#endif

#ifndef WIN32
#	define HANDLE	int
#	define SIZE_T	size_t
#endif

/** Структура дескриптора интерфейса. */
struct UIHandle {
	/** A handle to the device. */
	HANDLE  fd;

	/** A handle to a file mapping object. */
	HANDLE  file_mapping;

	/** A starting address of a mapped view. */
	unsigned int  *shared_buf;

	/** Size of a page in bytes. */
	SIZE_T  page_size;
};

HI_LOCAL int arch_get_driver_version(struct UIHandle *ui_handle, hi_component_mercurial_version_t *mercurial_version);

HI_LOCAL int arch_read_port(struct UIHandle *ui_handle, unsigned long reg_code, unsigned long length, uint32_t *buffer);
HI_LOCAL int arch_write_port(struct UIHandle *ui_handle, unsigned long reg_code, unsigned long length, const uint32_t *buffer);

HI_LOCAL int arch_write_mem(struct UIHandle *ui_handle, unsigned long length, const uint32_t *buffer);
HI_LOCAL int arch_read_mem(struct UIHandle *ui_handle, unsigned long length, uint32_t *buffer);

HI_LOCAL int arch_read_bar3(struct UIHandle *ui_handle, unsigned long index, uint16_t *value);
HI_LOCAL int arch_write_bar3(struct UIHandle *ui_handle, unsigned long index, uint16_t value);

HI_LOCAL int arch_read_mem_fast(struct UIHandle *ui_handle, unsigned long code, unsigned long dmc_id, unsigned long addr, unsigned long step, unsigned long length, unsigned int *buffer);
HI_LOCAL int arch_write_mem_fast(struct UIHandle *ui_handle, unsigned long code, unsigned long dmc_id, unsigned long addr, unsigned long step, unsigned long length, const unsigned int *buffer);

HI_LOCAL int arch_read_mem64_fast(struct UIHandle *ui_handle, unsigned long code, unsigned long dmc_id, unsigned long addr, unsigned long step, unsigned long length, uint64_t *buffer);
HI_LOCAL int arch_write_mem64_fast(struct UIHandle *ui_handle, unsigned long code, unsigned long dmc_id, unsigned long addr, unsigned long step, unsigned long length, const uint64_t *buffer);

HI_LOCAL void arch_msleep(int msecs);
HI_LOCAL unsigned long arch_get_msecs(void);

HI_LOCAL int arch_spec_buff_alloc(struct UIHandle *ui_handle, SIZE_T size, void **pbuffer);
HI_LOCAL int arch_spec_buff_free(struct UIHandle *ui_handle, SIZE_T size, void *buffer);

HI_LOCAL int arch_open_interface(struct UIHandle *ui_handle);
HI_LOCAL int arch_close_interface(struct UIHandle *ui_handle);

#endif /* _INTERNALS_H */

