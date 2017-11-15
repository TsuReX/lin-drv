#include "HardwareInterface.h"

#include <stddef.h>

#ifndef WIN32
#	include <dlfcn.h>
#	include <stdio.h>
#	include <limits.h>
#endif

#ifndef WIN32
#	define HMODULE	void *
#endif

/* Compiler-specific Thread-Local Storage support. */
#if defined(_MSC_VER)
#	define __THREAD	__declspec(thread)
#elif !defined(__MINGW32__) || (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
#	define __THREAD	__thread
#else
#	define __THREAD
#endif

#ifdef WIN32
#	define LIBRARY_NAME	TEXT("HardwareInterface.dll")
#else
#	define LIBRARY_NAME	"HardwareInterface.so"
#endif

#ifndef WIN32
typedef int  (APIENTRY *FARPROC)(void);
#endif

/* Дескриптор библиотеки. */
static __THREAD HMODULE  library_handle;
 
/* Указатели на функции библиотеки. */
static __THREAD int  (APIENTRY *p_OpenInterface)(const char *, void **);
static __THREAD int  (APIENTRY *p_CloseInterface)(void *);

static __THREAD int  (APIENTRY *p_GetVer)(void *, const hi_version_list_t **);

static __THREAD int  (APIENTRY *p_WriteCOReg)(void *, unsigned int, unsigned int, unsigned int);
static __THREAD int  (APIENTRY *p_WriteCOCmd)(void *, unsigned int, unsigned int);

static __THREAD int  (APIENTRY *p_ReadCOReg)(void *, unsigned int, unsigned int, unsigned int *);
static __THREAD int  (APIENTRY *p_ReadCOCmd)(void *, unsigned int, unsigned int *);

static __THREAD int  (APIENTRY *p_WriteBufferCOReg)(void *, unsigned int, unsigned int, unsigned int, const unsigned int *);
static __THREAD int  (APIENTRY *p_WriteBufferCODataReg)(void *, unsigned int, unsigned int, const unsigned int *);

static __THREAD int  (APIENTRY *p_ReadBufferCOReg)(void *, unsigned int, unsigned int, unsigned int, unsigned int *);
static __THREAD int  (APIENTRY *p_ReadBufferCODataReg)(void *, unsigned int, unsigned int, unsigned int *);

static __THREAD int  (APIENTRY *p_WriteCOMem32)(void *, unsigned int, unsigned int, const unsigned int *);
static __THREAD int  (APIENTRY *p_ReadCOMem32)(void *, unsigned int, unsigned int, unsigned int *);

static __THREAD int  (APIENTRY *p_ReadMemFast)(void *, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int *);
static __THREAD int  (APIENTRY *p_WriteMemFast)(void *, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, const unsigned int *);

static __THREAD int  (APIENTRY *p_ReadMem64Fast)(void *, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, uint64_t *);
static __THREAD int  (APIENTRY *p_WriteMem64Fast)(void *, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, const uint64_t *);

static __THREAD int  (APIENTRY *p_ReadMem64)(void *, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, uint64_t *);
static __THREAD int  (APIENTRY *p_WriteMem64)(void *, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, const uint64_t *);

static __THREAD int  (APIENTRY *p_ReadDMCReg)(void *, unsigned int, unsigned int, unsigned int, unsigned int *);
static __THREAD int  (APIENTRY *p_WriteDMCReg)(void *, unsigned int, unsigned int, unsigned int, unsigned int);

static __THREAD int  (APIENTRY *p_ReadDMCReg64)(void *, unsigned int, unsigned int, unsigned int, uint64_t *);
static __THREAD int  (APIENTRY *p_WriteDMCReg64)(void *, unsigned int, unsigned int, unsigned int, uint64_t);

static __THREAD int  (APIENTRY *p_RunDMC)(void *, unsigned int);
static __THREAD int  (APIENTRY *p_StopDMC)(void *, unsigned int);

static __THREAD int  (APIENTRY *p_RunAllDMC)(void *, unsigned int);

