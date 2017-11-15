#include "internals.h"

#ifdef __linux__

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>

/** Имя специфического файла устройства. */
#define DEVICE_NAME	"/dev/MVSDEV"

/** Имя объекта разделяемой памяти в формате /somename. */
#define IPC_NAME	"/mvs-shm"

int arch_get_driver_version(struct UIHandle *ui_handle, hi_component_mercurial_version_t *mercurial_version)
{
	struct mvs_info  info;

	if (ioctl(ui_handle->fd, MVS_IOCTL_GET_INFO, &info) < 0)
		return brDriverError;

	mercurial_version->long_changeset  = info.changeset;
	mercurial_version->short_changeset = info.revision;

	return brSuccess;
}

int arch_read_port(struct UIHandle *ui_handle, unsigned long reg_code, unsigned long length, uint32_t *buffer)
{
	struct mvs_cmd  data;

	data.reg  = reg_code;
	data.val  = (unsigned long)buffer;
	data.step = length;

	if (ioctl(ui_handle->fd, MVS_IOCTL_READIO, &data) < 0)
		return brDriverError;

	return brSuccess;
}

int arch_write_port(struct UIHandle *ui_handle, unsigned long reg_code, unsigned long length, const uint32_t *buffer)
{
	struct mvs_cmd  data;

	data.reg  = reg_code;
	data.val  = (unsigned long)buffer;
	data.step = length;

	if (ioctl(ui_handle->fd, MVS_IOCTL_WRITEIO, &data) < 0)
		return brDriverError;

	return brSuccess;
}

int arch_write_mem(struct UIHandle *ui_handle, unsigned long length, const uint32_t *buffer)
{
	struct mvs_cmd  data;

	data.val     = (unsigned long)buffer;
	data.reg     = length;
	data.cfg_dmc = 0;

	if (ioctl(ui_handle->fd, MVS_IOCTL_WRITEMEM, &data) < 0)
		return brDriverError;

	return brSuccess;
}

int arch_read_mem(struct UIHandle *ui_handle, unsigned long length, uint32_t *buffer)
{
	struct mvs_cmd  data;

	data.val     = (unsigned long)buffer;
	data.reg     = length;
	data.cfg_dmc = 0;

	if (ioctl(ui_handle->fd, MVS_IOCTL_READMEM, &data) < 0)
		return brDriverError;

	return brSuccess;
}

int arch_read_bar3(struct UIHandle *ui_handle, unsigned long index, uint16_t *value)
{
	struct mvs_cmd_tiny  data;

	data.reg = index / 2;

	if (ioctl(ui_handle->fd, MVS_IOCTL_READBAR3, &data) < 0)
		return brDriverError;

	*value = (index & 1) ? (data.val >> 16) : data.val;

	return brSuccess;
}

int arch_write_bar3(struct UIHandle *ui_handle, unsigned long index, uint16_t value)
{
	struct mvs_cmd_tiny  data;

	data.reg = index;
	data.val = value;

	if (ioctl(ui_handle->fd, MVS_IOCTL_WRITEBAR3, &data) < 0)
		return brDriverError;

	return brSuccess;
}

int arch_read_mem_fast(struct UIHandle *ui_handle, unsigned long code, unsigned long dmc_id, unsigned long addr, unsigned long step, unsigned long length, unsigned int *buffer)
{
	struct mvs_cmd  data;

	data.val     = (unsigned long)buffer;
	data.reg     = length;
	data.code    = code;
	data.dmc_id  = dmc_id;
	data.addr    = addr;
	data.step    = step;
	data.cfg_dmc = 1;

	if (ioctl(ui_handle->fd, MVS_IOCTL_READMEM_32X32, &data) < 0)
		return brDriverError;

	return brSuccess;
}

int arch_write_mem_fast(struct UIHandle *ui_handle, unsigned long code, unsigned long dmc_id, unsigned long addr, unsigned long step, unsigned long length, const unsigned int *buffer)
{
	struct mvs_cmd  data;

	data.val     = (unsigned long)buffer;
	data.reg     = length;
	data.code    = code;
	data.dmc_id  = dmc_id;
	data.addr    = addr;
	data.step    = step;
	data.cfg_dmc = 1;

	if (ioctl(ui_handle->fd, MVS_IOCTL_WRITEMEM_32X32, &data) < 0)
		return brDriverError;

	return brSuccess;
}

