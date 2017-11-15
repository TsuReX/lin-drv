#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "internals.h"

#define CMD_REG		(2)
#define DATA_REG	(3)

/** Версия Mercurial библиотеки. */
static const hi_component_mercurial_version_t  library_mercurial_version = {
	MVS_MERCURIAL_LONG_CHANGESET,
	MVS_MERCURIAL_SHORT_CHANGESET,
};

/** Версия Mercurial драйвера. */
static hi_component_mercurial_version_t  driver_mercurial_version;

/** Элементы списка версий компонент. */
static hi_version_list_item_t  version_list_item[] = {
	/* Элемент "библиотека" списка версий компонент. */
	{
		HI_LIBRARY_COMPONENT_ID,
		sizeof(library_mercurial_version),
		&library_mercurial_version
	},

	/* Элемент "драйвер" списка версий компонент. */
	{
		HI_DRIVER_COMPONENT_ID,
		sizeof(driver_mercurial_version),
		&driver_mercurial_version
	},
};

/** Список версий компонент. */
static hi_version_list_t  version_list = {
	sizeof(version_list_item) / sizeof(version_list_item[0]),
	version_list_item
};

/*
 * *************************************************************************** *
 *				Helper functions.
 * *************************************************************************** *
 */
static int generic_switch_bm(struct UIHandle *ui_handle, uint32_t bm_id)
{
	int  result;

	if (ui_handle == NULL)
		return brBadHandle;

	result = brSuccess;

	if (*ui_handle->shared_buf != bm_id) {
		result = arch_write_port(ui_handle, 8, 1, &bm_id);

		if (result == brSuccess)
			*ui_handle->shared_buf = bm_id;
	}

	return result;
}

static __inline__ int generic_write_co_cmd_reg(struct UIHandle *ui_handle, uint32_t value)
{
	return arch_write_port(ui_handle, CMD_REG, 1, &value);
}

static __inline__ int generic_write_co_data_reg(struct UIHandle *ui_handle, uint32_t value)
{
	return arch_write_port(ui_handle, DATA_REG, 1, &value);
}

static int generic_write_co_data_reg64(struct UIHandle *ui_handle, uint64_t value)
{
	int  result;

	result = generic_write_co_data_reg(ui_handle, value >> 32);

	if (result == brSuccess)
		result = generic_write_co_data_reg(ui_handle, value);

	return result;
}

static int generic_read_co_data_reg64(struct UIHandle *ui_handle, uint64_t *value)
{
	int       result;
	uint32_t  msw, lsw;

	result = arch_read_port(ui_handle, DATA_REG, 1, &lsw);

	if (result == brSuccess) {
		result = arch_read_port(ui_handle, DATA_REG, 1, &msw);

		if (result == brSuccess)
			*value = ((uint64_t)msw << 32) | lsw;
	}

	return result;
}

static __inline__ uint32_t bswap_32(uint32_t bsx)
{
	return (((bsx & 0xff000000) >> 24) | ((bsx & 0x00ff0000) >> 8) | ((bsx & 0x0000ff00) << 8) | ((bsx & 0x000000ff) << 24));
}

static __inline__ SIZE_T page_aligned_size(struct UIHandle *ui_handle, SIZE_T nelem, SIZE_T elsize)
{
	/* Data size with page size granularity. */
	return (((nelem * elsize + ui_handle->page_size - 1) / ui_handle->page_size) * ui_handle->page_size);
}

/*
 * *************************************************************************** *
 *				API functions.
 * *************************************************************************** *
 */
int APIENTRY OpenInterface(const char *ip, void **handle)
{
	int               result;
	struct UIHandle  *ui_handle;

	ui_handle = calloc(1, sizeof(*ui_handle));

	if (ui_handle == NULL)
		return brMemAllocError;

	result = arch_open_interface(ui_handle);

	if (result == brSuccess) {
		*ui_handle->shared_buf = (unsigned int)(-1);

		*handle = ui_handle;
	}

	return result;
}

int APIENTRY CloseInterface(void *handle)
{
	int  result;

	if (handle == NULL)
		return brBadHandle;

	result = arch_close_interface(handle);

	if (result == brSuccess)
		free(handle);

	return result;
}

