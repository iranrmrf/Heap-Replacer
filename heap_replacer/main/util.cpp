#include "util.h"

void* __cdecl operator new(size_t size)
{
	return hr::hr_malloc(size);
}

void* __cdecl operator new[](size_t size)
{
	return hr::hr_malloc(size);
}

void __cdecl operator delete(void* address)
{
	hr::hr_free(address);
}

void __cdecl operator delete[](void* address)
{
	hr::hr_free(address);
}

namespace util
{

	size_t get_highest_bit(size_t n)
	{
		return 31u - __lzcnt(n);
	}

	size_t round_pow2(size_t n)
	{
		return (1u << (32u - __lzcnt(n - 1u)));
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

	char ctoupper(char c)
	{
		return c &= ~(!((c < 'a') | (c > 'z')) << 5);
	}

	char ctolower(char c)
	{
		return c |= (!((c < 'A') | (c > 'Z')) << 5);
	}

	int cstrcmp(const char* s1, const char* s2)
	{
		while ((!!(*s1)) & (*s1 == *s2)) { s1++; s2++; } return *s1 - *s2;
	}

	int cstricmp(const char* s1, const char* s2)
	{
		while ((!!(*s1)) & (tolower(*s1) == tolower(*s2))) { s1++; s2++; } return tolower(*s1) - tolower(*s2);
	}

	void cmemcpy(void* dst, const void* src, size_t cnt)
	{
		for (BYTE* d = (BYTE*)dst, *s = (BYTE*)src; cnt--; d++, s++) { *d = *s; }
	}

	void cmemset8(void* dst, BYTE val, size_t cnt)
	{
		for (BYTE* pos = (BYTE*)dst; cnt--; pos++) { *pos = val; }
	}

	void cmemset16(void* dst, WORD val, size_t cnt)
	{
		for (WORD* pos = (WORD*)dst; cnt--; pos++) { *pos = val; }
	}

	void cmemset32(void* dst, DWORD val, size_t cnt)
	{
		for (DWORD* pos = (DWORD*)dst; cnt--; pos++) { *pos = val; }
	}

	void patch_bytes(void* address, BYTE* data, DWORD size)
	{
		DWORD p;
		VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &p);
		cmemcpy(address, data, size);
		VirtualProtect(address, size, p, &p);
		FlushInstructionCache(GetCurrentProcess(), address, size);
	}

	void patch_bytes(uintptr_t address, BYTE* data, DWORD size)
	{
		patch_bytes((void*)address, data, size);
	}

	void patch_BYTE(void* address, BYTE data)
	{
		patch_bytes(address, &data, 1u);
	}

	void patch_WORD(void* address, WORD data)
	{
		BYTE bytes[2];
		*(WORD*)bytes = data;
		patch_bytes(address, bytes, 2u);
	}

	void patch_DWORD(void* address, DWORD data)
	{
		BYTE bytes[4];
		*(DWORD*)bytes = data;
		patch_bytes(address, bytes, 4u);
	}

	void patch_func_ptr(void* address, void* ptr)
	{
		patch_DWORD(address, (DWORD)ptr);
	}

	void patch_func_ptr(uintptr_t address, void* ptr)
	{
		patch_func_ptr((void*)address, ptr);
	}

	void patch_detour(void* address, void* new_func, void** old_func)
	{
		*old_func = *(void**)address;
		patch_func_ptr(address, new_func);
	}

	void patch_detour(uintptr_t address, void* new_func, void** old_func)
	{
		patch_detour((void*)address, new_func, old_func);
	}

	void patch_call(void* address, void* destination)
	{
		BYTE bytes[5];
		bytes[0] = 0xE8u;
		*(DWORD*)((DWORD)bytes + 1u) = (DWORD)destination - (DWORD)address - 5u;
		patch_bytes(address, bytes, 5u);
	}

	void patch_call(uintptr_t address, void* destination)
	{
		patch_call((void*)address, destination);
	}

	void patch_call(void* address, void* destination, size_t nops)
	{
		patch_call(address, destination);
		patch_nops(VPTRSUM(address, 5u), nops);
	}

