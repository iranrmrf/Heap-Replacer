#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>

// ONE OF THESE NEED TO BE DEFINED!
#if defined(FNV)
	#define HR_NAME "NVHR"
	#define HR_VERSION "1.4.0.525"
	#define HR_GAME_QPC_HOOK (void*)(0x00FDF0A0)
	#define HR_GECK_QPC_HOOK (void*)(0x00D2320C)
#elif defined(FO3)
	#define HR_NAME "F3HR"
	#define HR_VERSION "1.7.0.3"
	#define HR_GAME_QPC_HOOK (void*)(0x00D9B0E4)
	#define HR_GECK_QPC_HOOK (void*)(0x00D03208)
#endif

//#define HR_ZERO_MEM

#define HR_MSGBOX(msg) MessageBox(NULL, HR_NAME " - " msg, "Error", NULL)
#define HR_PRINTF(msg) printf(HR_NAME " - " msg "\n")

#define TFPARAM(self, ...) self, void* _, __VA_ARGS__
#define TFCALL(self, ...) self, nullptr, __VA_ARGS__

#define LOCK(v) while (InterlockedCompareExchange((v), 1u, 0u)) { }
#define UNLOCK(v) *(v) = 0u

#define BITWISE_NULLIFIER(c, v) ((DWORD)(v) & ((!(c)) - 1))
#define BITWISE_IF_ELSE_NULL(c, t, f) ((BITWISE_NULLIFIER((c), (t))) + (BITWISE_NULLIFIER((!(c)), (f))))

#define UPTRSUM(x, y) ((uintptr_t)(x) + (uintptr_t)(y))
#define UPTRDIFF(x, y) ((uintptr_t)(x) - (uintptr_t)(y))

#define VPTRSUM(x, y) (void*)UPTRSUM((x), (y))
#define VPTRDIFF(x, y) (void*)UPTRDIFF((x), (y))

#define DEBUG_BREAK __asm { int 3 }
#define NOINLINE __declspec(noinline)

constexpr size_t highest_bit(size_t v) { return ((v / ((v % 255) + 1) / 255) % 255) * 8 - 86 / ((v % 255) + 12) + 7; }
constexpr size_t bit_index(size_t v) { return highest_bit(v - 1); }

constexpr size_t KB = 1024u * 1u;
constexpr size_t MB = 1024u * KB;
constexpr size_t GB = 1024u * MB;

namespace nvhr
{

	void* __fastcall nvhr_ina_malloc(size_t size);
	void* __fastcall nvhr_ina_calloc(size_t count, size_t size);
	size_t __fastcall nvhr_ina_mem_size(void* address);
	void __fastcall nvhr_ina_free(void* address);
	void* __fastcall nvhr_malloc(size_t size);
	void* __fastcall nvhr_calloc(size_t count, size_t size);
	void* __fastcall nvhr_realloc(void* address, size_t size);
	void* __fastcall nvhr_recalloc(void* address, size_t count, size_t size);
	size_t __fastcall nvhr_mem_size(void* address);
	void __fastcall nvhr_free(void* address);

}

void* __cdecl operator new(size_t size);
void* __cdecl operator new[](size_t size);
void __cdecl operator delete(void* address);
void __cdecl operator delete[](void* address);

namespace util
{

	template <typename O, typename I>
	O force_cast(I in) { union { I in; O out; } u = { in }; return u.out; };

	size_t get_highest_bit(DWORD value);
	size_t round_pow2(size_t value);

	void* winapi_alloc(size_t size);
	void* winapi_malloc(size_t size);
	void* winapi_calloc(size_t count, size_t size);
	void winapi_free(void* address);

	void memset8(void* destination, BYTE value, size_t count);
	void memset16(void* destination, WORD value, size_t count);
	void memset32(void* destination, DWORD value, size_t count);

	void patch_bytes(uintptr_t address, BYTE* data, DWORD size);
	void patch_BYTE(uintptr_t address, BYTE data);
	void patch_WORD(uintptr_t address, WORD data);
	void patch_DWORD(uintptr_t address, DWORD data);

	void patch_call(uintptr_t address, void* destination);
	void patch_jmp(uintptr_t address, void* destination);
	void patch_ret(uintptr_t address);
	void patch_ret_nullptr(uintptr_t address);
	void patch_ret_nullptr(uintptr_t address, size_t argc);
	void patch_ret(uintptr_t address, size_t argc);
	void patch_bp(uintptr_t address);

	void patch_nops(uintptr_t address, size_t count);
	void patch_nop_call(uintptr_t address);

	template <size_t A>
	size_t align(size_t size) { DWORD s = (size + (A - 1)) & ~(A - 1); return BITWISE_IF_ELSE_NULL(size & (A - 1), s, size); }

	template <size_t A>
	void* align(void* address) { return (void*)align<A>((uintptr_t)address); }

	void* get_IAT_address(BYTE* base, const char* dll_name, const char* search);
	bool is_LAA(BYTE* base);
	bool file_exists(const char* name);
	void create_console();

}