int APIENTRY GetVer(void *handle, const hi_version_list_t **buffer)
{
	version_list.count = sizeof(version_list_item) / sizeof(version_list_item[0]);

	if (handle == NULL || arch_get_driver_version(handle, &driver_mercurial_version) != brSuccess)
		--version_list.count;

	if (buffer != NULL)
		*buffer = &version_list;

	return brSuccess;
}

int APIENTRY WriteCOReg(void *handle, unsigned int bm_id, unsigned int reg_code, unsigned int value)
{
	int  result;

	result = generic_switch_bm(handle, bm_id);

	if (result == brSuccess)
		result = arch_write_port(handle, reg_code, 1, &value);

	return result;
}

int APIENTRY WriteCOCmd(void *handle, unsigned int bm_id, unsigned int value)
{
	int  result;

	result = generic_switch_bm(handle, bm_id);

	if (result == brSuccess) {
		result = generic_write_co_cmd_reg(handle, 4);

		if (result == brSuccess)
			result = generic_write_co_data_reg(handle, value);
	}

	return result;
}

int APIENTRY ReadCOReg(void *handle, unsigned int bm_id, unsigned int reg_code, unsigned int *value)
{
	int  result;

	result = generic_switch_bm(handle, bm_id);

	if (result == brSuccess)
		result = arch_read_port(handle, reg_code, 1, value);

	return result;
}

int APIENTRY ReadCOCmd(void *handle, unsigned int bm_id, unsigned int *value)
{
	int  result;

	result = generic_switch_bm(handle, bm_id);

	if (result == brSuccess)
		result = arch_read_port(handle, CMD_REG, 1, value);

	return result;
}

int APIENTRY WriteBufferCOReg(void *handle, unsigned int bm_id, unsigned int reg_code, unsigned int length, const unsigned int *buffer)
{
	int  result;

	result = generic_switch_bm(handle, bm_id);

	if (result == brSuccess)
		result = arch_write_port(handle, reg_code, length, buffer);

	return result;
}

int APIENTRY WriteBufferCODataReg(void *handle, unsigned int bm_id, unsigned int length, const unsigned int *buffer)
{
	int  result;

	result = generic_switch_bm(handle, bm_id);

	if (result == brSuccess)
		result = arch_write_port(handle, DATA_REG, length, buffer);

	return result;
}

int APIENTRY ReadBufferCOReg(void *handle, unsigned int bm_id, unsigned int reg_code, unsigned int length, unsigned int *buffer)
{
	int  result;

	result = generic_switch_bm(handle, bm_id);

	if (result == brSuccess)
		result = arch_read_port(handle, reg_code, length, buffer);

	return result;
}

int APIENTRY ReadBufferCODataReg(void *handle, unsigned int bm_id, unsigned int length, unsigned int *buffer)
{
	int  result;

	result = generic_switch_bm(handle, bm_id);

	if (result == brSuccess)
		result = arch_read_port(handle, DATA_REG, length, buffer);

	return result;
}

int APIENTRY WriteCOMem32(void *handle, unsigned int bm_id, unsigned int length, const unsigned int *buffer)
{
	int  result;

	result = generic_switch_bm(handle, bm_id);

	if (result == brSuccess)
		result = arch_write_mem(handle, length, buffer);

	return result;
}

int APIENTRY ReadCOMem32(void *handle, unsigned int bm_id, unsigned int length, unsigned int *buffer)
{
	int  result;

	result = generic_switch_bm(handle, bm_id);

	if (result == brSuccess)
		result = arch_read_mem(handle, length, buffer);

	return result;
}

int APIENTRY ReadMemFast(void *handle, unsigned int bm_id, unsigned int code, unsigned int dmc_id, unsigned int addr, unsigned int step, unsigned int length, unsigned int *buffer)
{
	int  result;

	result = generic_switch_bm(handle, bm_id);

	if (result == brSuccess)
		result = arch_read_mem_fast(handle, code, dmc_id, addr, step, length + (length & 1), buffer);

	return result;
}

