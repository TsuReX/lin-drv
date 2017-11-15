#include "internals.h"

#ifdef WIN32

#define DMC_MODE_COMMON	(0)
#define DMC_MODE_32X32	(1)

int arch_get_driver_version(struct UIHandle *ui_handle, hi_component_mercurial_version_t *mercurial_version)
{
	DWORD                   nbytes;
	struct mvs_driver_info  info;

	if (!DeviceIoControl(ui_handle->fd, IOCTL_BM_GET_INFORMATION, NULL, 0, &info, sizeof(info), &nbytes, NULL))
		return brDriverError;

	mercurial_version->long_changeset  = info.changeset;
	mercurial_version->short_changeset = info.revision;

	return brSuccess;
}

int arch_read_port(struct UIHandle *ui_handle, unsigned long reg_code, unsigned long length, uint32_t *buffer)
{
	DWORD               nbytes;
	struct mvs_rw_port  read_port;

	read_port.reg_code = reg_code;
	read_port.length   = length;

	if (!DeviceIoControl(ui_handle->fd, IOCTL_BM_READ_PORT, &read_port, sizeof(read_port), buffer, length * sizeof(*buffer), &nbytes, NULL))
		return brDriverError;

	return brSuccess;
}

int arch_write_port(struct UIHandle *ui_handle, unsigned long reg_code, unsigned long length, const uint32_t *buffer)
{
	DWORD               nbytes;
	struct mvs_rw_port  write_port;

	write_port.reg_code = reg_code;
	write_port.length   = length;

	if (!DeviceIoControl(ui_handle->fd, IOCTL_BM_WRITE_PORT, &write_port, sizeof(write_port), (void *)buffer, length * sizeof(*buffer), &nbytes, NULL))
		return brDriverError;

	return brSuccess;
}

int arch_write_mem(struct UIHandle *ui_handle, unsigned long length, const uint32_t *buffer)
{
	DWORD  nbytes;

	if (!DeviceIoControl(ui_handle->fd, IOCTL_BM_WRITE_BUFFERED, &length, sizeof(length), (void *)buffer, length * sizeof(*buffer), &nbytes, NULL))
		return brDriverError;

	return brSuccess;
}

int arch_read_mem(struct UIHandle *ui_handle, unsigned long length, uint32_t *buffer)
{
	DWORD  nbytes;

	if (!DeviceIoControl(ui_handle->fd, IOCTL_BM_READ_BUFFERED, &length, sizeof(length), buffer, length * sizeof(*buffer), &nbytes, NULL))
		return brDriverError;

	return brSuccess;
}

int arch_read_bar3(struct UIHandle *ui_handle, unsigned long index, uint16_t *value)
{
	DWORD  offset, data, nbytes;

	offset = index / 2;

	if (!DeviceIoControl(ui_handle->fd, IOCTL_BM_READ_BAR3, &offset, sizeof(offset), &data, sizeof(data), &nbytes, NULL))
		return brDriverError;

	*value = (index & 1) ? (data >> 16) : data;

	return brSuccess;
}

int arch_write_bar3(struct UIHandle *ui_handle, unsigned long index, uint16_t value)
{
	DWORD                  nbytes;
	struct mvs_write_bar3  write_bar3;

	write_bar3.index = index;
	write_bar3.data  = value;

	if (!DeviceIoControl(ui_handle->fd, IOCTL_BM_WRITE_BAR3, NULL, 0, &write_bar3, sizeof(write_bar3), &nbytes, NULL))
		return brDriverError;

	return brSuccess;
}

int arch_read_mem_fast(struct UIHandle *ui_handle, unsigned long code, unsigned long dmc_id, unsigned long addr, unsigned long step, unsigned long length, unsigned int *buffer)
{
	DWORD              nbytes;
	struct mvs_rw_dmc  read_dmc;

	read_dmc.seg        = code;
	read_dmc.dmc_id     = dmc_id;
	read_dmc.addr       = addr;
	read_dmc.step       = step;
	read_dmc.mode       = DMC_MODE_32X32;
	read_dmc.length     = length;
	read_dmc.is_reading = 1;

	if (!DeviceIoControl(ui_handle->fd, IOCTL_BM_READWRITE_DMC, &read_dmc, sizeof(read_dmc), buffer, length * sizeof(*buffer), &nbytes, NULL))
		return brDriverError;

	return brSuccess;
}

int arch_write_mem_fast(struct UIHandle *ui_handle, unsigned long code, unsigned long dmc_id, unsigned long addr, unsigned long step, unsigned long length, const unsigned int *buffer)
{
	DWORD              nbytes;
	struct mvs_rw_dmc  write_dmc;

	write_dmc.seg        = code;
	write_dmc.dmc_id     = dmc_id;
	write_dmc.addr       = addr;
	write_dmc.step       = step;
	write_dmc.mode       = DMC_MODE_32X32;
	write_dmc.length     = length;
	write_dmc.is_reading = 0;

	if (!DeviceIoControl(ui_handle->fd, IOCTL_BM_READWRITE_DMC, &write_dmc, sizeof(write_dmc), (void *)buffer, length * sizeof(*buffer), &nbytes, NULL))
		return brDriverError;

	return brSuccess;
}

