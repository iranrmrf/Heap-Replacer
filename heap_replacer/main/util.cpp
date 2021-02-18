#include "util.h"

void* __cdecl operator new(size_t size)
{
	return nvhr::nvhr_malloc(size);
}

void* __cdecl operator new[](size_t size)
{
	return nvhr::nvhr_malloc(size);
}

void __cdecl operator delete(void* address)
{
	nvhr::nvhr_free(address);
}

void __cdecl operator delete[](void* address)
{
	nvhr::nvhr_free(address);
}

namespace util
{

	size_t get_highest_bit(DWORD value)
	{
		DWORD index;
		unsigned char nonzero = _BitScanReverse(&index, value);
		return index;
	}

	size_t round_pow2(size_t value)
	{
		if (value & (value - 1))
		{
			value--;
			value |= value >> 1;
			value |= value >> 2;
			value |= value >> 4;
			value |= value >> 8;
			value |= value >> 16;
			value++;
		}
		return value;
	}

	void* winapi_alloc(size_t size)
	{
		return VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	}

	void* winapi_malloc(size_t size)
	{
		return winapi_alloc(size);
	}

	void* winapi_calloc(size_t count, size_t size)
	{
		return winapi_alloc(count * size);
	}

	void winapi_free(void* address)
	{
		VirtualFree(address, 0u, MEM_RELEASE);
	}

	void memset8(void* destination, BYTE value, size_t count)
	{
		BYTE* position = (BYTE*)destination;
		for (size_t i = 0; i < count; i++) { *position++ = value; }
	}

	void memset16(void* destination, WORD value, size_t count)
	{
		WORD* position = (WORD*)destination;
		for (size_t i = 0; i < count; i++) { *position++ = value; }
	}

	void memset32(void* destination, DWORD value, size_t count)
	{
		DWORD* position = (DWORD*)destination;
		for (size_t i = 0; i < count; i++) { *position++ = value; }
	}

	void patch_bytes(uintptr_t address, BYTE* data, DWORD size)
	{
		DWORD p;
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
		patch_BYTE(address + 2, (BYTE)(argc * 4));
		patch_ret_nullptr(address + 3);
	}

	void patch_ret(uintptr_t address, size_t argc)
	{
		BYTE bytes = 0xC2;
		patch_bytes(address, &bytes, 1);
		patch_WORD(address + 1, (WORD)(4 * argc));
	}

	void patch_bp(uintptr_t address)
	{
		BYTE bytes = 0xCC;
		patch_bytes(address, &bytes, 1);
	}

	void patch_nops(uintptr_t address, size_t count)
	{
		BYTE* bytes = new BYTE[count];
		memset8(bytes, 0x90, count);
		patch_bytes(address, bytes, count);
		delete[] bytes;
	}

	void patch_nop_call(uintptr_t address)
	{
		patch_nops(address, 5);
	}

	void* get_IAT_address(BYTE* base, const char* dll_name, const char* search)
	{
		IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)base;
		IMAGE_NT_HEADERS* nt_headers = (IMAGE_NT_HEADERS*)(base + dos_header->e_lfanew);
		IMAGE_DATA_DIRECTORY section = nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
		IMAGE_IMPORT_DESCRIPTOR* import_descriptor = (IMAGE_IMPORT_DESCRIPTOR*)(base + section.VirtualAddress);
		for (size_t i = 0; import_descriptor[i].Name != 0; i++)
		{
			if (!_stricmp((char*)(base + import_descriptor[i].Name), dll_name))
			{
				if (!import_descriptor[i].FirstThunk) { return nullptr; }
				IMAGE_THUNK_DATA* name_table = (IMAGE_THUNK_DATA*)(base + import_descriptor[i].OriginalFirstThunk);
				IMAGE_THUNK_DATA* import_table = (IMAGE_THUNK_DATA*)(base + import_descriptor[i].FirstThunk);
				for (; name_table->u1.Ordinal != 0; ++name_table, ++import_table)
				{
					if (!IMAGE_SNAP_BY_ORDINAL(name_table->u1.Ordinal))
					{
						IMAGE_IMPORT_BY_NAME* import_name = (IMAGE_IMPORT_BY_NAME*)(base + name_table->u1.ForwarderString);
						char* func_name = &import_name->Name[0];
						if (!_stricmp(func_name, search)) { return &import_table->u1.AddressOfData; }
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

	bool file_exists(const char* name)
	{
		if (FILE* file = fopen(name, "r"))
		{
			fclose(file);
			return true;
		}
		return false;
	}

	void create_console()
	{
		AllocConsole();
		FILE* f;
		f = freopen("CONIN$", "r", stdin);
		f = freopen("CONOUT$", "w", stderr);
		f = freopen("CONOUT$", "w", stdout);
	}

}