int APIENTRY WriteMemFast(void *handle, unsigned int bm_id, unsigned int code, unsigned int dmc_id, unsigned int addr, unsigned int step, unsigned int length, const unsigned int *buffer)
{
	int        result;
	uint64_t  *temp;

	result = (length & 1) ? SpecBuffAlloc(handle, 1, &temp) : brSuccess;

	if (result != brSuccess)
		return result;

	result = generic_switch_bm(handle, bm_id);

	if (result != brSuccess)
		goto out;

	result = arch_write_mem_fast(handle, code, dmc_id, addr, step, length & ~1UL, buffer);

	if (result != brSuccess)
		goto out;

	if (length & 1) {
		addr += (length & ~1UL) * step;

		result = arch_read_mem_fast(handle, code, dmc_id, addr, step, 2, (unsigned int *)temp);

		if (result != brSuccess)
			goto out;

		*temp = (*temp & ~0xffffffffULL) | buffer[length & ~1UL];

		result = arch_write_mem_fast(handle, code, dmc_id, addr, step, 2, (const unsigned int *)temp);
	}

out:
	if (length & 1)
		SpecBuffFree(handle, 1, temp);

	return result;
}

int APIENTRY ReadMem64Fast(void *handle, unsigned int bm_id, unsigned int code, unsigned int dmc_id, unsigned int addr, unsigned int step, unsigned int length, uint64_t *buffer)
{
	int  result;

	result = generic_switch_bm(handle, bm_id);

	if (result == brSuccess)
		result = arch_read_mem64_fast(handle, code, dmc_id, addr, step, length * 2, buffer);

	return result;
}

int APIENTRY WriteMem64Fast(void *handle, unsigned int bm_id, unsigned int code, unsigned int dmc_id, unsigned int addr, unsigned int step, unsigned int length, const uint64_t *buffer)
{
	int  result;

	result = generic_switch_bm(handle, bm_id);

	if (result == brSuccess)
		result = arch_write_mem64_fast(handle, code, dmc_id, addr, step, length * 2, buffer);

	return result;
}

int APIENTRY ReadMem64(void *handle, unsigned int bm_id, unsigned int code, unsigned int dmc_id, unsigned int addr, unsigned int step, unsigned int length, uint64_t *buffer)
{
	return brNotImplemented;
}

int APIENTRY WriteMem64(void *handle, unsigned int bm_id, unsigned int code, unsigned int dmc_id, unsigned int addr, unsigned int step, unsigned int length, const uint64_t *buffer)
{
	return brNotImplemented;
}

int APIENTRY ReadDMCReg(void *handle, unsigned int bm_id, unsigned int reg_code, unsigned int dmc_id, unsigned int *value)
{
	int  result;

	result = generic_switch_bm(handle, bm_id);

	if (result != brSuccess)
		return result;

	result = generic_write_co_cmd_reg(handle, 4);

	if (result != brSuccess)
		return result;

	result = generic_write_co_data_reg(handle, (dmc_id << 24) | (0 << 15) | reg_code);

	if (result != brSuccess)
		return result;

	result = generic_write_co_cmd_reg(handle, 1);

	if (result != brSuccess)
		return result;

	result = arch_read_port(handle, DATA_REG, 1, value);

	return result;
}

int APIENTRY WriteDMCReg(void *handle, unsigned int bm_id, unsigned int reg_code, unsigned int dmc_id, unsigned int value)
{
	int  result;

	result = generic_switch_bm(handle, bm_id);

	if (result != brSuccess)
		return result;

	result = generic_write_co_cmd_reg(handle, 4);

	if (result != brSuccess)
		return result;

	result = generic_write_co_data_reg(handle, (dmc_id << 24) | (1 << 15) | reg_code);

	if (result != brSuccess)
		return result;

	result = generic_write_co_data_reg(handle, value);

	return result;
}

int APIENTRY ReadDMCReg64(void *handle, unsigned int bm_id, unsigned int reg_code, unsigned int dmc_id, uint64_t *value)
{
	int  result;

	result = generic_switch_bm(handle, bm_id);

	if (result != brSuccess)
		return result;

	result = generic_write_co_cmd_reg(handle, 4);

	if (result != brSuccess)
		return result;

	result = generic_write_co_data_reg(handle, dmc_id << 24);

	if (result != brSuccess)
		return result;

	result = generic_write_co_cmd_reg(handle, 4);

	if (result != brSuccess)
		return result;

	result = generic_write_co_data_reg(handle, reg_code);

	if (result != brSuccess)
		return result;

	result = generic_write_co_cmd_reg(handle, 1);

	if (result != brSuccess)
		return result;

	result = generic_read_co_data_reg64(handle, value);

	return result;
}

