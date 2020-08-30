#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>

#pragma warning(disable:4244)

#pragma warning(disable:6031)
#pragma warning(disable:6250)

constexpr size_t KB = 1024 * 1u;
constexpr size_t MB = 1024 * KB;
constexpr size_t GB = 1024 * MB;

void*	__fastcall	nvhr_malloc(size_t size);
void*	__fastcall	nvhr_calloc(size_t count, size_t size);
void	__fastcall	nvhr_free(void* address);

FILE* file = fopen("log.log", "w");

void* winapi_alloc(size_t size)
{
	return VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

void patch_bytes(uintptr_t address, BYTE* data, DWORD size)
{
	DWORD p = 0;
	VirtualProtect((void*)address, size, PAGE_EXECUTE_READWRITE, &p);
	memcpy((void*)address, data, size);
	VirtualProtect((void*)address, size, p, &p);
	FlushInstructionCache(GetCurrentProcess(), (void*)address, size);
}

void patch_BYTE(uintptr_t address, BYTE data)
{
	patch_bytes(address, &data, 1);
}

void patch_WORD(uintptr_t address, WORD data)
{
	BYTE bytes[2];
	*(WORD*)bytes = data;
	patch_bytes(address, bytes, 2);
}

void patch_DWORD(uintptr_t address, DWORD data)
{
	BYTE bytes[4];
	*(DWORD*)bytes = data;
	patch_bytes(address, bytes, 4);
}

void patch_call(uintptr_t address, void* destination)
{
	BYTE bytes[5];
	bytes[0] = 0xE8;
	*(DWORD*)((DWORD)bytes + 1) = (DWORD)destination - (DWORD)address - 5;
	patch_bytes(address, bytes, 5);
}

void patch_jmp(uintptr_t address, void* destination)
{
	BYTE bytes[5];
	bytes[0] = 0xE9;
	*(DWORD*)((DWORD)bytes + 1) = (DWORD)destination - (DWORD)address - 5;
	patch_bytes(address, bytes, 5);
}

void patch_ret(uintptr_t address)
{
	BYTE bytes = 0xC3;
	patch_bytes(address, &bytes, 1);
}

void patch_ret_nullptr(uintptr_t address)
{
	patch_bytes(address, (BYTE*)"\x31\xC0", 2);
	patch_ret(address + 2);
}

void patch_ret_nullptr(uintptr_t address, size_t argc)
{
	patch_bytes(address, (BYTE*)"\x83\xC4", 2);
	patch_BYTE(address + 2, argc * 4);
	patch_ret_nullptr(address + 3);
}

void patch_ret(uintptr_t address, size_t argc)
{
	BYTE bytes = 0xC2;
	patch_bytes(address, &bytes, 1);
	patch_WORD(address + 1, 4 * argc);
}

void patch_bp(uintptr_t address)
{
	BYTE bytes = 0xCC;
	patch_bytes(address, &bytes, 1);
}

void patch_nops(uintptr_t address, size_t count)
{
	DWORD p = 0;
	VirtualProtect((void*)address, count, PAGE_EXECUTE_READWRITE, &p);
	memset((void*)address, 0x90, count);
	VirtualProtect((void*)address, count, p, &p);
	FlushInstructionCache(GetCurrentProcess(), (void*)address, count);
}

size_t next_power_of_2(size_t size)
{
	if (size & (size - 1))
	{
		size--;
		size |= size >> 1;
		size |= size >> 2;
		size |= size >> 4;
		size |= size >> 8;
		size |= size >> 16;
		size++;
	}
	return size;
}

void* get_IAT_address(BYTE* base, const char* dll_name, const char* search)
{
	IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)base;
	IMAGE_NT_HEADERS* nt_headers = (IMAGE_NT_HEADERS*)(base + dos_header->e_lfanew);
	IMAGE_DATA_DIRECTORY* data_directory = nt_headers->OptionalHeader.DataDirectory;
	IMAGE_DATA_DIRECTORY section = data_directory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	IMAGE_IMPORT_DESCRIPTOR* importDescriptor = (IMAGE_IMPORT_DESCRIPTOR*)(base + section.VirtualAddress);
	for (size_t i = 0; importDescriptor[i].Name != 0; i++)
	{
		char* curr_dll_name = (char*)(base + importDescriptor[i].Name);
		if (!_stricmp(curr_dll_name, dll_name))
		{
			if (!importDescriptor[i].FirstThunk)
			{
				return 0;
			}
			IMAGE_THUNK_DATA* name_table = (IMAGE_THUNK_DATA*)(base + importDescriptor[i].OriginalFirstThunk);
			IMAGE_THUNK_DATA* import_table = (IMAGE_THUNK_DATA*)(base + importDescriptor[i].FirstThunk);
			for (; name_table->u1.Ordinal != 0; ++name_table, ++import_table)
			{
				if (!IMAGE_SNAP_BY_ORDINAL(name_table->u1.Ordinal))
				{
					IMAGE_IMPORT_BY_NAME* import_name = (IMAGE_IMPORT_BY_NAME*)(base + name_table->u1.ForwarderString);
					char* func_name = &import_name->Name[0];
					if (!_stricmp(func_name, search))
					{
						return &import_table->u1.AddressOfData;
					}
				}
			}
		}
	}
	return nullptr;
}

bool is_LAA(BYTE* base)
{
	IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)base;
	IMAGE_NT_HEADERS* nt_headers = (IMAGE_NT_HEADERS*)(base + dos_header->e_lfanew);
	return nt_headers->FileHeader.Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE;
}

bool exists(const char* name)
{
	FILE* file;
	if (file = fopen(name, "r"))
	{
		fclose(file);
		return true;
	}
	return false;
}

void create_console()
{
	AllocConsole();
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stderr);
	freopen("CONOUT$", "w", stdout);
}