static __THREAD int  (APIENTRY *p_WaitEndStatus)(void *, unsigned int, unsigned long *, unsigned long);

static __THREAD int  (APIENTRY *p_GlobalReset)(void *, unsigned int);

static __THREAD int  (APIENTRY *p_LoadFileFast)(void *, unsigned int, const char *);
static __THREAD int  (APIENTRY *p_LoadFile64Fast)(void *, unsigned int, const char *);

static __THREAD int  (APIENTRY *p_SpecBuffAlloc)(void *, unsigned int, uint64_t **);
static __THREAD int  (APIENTRY *p_SpecBuffFree)(void *, unsigned int, uint64_t *);

static __THREAD int  (APIENTRY *p_ReadBAR3)(void *, unsigned int, unsigned short *);
static __THREAD int  (APIENTRY *p_WriteBAR3)(void *, unsigned int, unsigned short);

/* ************************************************************************** *
 *                                                                            *
 *                           Вспомогательные функции.                         *
 *                                                                            *
 * ************************************************************************** */

#ifndef WIN32
static HMODULE LoadLibrary(const char *filename)
{
	int      result;
	char     path[PATH_MAX];
	HMODULE  handle;

	result = snprintf(path, sizeof(path), "./%s", filename);

	handle = (result < 0 || (size_t)result >= sizeof(path)) ? NULL : (HMODULE)dlopen(path, RTLD_LAZY);

	if (handle == NULL)
		handle = (HMODULE)dlopen(filename, RTLD_LAZY);

	return handle;
}
 
static __inline__ FARPROC GetProcAddress(HMODULE handle, const char *symbol)
{
	return (FARPROC)dlsym(handle, symbol);
}
#endif

static int get_symbol_address(const char *symbol, FARPROC *addrp)
{
	/* Get a handle to the library module. */
	if (library_handle == NULL) {
		library_handle = LoadLibrary(LIBRARY_NAME);

		if (library_handle == NULL)
			return brNoSuchEntry;
	}

	/* If the handle is valid, try to get the function address. */
	if (*addrp == NULL)
		*addrp = GetProcAddress(library_handle, symbol);

	return (*addrp == NULL) ? brNotImplemented : brSuccess;
}

/* ************************************************************************** *
 *                                                                            *
 *                           Интерфейсные функции.                            *
 *                                                                            *
 * ************************************************************************** */

int APIENTRY OpenInterface(const char *ip, void **handle)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_OpenInterface);

	if (result != brSuccess)
		return result;

	return (*p_OpenInterface)(ip, handle);
}

int APIENTRY CloseInterface(void *handle)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_CloseInterface);

	if (result != brSuccess)
		return result;

	return (*p_CloseInterface)(handle);
}

int APIENTRY GetVer(void *handle, const hi_version_list_t **buffer)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_GetVer);

	if (result != brSuccess)
		return result;

	return (*p_GetVer)(handle, buffer);
}

int APIENTRY WriteCOReg(void *handle, unsigned int bm_id, unsigned int reg_code, unsigned int value)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_WriteCOReg);

	if (result != brSuccess)
		return result;

	return (*p_WriteCOReg)(handle, bm_id, reg_code, value);
}

int APIENTRY WriteCOCmd(void *handle, unsigned int bm_id, unsigned int value)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_WriteCOCmd);

	if (result != brSuccess)
		return result;

	return (*p_WriteCOCmd)(handle, bm_id, value);
}

int APIENTRY ReadCOReg(void *handle, unsigned int bm_id, unsigned int reg_code, unsigned int *value)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_ReadCOReg);

	if (result != brSuccess)
		return result;

	return (*p_ReadCOReg)(handle, bm_id, reg_code, value);
}

int APIENTRY ReadCOCmd(void *handle, unsigned int bm_id, unsigned int *value)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_ReadCOCmd);

	if (result != brSuccess)
		return result;

	return (*p_ReadCOCmd)(handle, bm_id, value);
}