int arch_read_mem64_fast(struct UIHandle *ui_handle, unsigned long code, unsigned long dmc_id, unsigned long addr, unsigned long step, unsigned long length, uint64_t *buffer)
{
	struct mvs_cmd  data;

	data.val     = (unsigned long)buffer;
	data.reg     = length;
	data.code    = code;
	data.dmc_id  = dmc_id;
	data.addr    = addr;
	data.step    = step;
	data.cfg_dmc = 1;

	if (ioctl(ui_handle->fd, MVS_IOCTL_READMEM, &data) < 0)
		return brDriverError;

	return brSuccess;
}

int arch_write_mem64_fast(struct UIHandle *ui_handle, unsigned long code, unsigned long dmc_id, unsigned long addr, unsigned long step, unsigned long length, const uint64_t *buffer)
{
	struct mvs_cmd  data;

	data.val     = (unsigned long)buffer;
	data.reg     = length;
	data.code    = code;
	data.dmc_id  = dmc_id;
	data.addr    = addr;
	data.step    = step;
	data.cfg_dmc = 1;

	if (ioctl(ui_handle->fd, MVS_IOCTL_WRITEMEM, &data) < 0)
		return brDriverError;

	return brSuccess;
}

void arch_msleep(int msecs)
{
	struct timespec  ts;

	ts.tv_sec  = (msecs / 1000);
	ts.tv_nsec = (msecs % 1000) * 1000000;

	nanosleep(&ts, NULL);
}

unsigned long arch_get_msecs(void)
{
	struct timespec  ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

int arch_spec_buff_alloc(struct UIHandle *ui_handle, SIZE_T size, void **pbuffer)
{
	*pbuffer = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (*pbuffer == MAP_FAILED)
		return brMemAllocError;

	return brSuccess;
}

int arch_spec_buff_free(struct UIHandle *ui_handle, SIZE_T size, void *buffer)
{
	if (munmap(buffer, size) < 0)
		return brMemAllocError;

	return brSuccess;
}

int arch_open_interface(struct UIHandle *ui_handle)
{
	/* Open a device. */
	ui_handle->fd = open(DEVICE_NAME, O_RDWR);

	if (ui_handle->fd < 0)
		return brDriverError;

	/* Create or open POSIX shared memory object. */
	ui_handle->file_mapping = shm_open(IPC_NAME, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

	if (ui_handle->file_mapping < 0)
		goto shm_open_failed;

	fchmod(ui_handle->file_mapping, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

	/* Set the size of the preallocated shared memory area. */
	if (ftruncate(ui_handle->file_mapping, sizeof(*ui_handle->shared_buf)) < 0)
		goto ftruncate_failed;

	/* Map pages of memory. */
	ui_handle->shared_buf = mmap(NULL, sizeof(*ui_handle->shared_buf), PROT_READ | PROT_WRITE, MAP_SHARED, ui_handle->file_mapping, 0);

	/* The file descriptor may be closed without affecting the memory mapping. */
	close(ui_handle->file_mapping);

	if (ui_handle->shared_buf == MAP_FAILED)
		goto mmap_failed;

	/* A fixed-length block for memory allocation and file mapping performed by mmap(). */
	ui_handle->page_size = sysconf(_SC_PAGESIZE);

	return brSuccess;

ftruncate_failed:
	close(ui_handle->file_mapping);

mmap_failed:
	/*shm_unlink(IPC_NAME);*/

shm_open_failed:
	close(ui_handle->fd);

	return brFailed;
}

int arch_close_interface(struct UIHandle *ui_handle)
{
	munmap(ui_handle->shared_buf, sizeof(*ui_handle->shared_buf));

	/*shm_unlink(IPC_NAME);*/

	close(ui_handle->fd);

	return brSuccess;
}

#endif /* __linux__ */

