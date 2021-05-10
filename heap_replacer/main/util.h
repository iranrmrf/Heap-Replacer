#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdlib>
#include <stdio.h>

#include <intrin.h>

#include "types.h"
#include "MinHook.h"

#define TFPARAM(...) void* self, void* _, __VA_ARGS__

#define BIT_SET(w, m, f) w = ((w & ~(m)) | (-(f) & (m)))
#define BIT_ON(w, m) BIT_SET(w, m, true)
#define BIT_OFF(w, m) BIT_SET(w, m, false)

#define UPTRSUM(x, y) ((uintptr_t)(x) + (uintptr_t)(y))
#define UPTRDIFF(x, y) ((uintptr_t)(x) - (uintptr_t)(y))

#define VPTRSUM(x, y) (void*)UPTRSUM((x), (y))
#define VPTRDIFF(x, y) (void*)UPTRDIFF((x), (y))

#define DEBUG_BREAK __asm { int 3 }
#define NOINLINE __declspec(noinline)

template<size_t N, typename T>
constexpr size_t countof(T(&)[N]) { return N; }

constexpr size_t highest_bit(size_t v) { return ((v / ((v % 255u) + 1u) / 255u) % 255u) * 8u - 86u / ((v % 255u) + 12u) + 7u; }
constexpr size_t bit_index(size_t v) { return highest_bit(v - 1u); }

constexpr size_t KB = 1024u * 1u;
constexpr size_t MB = 1024u * KB;
constexpr size_t GB = 1024u * MB;

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long int ulong;
typedef unsigned long long int ullong;

void* __cdecl operator new(size_t size);
void* __cdecl operator new[](size_t size);
void __cdecl operator delete(void* address);
void __cdecl operator delete[](void* address);

namespace util
{

	template <typename O, typename I>
	O force_cast(I in) { union { I in; O out; } u = { in }; return u.out; };

	size_t get_highest_bit(size_t n);
	size_t round_pow2(size_t n);

	void* winapi_alloc(size_t size);
	void* winapi_malloc(size_t size);
	void* winapi_calloc(size_t count, size_t size);
	void winapi_free(void* address);

	char ctolower(char c);
	int cstrcmp(const char* s1, const char* s2);
	int cstricmp(const char* s1, const char* s2);
	void cmemcpy(void* dst, const void* src, size_t cnt);

	void cmemset8(void* dst, BYTE val, size_t cnt);
	void cmemset16(void* dst, WORD val, size_t cnt);
	void cmemset32(void* dst, DWORD val, size_t cnt);

	void patch_bytes(void* address, BYTE* data, DWORD size);
	void patch_bytes(uintptr_t address, BYTE* data, DWORD size);
	void patch_BYTE(void* address, BYTE data);
	void patch_WORD(void* address, WORD data);
	void patch_DWORD(void* address, DWORD data);
	void patch_func_ptr(void* address, void* ptr);
	void patch_func_ptr(uintptr_t address, void* ptr);
	void patch_detour(void* address, void* new_func, void** old_func);
	void patch_detour(uintptr_t address, void* new_func, void** old_func);

	void patch_call(void* address, void* destination);
	void patch_call(void* address, void* destination, size_t nops);
	void patch_jmp(void* address, void* destination);
	void patch_jmp(uintptr_t address, void* destination);
	void patch_ret(void* address);
	void patch_ret(uintptr_t address);
	void patch_ret_nullptr(void* address);
	void patch_ret_nullptr(void* address, size_t argc);
	void patch_ret(void* address, size_t argc);
	void patch_bp(void* address);

	void patch_nops(void* address, size_t count);
	void patch_nops(uintptr_t address, size_t count);
	void patch_nop_call(void* address);
	void patch_nop_call(uintptr_t address);

	void detour_vtable(void* obj, size_t index, void* new_func, void** old_func);

	template <size_t A>
	size_t align(size_t size) { return (size + A - 1u) & ~(A - 1u); }

	template <size_t A>
	void* align(void* address) { return (void*)align<A>((uintptr_t)address); }

	// for primitives
	template <typename T>
	void swap(T& a, T& b) { T t = a; a = b; b = t; }

	void* get_IAT_address(BYTE* base, const char* dll_name, const char* search);
	bool is_LAA(BYTE* base);
	bool file_exists(const char* name);
	void create_console();

}