int APIENTRY WriteDMCReg64(void *handle, unsigned int bm_id, unsigned int reg_code, unsigned int dmc_id, uint64_t value)
{
	int  result;

	result = generic_switch_bm(handle, bm_id);

	if (result != brSuccess)
		return result;

	result = generic_write_co_cmd_reg(handle, 4);

	if (result != brSuccess)
		return result;

	result = generic_write_co_data_reg(handle, dmc_id << 24);

	if (result != brSuccess)
		return result;

	result = generic_write_co_cmd_reg(handle, 4);

	if (result != brSuccess)
		return result;

	result = generic_write_co_data_reg(handle, reg_code | 0x8000);

	if (result != brSuccess)
		return result;

	result = generic_write_co_data_reg64(handle, value);

	return result;
}

int APIENTRY RunDMC(void *handle, unsigned int bm_id)
{
	int  result;

	result = generic_switch_bm(handle, bm_id);

	if (result != brSuccess)
		return result;

	result = generic_write_co_cmd_reg(handle, 3);

	if (result != brSuccess)
		return result;

	arch_msleep(1);

	result = generic_write_co_cmd_reg(handle, 2);

	return result;
}

int APIENTRY StopDMC(void *handle, unsigned int bm_id)
{
	int  result;

	result = generic_switch_bm(handle, bm_id);

	if (result == brSuccess)
		result = generic_write_co_cmd_reg(handle, 0);

	return result;
}

int APIENTRY RunAllDMC(void *handle, unsigned int bm_id)
{
	return brNotImplemented;
}

int APIENTRY WaitEndStatus(void *handle, unsigned int bm_id, unsigned long *time, unsigned long timeout)
{
	int            result;
	unsigned int   temp;
	unsigned long  before;

	result = generic_switch_bm(handle, bm_id);

	if (result != brSuccess)
		return result;

	before = arch_get_msecs();

	do {
		result = arch_read_port(handle, CMD_REG, 1, &temp);

		if (result != brSuccess)
			return result;

		*time = arch_get_msecs() - before;

		if (temp & 0x20)
			return brSuccess;

	} while (*time < timeout);

	return brTimeout;
}

int APIENTRY GlobalReset(void *handle, unsigned int bm_id)
{
	int  result;

	result = generic_switch_bm(handle, bm_id);

	if (result == brSuccess)
		result = generic_write_co_cmd_reg(handle, 3);

	if (result == brSuccess)
		arch_msleep(1);

	return result;
}