int APIENTRY WriteBufferCOReg(void *handle, unsigned int bm_id, unsigned int reg_code, unsigned int length, const unsigned int *buffer)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_WriteBufferCOReg);

	if (result != brSuccess)
		return result;

	return (*p_WriteBufferCOReg)(handle, bm_id, reg_code, length, buffer);
}

int APIENTRY WriteBufferCODataReg(void *handle, unsigned int bm_id, unsigned int length, const unsigned int *buffer)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_WriteBufferCODataReg);

	if (result != brSuccess)
		return result;

	return (*p_WriteBufferCODataReg)(handle, bm_id, length, buffer);
}

int APIENTRY ReadBufferCOReg(void *handle, unsigned int bm_id, unsigned int reg_code, unsigned int length, unsigned int *buffer)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_ReadBufferCOReg);

	if (result != brSuccess)
		return result;

	return (*p_ReadBufferCOReg)(handle, bm_id, reg_code, length, buffer);
}

int APIENTRY ReadBufferCODataReg(void *handle, unsigned int bm_id, unsigned int length, unsigned int *buffer)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_ReadBufferCODataReg);

	if (result != brSuccess)
		return result;

	return (*p_ReadBufferCODataReg)(handle, bm_id, length, buffer);
}

int APIENTRY WriteCOMem32(void *handle, unsigned int bm_id, unsigned int length, const unsigned int *buffer)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_WriteCOMem32);

	if (result != brSuccess)
		return result;

	return (*p_WriteCOMem32)(handle, bm_id, length, buffer);
}

int APIENTRY ReadCOMem32(void *handle, unsigned int bm_id, unsigned int length, unsigned int *buffer)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_ReadCOMem32);

	if (result != brSuccess)
		return result;

	return (*p_ReadCOMem32)(handle, bm_id, length, buffer);
}

int APIENTRY ReadMemFast(void *handle, unsigned int bm_id, unsigned int code, unsigned int dmc_id, unsigned int addr, unsigned int step, unsigned int length, unsigned int *buffer)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_ReadMemFast);

	if (result != brSuccess)
		return result;

	return (*p_ReadMemFast)(handle, bm_id, code, dmc_id, addr, step, length, buffer);
}

int APIENTRY WriteMemFast(void *handle, unsigned int bm_id, unsigned int code, unsigned int dmc_id, unsigned int addr, unsigned int step, unsigned int length, const unsigned int *buffer)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_WriteMemFast);

	if (result != brSuccess)
		return result;

	return (*p_WriteMemFast)(handle, bm_id, code, dmc_id, addr, step, length, buffer);
}

int APIENTRY ReadMem64Fast(void *handle, unsigned int bm_id, unsigned int code, unsigned int dmc_id, unsigned int addr, unsigned int step, unsigned int length, uint64_t *buffer)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_ReadMem64Fast);

	if (result != brSuccess)
		return result;

	return (*p_ReadMem64Fast)(handle, bm_id, code, dmc_id, addr, step, length, buffer);
}

int APIENTRY WriteMem64Fast(void *handle, unsigned int bm_id, unsigned int code, unsigned int dmc_id, unsigned int addr, unsigned int step, unsigned int length, const uint64_t *buffer)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_WriteMem64Fast);

	if (result != brSuccess)
		return result;

	return (*p_WriteMem64Fast)(handle, bm_id, code, dmc_id, addr, step, length, buffer);
}

int APIENTRY ReadMem64(void *handle, unsigned int bm_id, unsigned int code, unsigned int dmc_id, unsigned int addr, unsigned int step, unsigned int length, uint64_t *buffer)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_ReadMem64);

	if (result != brSuccess)
		return result;

	return (*p_ReadMem64)(handle, bm_id, code, dmc_id, addr, step, length, buffer);
}

int APIENTRY WriteMem64(void *handle, unsigned int bm_id, unsigned int code, unsigned int dmc_id, unsigned int addr, unsigned int step, unsigned int length, const uint64_t *buffer)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_WriteMem64);

	if (result != brSuccess)
		return result;

	return (*p_WriteMem64)(handle, bm_id, code, dmc_id, addr, step, length, buffer);
}