int arch_read_mem64_fast(struct UIHandle *ui_handle, unsigned long code, unsigned long dmc_id, unsigned long addr, unsigned long step, unsigned long length, uint64_t *buffer)
{
	DWORD              nbytes;
	struct mvs_rw_dmc  read_dmc;

	read_dmc.seg        = code;
	read_dmc.dmc_id     = dmc_id;
	read_dmc.addr       = addr;
	read_dmc.step       = step;
	read_dmc.mode       = DMC_MODE_COMMON;
	read_dmc.length     = length;
	read_dmc.is_reading = 1;

	if (!DeviceIoControl(ui_handle->fd, IOCTL_BM_READWRITE_DMC, &read_dmc, sizeof(read_dmc), buffer, (length / 2) * sizeof(*buffer), &nbytes, NULL))
		return brDriverError;

	return brSuccess;
}

int arch_write_mem64_fast(struct UIHandle *ui_handle, unsigned long code, unsigned long dmc_id, unsigned long addr, unsigned long step, unsigned long length, const uint64_t *buffer)
{
	DWORD              nbytes;
	struct mvs_rw_dmc  write_dmc;

	write_dmc.seg        = code;
	write_dmc.dmc_id     = dmc_id;
	write_dmc.addr       = addr;
	write_dmc.step       = step;
	write_dmc.mode       = DMC_MODE_COMMON;
	write_dmc.length     = length;
	write_dmc.is_reading = 0;

	if (!DeviceIoControl(ui_handle->fd, IOCTL_BM_READWRITE_DMC, &write_dmc, sizeof(write_dmc), (void *)buffer, (length / 2) * sizeof(*buffer), &nbytes, NULL))
		return brDriverError;

	return brSuccess;
}

void arch_msleep(int msecs)
{
	Sleep(msecs);
}

unsigned long arch_get_msecs(void)
{
	return GetTickCount();
}

int arch_spec_buff_alloc(struct UIHandle *ui_handle, SIZE_T size, void **pbuffer)
{
	int     result;
	SIZE_T  MinimumWorkingSetSize, MaximumWorkingSetSize;

	UNREFERENCED_PARAMETER(ui_handle);

	result = brMemAllocError;

	if (!GetProcessWorkingSetSize(GetCurrentProcess(), &MinimumWorkingSetSize, &MaximumWorkingSetSize))
		return result;

	if (!SetProcessWorkingSetSize(GetCurrentProcess(), MinimumWorkingSetSize + size, MaximumWorkingSetSize + size))
		return result;

	*pbuffer = VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);

	if (*pbuffer == NULL)
		goto virtual_alloc_failed;

	result = brMemLockError;

	/*
	 * Lock the specified region of the process's virtual address space into physical memory,
	 * ensuring that subsequent access to the region will not incur a page fault.
	 */
	if (!VirtualLock(*pbuffer, size))
		goto virtual_lock_failed;

	return brSuccess;

virtual_lock_failed:
	VirtualFree(*pbuffer, size, MEM_RELEASE);

virtual_alloc_failed:
	SetProcessWorkingSetSize(GetCurrentProcess(), MinimumWorkingSetSize, MaximumWorkingSetSize);

	return result;
}

int arch_spec_buff_free(struct UIHandle *ui_handle, SIZE_T size, void *buffer)
{
	SIZE_T  MinimumWorkingSetSize, MaximumWorkingSetSize;

	UNREFERENCED_PARAMETER(ui_handle);

	if (!VirtualUnlock(buffer, size))
		return brMemLockError;

	if (!VirtualFree(buffer, 0, MEM_RELEASE))
		return brMemAllocError;

	if (!GetProcessWorkingSetSize(GetCurrentProcess(), &MinimumWorkingSetSize, &MaximumWorkingSetSize))
		return brMemAllocError;

	if (!SetProcessWorkingSetSize(GetCurrentProcess(), MinimumWorkingSetSize - size, MaximumWorkingSetSize - size))
		return brMemAllocError;

	return brSuccess;
}

int arch_open_interface(struct UIHandle *ui_handle)
{
	SYSTEM_INFO  SystemInfo;

	if (!SetProcessWorkingSetSize(GetCurrentProcess(), 0x1000000, 0x2000000))
		return brMemAllocError;

	/* Open driver. */
	ui_handle->fd = CreateFile("\\\\.\\BmDev", GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);

	if (ui_handle->fd == INVALID_HANDLE_VALUE)
		return brDriverError;

	/* Create or open existing FileMapping object. */
	ui_handle->file_mapping = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "CurrentBMIndex");

	/* If the function OpenFileMapping() or CreateFileMapping() fails, the return value is NULL. */
	if (ui_handle->file_mapping == NULL)
		ui_handle->file_mapping = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(*ui_handle->shared_buf), "CurrentBMIndex");

	if (ui_handle->file_mapping == NULL)
		goto file_mapping_failed;

	ui_handle->shared_buf = MapViewOfFile(ui_handle->file_mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(*ui_handle->shared_buf));

	/* If the function MapViewOfFile() fails, the return value is NULL. */
	if (ui_handle->shared_buf == NULL)
		goto map_view_of_file_failed;

	GetSystemInfo(&SystemInfo);

	/*
	 * The page size and the granularity of page protection and commitment.
	 * This is the page size used by the VirtualAlloc() function.
	 */
	ui_handle->page_size = SystemInfo.dwPageSize;

	return brSuccess;

map_view_of_file_failed:
	CloseHandle(ui_handle->file_mapping);

file_mapping_failed:
	CloseHandle(ui_handle->fd);

	return brFailed;
}

int arch_close_interface(struct UIHandle *ui_handle)
{
	/* Close driver. */
	CloseHandle(ui_handle->fd);

	/* FileMapping */
	UnmapViewOfFile(ui_handle->shared_buf);
	CloseHandle(ui_handle->file_mapping);

	return brSuccess;
}

#endif /* WIN32 */