int APIENTRY LoadFileFast(void *handle, unsigned int bm_id, const char *filename)
{
	char           extra_filename[PATH_MAX];
	uint64_t      *spec_buffer;
	int            result;
	FILE          *stream, *extra_stream;
	unsigned int   bm;
	struct stat    stat;
	uint32_t       code, dmc_id;
	uint64_t       i, addr, step, length, extra_filename_size;
	unsigned int  *temp;

	result = brFileLoadError;

	stream = fopen(filename, "rb");

	if (stream == NULL)
		return result;

	if (fstat(fileno(stream), &stat) < 0)
		goto close_stream_and_exit;

	bm = bm_id;

	while (ftell(stream) < stat.st_size) {
		result = brFileLoadError;

		if (fread(&code, sizeof(code), 1, stream) != 1)
			goto close_stream_and_exit;

		switch (code) {
			case (0x8000 | HI_SC_OPER_SEG):
			case (0x8000 | HI_SC_PARAM_SEG):
			case (0x8000 | HI_SC_DATA_SEG):
			case (0x8000 | HI_SC_CYCLE_SEG):
				if (fread(&dmc_id, sizeof(dmc_id), 1, stream) != 1)
					goto close_stream_and_exit;

				dmc_id = bswap_32(dmc_id);

				if (fread(&addr, sizeof(addr), 1, stream) != 1)
					goto close_stream_and_exit;

				if (fread(&step, sizeof(step), 1, stream) != 1)
					goto close_stream_and_exit;

				if (fread(&length, sizeof(length), 1, stream) != 1)
					goto close_stream_and_exit;

				result = SpecBuffAlloc(handle, length, &spec_buffer);

				if (result != brSuccess)
					goto close_stream_and_exit;

				result = brFileLoadError;

				if (fread(spec_buffer, sizeof(*spec_buffer), length, stream) != length)
					goto free_spec_buffer_and_exit;

				if (code == (0x8000 | HI_SC_OPER_SEG)) {
					result = WriteMemFast(handle, bm, code & 0xf, dmc_id, addr, step, length * 2, (const unsigned int *)spec_buffer);

				} else {
					temp = (unsigned int *)spec_buffer;

					for (i = 0; i < length; ++i)
						temp[i] = spec_buffer[i];

					result = WriteMemFast(handle, bm, code & 0xf, dmc_id, addr, step, length, (const unsigned int *)spec_buffer);
				}

				SpecBuffFree(handle, length, spec_buffer);

				if (result != brSuccess)
					goto close_stream_and_exit;
				break;

			case (0x8100 | HI_SC_OPER_SEG):
			case (0x8100 | HI_SC_PARAM_SEG):
			case (0x8100 | HI_SC_DATA_SEG):
			case (0x8100 | HI_SC_CYCLE_SEG):
				if (fread(&dmc_id, sizeof(dmc_id), 1, stream) != 1)
					goto close_stream_and_exit;

				dmc_id = bswap_32(dmc_id);

				if (fread(&addr, sizeof(addr), 1, stream) != 1)
					goto close_stream_and_exit;

				if (fread(&step, sizeof(step), 1, stream) != 1)
					goto close_stream_and_exit;

				if (fread(&length, sizeof(length), 1, stream) != 1)
					goto close_stream_and_exit;

				if (fread(&extra_filename_size, sizeof(extra_filename_size), 1, stream) != 1 || extra_filename_size >= sizeof(extra_filename))
					goto close_stream_and_exit;

				if (fread(extra_filename, extra_filename_size, 1, stream) != 1)
					goto close_stream_and_exit;

				extra_filename[extra_filename_size] = '\0';

				result = SpecBuffAlloc(handle, length, &spec_buffer);

				if (result != brSuccess)
					goto close_stream_and_exit;

				result = brFileLoadError;

				extra_stream = fopen(extra_filename, "rb");

				if (extra_stream == NULL)
					goto free_spec_buffer_and_exit;

				if (fread(spec_buffer, sizeof(*spec_buffer), length, extra_stream) != length)
					goto close_extra_stream_and_exit;

				fclose(extra_stream);

				if (code == (0x8100 | HI_SC_OPER_SEG)) {
					result = WriteMemFast(handle, bm, code & 0xf, dmc_id, addr, step, length * 2, (const unsigned int *)spec_buffer);

				} else {
					temp = (unsigned int *)spec_buffer;

					for (i = 0; i < length; ++i)
						temp[i] = spec_buffer[i];

					result = WriteMemFast(handle, bm, code & 0xf, dmc_id, addr, step, length, (const unsigned int *)spec_buffer);
				}

				SpecBuffFree(handle, length, spec_buffer);

				if (result != brSuccess)
					goto close_stream_and_exit;
				break;

			default:
				if (bm_id == ~0 && (code & 0xffff) == 0xaaaa)
					bm = (code >> 24) + ((code >> 8) & 0xff00);
				break;
		}
	}

	result = brSuccess;

	goto close_stream_and_exit;

close_extra_stream_and_exit:
	fclose(extra_stream);

free_spec_buffer_and_exit:
	SpecBuffFree(handle, length, spec_buffer);

close_stream_and_exit:
	fclose(stream);

	return result;
}