	void patch_jmp(void* address, void* destination)
	{
		BYTE bytes[5];
		bytes[0] = 0xE9u;
		*(DWORD*)((DWORD)bytes + 1u) = (DWORD)destination - (DWORD)address - 5u;
		patch_bytes(address, bytes, 5u);
	}

	void patch_jmp(uintptr_t address, void* destination)
	{
		patch_jmp((void*)address, destination);
	}

	void patch_ret(void* address)
	{
		BYTE bytes = 0xC3u;
		patch_bytes(address, &bytes, 1u);
	}

	void patch_ret(uintptr_t address)
	{
		patch_ret((void*)address);
	}

	void patch_ret_nullptr(void* address)
	{
		patch_bytes(address, (BYTE*)"\x31\xC0", 2u);
		patch_ret(VPTRSUM(address, 2u));
	}

	void patch_ret_nullptr(void* address, size_t argc)
	{
		patch_bytes(address, (BYTE*)"\x83\xC4", 2u);
		patch_BYTE(VPTRSUM(address, 2u), (BYTE)(argc * 4u));
		patch_ret_nullptr(VPTRSUM(address, 3u));
	}

	void patch_ret(void* address, size_t argc)
	{
		BYTE bytes = 0xC2u;
		patch_bytes(address, &bytes, 1u);
		patch_WORD(VPTRSUM(address, 1u), (WORD)(4u * argc));
	}

	void patch_bp(void* address)
	{
		BYTE bytes = 0xCCu;
		patch_bytes(address, &bytes, 1u);
	}

	void patch_nops(void* address, size_t count)
	{
		BYTE* bytes = new BYTE[count];
		cmemset8(bytes, 0x90u, count);
		patch_bytes(address, bytes, count);
		delete[] bytes;
	}

	void patch_nops(uintptr_t address, size_t count)
	{
		patch_nops((void*)address, count);
	}

	void patch_nop_call(void* address)
	{
		patch_nops(address, 5u);
	}

	void patch_nop_call(uintptr_t address)
	{
		patch_nop_call((void*)address);
	}

	void detour_vtable(void* obj, size_t index, void* new_func, void** old_func)
	{
		void** vtable = *(void***)obj;
		*old_func = vtable[index];
		util::patch_func_ptr(&vtable[index], new_func);
	}

	void* get_IAT_address(BYTE* base, const char* dll_name, const char* search)
	{
		IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)base;
		IMAGE_NT_HEADERS* nt_headers = (IMAGE_NT_HEADERS*)(base + dos_header->e_lfanew);
		IMAGE_DATA_DIRECTORY section = nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
		IMAGE_IMPORT_DESCRIPTOR* import_descriptor = (IMAGE_IMPORT_DESCRIPTOR*)(base + section.VirtualAddress);
		for (size_t i = 0u; import_descriptor[i].Name; i++)
		{
			const char* loaded_dll = (const char*)(base + import_descriptor[i].Name);
			if (!cstricmp(loaded_dll, dll_name))
			{
				if (!import_descriptor[i].FirstThunk) { return nullptr; }
				IMAGE_THUNK_DATA* name_table = (IMAGE_THUNK_DATA*)(base + import_descriptor[i].OriginalFirstThunk);
				IMAGE_THUNK_DATA* import_table = (IMAGE_THUNK_DATA*)(base + import_descriptor[i].FirstThunk);
				while (name_table->u1.Ordinal)
				{
					if (!IMAGE_SNAP_BY_ORDINAL(name_table->u1.Ordinal))
					{
						IMAGE_IMPORT_BY_NAME* import_name = (IMAGE_IMPORT_BY_NAME*)(base + name_table->u1.ForwarderString);
						char* func_name = import_name->Name;
						if (!cstricmp(func_name, search))
						{
							return &import_table->u1.AddressOfData;
						}
					}
					name_table++;
					import_table++;
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
		if (FILE* file = fopen(name, "r")) { fclose(file); return true; }
		return false;
	}

	void create_console()
	{
		AllocConsole();
		FILE* f;
		f = freopen("CONIN$", "r", stdin);
		f = freopen("CONOUT$", "w", stdout);
		f = freopen("CONOUT$", "w", stderr);
	}

}