int APIENTRY ReadDMCReg(void *handle, unsigned int bm_id, unsigned int reg_code, unsigned int dmc_id, unsigned int *value)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_ReadDMCReg);

	if (result != brSuccess)
		return result;

	return (*p_ReadDMCReg)(handle, bm_id, reg_code, dmc_id, value);
}

int APIENTRY WriteDMCReg(void *handle, unsigned int bm_id, unsigned int reg_code, unsigned int dmc_id, unsigned int value)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_WriteDMCReg);

	if (result != brSuccess)
		return result;

	return (*p_WriteDMCReg)(handle, bm_id, reg_code, dmc_id, value);
}

int APIENTRY ReadDMCReg64(void *handle, unsigned int bm_id, unsigned int reg_code, unsigned int dmc_id, uint64_t *value)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_ReadDMCReg64);

	if (result != brSuccess)
		return result;

	return (*p_ReadDMCReg64)(handle, bm_id, reg_code, dmc_id, value);
}

int APIENTRY WriteDMCReg64(void *handle, unsigned int bm_id, unsigned int reg_code, unsigned int dmc_id, uint64_t value)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_WriteDMCReg64);

	if (result != brSuccess)
		return result;

	return (*p_WriteDMCReg64)(handle, bm_id, reg_code, dmc_id, value);
}

int APIENTRY RunDMC(void *handle, unsigned int bm_id)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_RunDMC);

	if (result != brSuccess)
		return result;

	return (*p_RunDMC)(handle, bm_id);
}

int APIENTRY StopDMC(void *handle, unsigned int bm_id)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_StopDMC);

	if (result != brSuccess)
		return result;

	return (*p_StopDMC)(handle, bm_id);
}

int APIENTRY RunAllDMC(void *handle, unsigned int bm_id)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_RunAllDMC);

	if (result != brSuccess)
		return result;

	return (*p_RunAllDMC)(handle, bm_id);
}

int APIENTRY WaitEndStatus(void *handle, unsigned int bm_id, unsigned long *time, unsigned long timeout)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_WaitEndStatus);

	if (result != brSuccess)
		return result;

	return (*p_WaitEndStatus)(handle, bm_id, time, timeout);
}

int APIENTRY GlobalReset(void *handle, unsigned int bm_id)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_GlobalReset);

	if (result != brSuccess)
		return result;

	return (*p_GlobalReset)(handle, bm_id);
}

int APIENTRY LoadFileFast(void *handle, unsigned int bm_id, const char *filename)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_LoadFileFast);

	if (result != brSuccess)
		return result;

	return (*p_LoadFileFast)(handle, bm_id, filename);
}

int APIENTRY LoadFile64Fast(void *handle, unsigned int bm_id, const char *filename)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_LoadFile64Fast);

	if (result != brSuccess)
		return result;

	return (*p_LoadFile64Fast)(handle, bm_id, filename);
}

int APIENTRY SpecBuffAlloc(void *handle, unsigned int length, uint64_t **pbuf)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_SpecBuffAlloc);

	if (result != brSuccess)
		return result;

	return (*p_SpecBuffAlloc)(handle, length, pbuf);
}

int APIENTRY SpecBuffFree(void *handle, unsigned int length, uint64_t *buf)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_SpecBuffFree);

	if (result != brSuccess)
		return result;

	return (*p_SpecBuffFree)(handle, length, buf);
}

int APIENTRY ReadBAR3(void *handle, unsigned int index, unsigned short *value)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_ReadBAR3);

	if (result != brSuccess)
		return result;

	return (*p_ReadBAR3)(handle, index, value);
}

int APIENTRY WriteBAR3(void *handle, unsigned int index, unsigned short value)
{
	int  result;

	result = get_symbol_address(__FUNCTION__, (FARPROC *)&p_WriteBAR3);

	if (result != brSuccess)
		return result;

	return (*p_WriteBAR3)(handle, index, value);
}