int APIENTRY LoadFile64Fast(void *handle, unsigned int bm_id, const char *filename)
{
	char          extra_filename[PATH_MAX];
	uint64_t     *spec_buffer;
	int           result;
	FILE         *stream, *extra_stream;
	struct stat   stat;
	unsigned int  bm;
	uint32_t      code, dmc_id;
	uint64_t      addr, step, length, extra_filename_size;

	result = brFileLoadError;

	stream = fopen(filename, "rb");

	if (stream == NULL)
		return result;

	if (fstat(fileno(stream), &stat) < 0)
		goto close_stream_and_exit;

	bm = bm_id;

	while (ftell(stream) < stat.st_size) {
		result = brFileLoadError;

		if (fread(&code, sizeof(code), 1, stream) != 1)
			goto close_stream_and_exit;

		switch (code) {
			case (0x8000 | HI_SC_OPER_SEG):
			case (0x8000 | HI_SC_PARAM_SEG):
			case (0x8000 | HI_SC_DATA_SEG):
			case (0x8000 | HI_SC_CYCLE_SEG):
				if (fread(&dmc_id, sizeof(dmc_id), 1, stream) != 1)
					goto close_stream_and_exit;

				dmc_id = bswap_32(dmc_id);

				if (fread(&addr, sizeof(addr), 1, stream) != 1)
					goto close_stream_and_exit;

				if (fread(&step, sizeof(step), 1, stream) != 1)
					goto close_stream_and_exit;

				if (fread(&length, sizeof(length), 1, stream) != 1)
					goto close_stream_and_exit;

				result = SpecBuffAlloc(handle, length, &spec_buffer);

				if (result != brSuccess)
					goto close_stream_and_exit;

				result = brFileLoadError;

				if (fread(spec_buffer, sizeof(*spec_buffer), length, stream) != length)
					goto free_spec_buffer_and_exit;

				result = WriteMem64Fast(handle, bm, code & 0xff, dmc_id, addr, step, length, spec_buffer);

				SpecBuffFree(handle, length, spec_buffer);

				if (result != brSuccess)
					goto close_stream_and_exit;
				break;

			case (0x8100 | HI_SC_OPER_SEG):
			case (0x8100 | HI_SC_PARAM_SEG):
			case (0x8100 | HI_SC_DATA_SEG):
			case (0x8100 | HI_SC_CYCLE_SEG):
				if (fread(&dmc_id, sizeof(dmc_id), 1, stream) != 1)
					goto close_stream_and_exit;

				dmc_id = bswap_32(dmc_id);

				if (fread(&addr, sizeof(addr), 1, stream) != 1)
					goto close_stream_and_exit;

				if (fread(&step, sizeof(step), 1, stream) != 1)
					goto close_stream_and_exit;

				if (fread(&length, sizeof(length), 1, stream) != 1)
					goto close_stream_and_exit;

				if (fread(&extra_filename_size, sizeof(extra_filename_size), 1, stream) != 1 || extra_filename_size >= sizeof(extra_filename))
					goto close_stream_and_exit;

				if (fread(extra_filename, extra_filename_size, 1, stream) != 1)
					goto close_stream_and_exit;

				extra_filename[extra_filename_size] = '\0';

				result = SpecBuffAlloc(handle, length, &spec_buffer);

				if (result != brSuccess)
					goto close_stream_and_exit;

				result = brFileLoadError;

				extra_stream = fopen(extra_filename, "rb");

				if (extra_stream == NULL)
					goto free_spec_buffer_and_exit;

				if (fread(spec_buffer, sizeof(*spec_buffer), length, extra_stream) != length)
					goto close_extra_stream_and_exit;

				fclose(extra_stream);

				result = WriteMem64Fast(handle, bm, code & 0xff, dmc_id, addr, step, length, spec_buffer);

				SpecBuffFree(handle, length, spec_buffer);

				if (result != brSuccess)
					goto close_stream_and_exit;
				break;

			default:
				if (bm_id == ~0 && (code & 0xffff) == 0xaaaa)
					bm = (code >> 24) + ((code >> 8) & 0xff00);
				break;
		}
	}

	result = brSuccess;

	goto close_stream_and_exit;

close_extra_stream_and_exit:
	fclose(extra_stream);

free_spec_buffer_and_exit:
	SpecBuffFree(handle, length, spec_buffer);

close_stream_and_exit:
	fclose(stream);

	return result;
}

int APIENTRY SpecBuffAlloc(void *handle, unsigned int length, uint64_t **pbuffer)
{
	if (handle == NULL)
		return brBadHandle;

	if (pbuffer == NULL)
		return brBadAddress;

	*pbuffer = NULL;

	if (length != 0)
		return arch_spec_buff_alloc(handle, page_aligned_size(handle, length, sizeof(**pbuffer)), (void **)pbuffer);

	return brSuccess;
}

int APIENTRY SpecBuffFree(void *handle, unsigned int length, uint64_t *buffer)
{
	if (handle == NULL)
		return brBadHandle;

	if (length != 0)
		return arch_spec_buff_free(handle, page_aligned_size(handle, length, sizeof(*buffer)), buffer);

	return brSuccess;
}

int APIENTRY ReadBAR3(void *handle, unsigned int index, unsigned short *value)
{
	if (handle == NULL)
		return brBadHandle;

	return arch_read_bar3(handle, index, value);
}

int APIENTRY WriteBAR3(void *handle, unsigned int index, unsigned short value)
{
	if (handle == NULL)
		return brBadHandle;

	return arch_write_bar3(handle, index, value);
}

