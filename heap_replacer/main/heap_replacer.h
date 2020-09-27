#pragma once

#include "util.h"

#include "initial_allocator/inital_allocator.h"

#include "default_heap/default_heap_manager.h"
#include "memory_pools/memory_pool_manager.h"
#include "scrap_heap/scrap_heap_manager.h"

namespace NVHR
{

	memory_pool_manager* mpm;
	default_heap_manager* dhm;

	void* __fastcall nvhr_malloc(size_t size)
	{
		if (size < 4) { size = 4; }
		if (size <= 2048)
		{
			if (void* address = mpm->malloc(size)) [[likely]] { return address; }
		}
		else
		{
			if (void* address = dhm->malloc(size)) [[likely]] { return address; }
		}
		MessageBox(NULL, "NVHR - nullptr malloc!", "Error", NULL);
		return nullptr;
	}

	void* __fastcall nvhr_calloc(size_t count, size_t size)
	{
		size *= count;
		if (size < 4) { size = 4; }
		if (size <= 2048)
		{
			if (void* address = mpm->calloc(size)) [[likely]] { return address; }
		}
		else
		{
			if (void* address = dhm->calloc(size)) [[likely]] { return address; }
		}
		MessageBox(NULL, "NVHR - nullptr calloc!", "Error", NULL);
		return nullptr;
	}

	void* __fastcall nvhr_realloc(void* address, size_t size)
	{
		if (address == nullptr) [[unlikely]] { return nvhr_malloc(size); }
		size_t old_size = nvhr_mem_size(address);
		if (old_size >= size) { return address; }
		void* new_address = nvhr_malloc(size);
		memcpy(new_address, address, size < old_size ? size : old_size);
		nvhr_free(address);
		return new_address;
	}

	size_t __fastcall nvhr_mem_size(void* address)
	{
		size_t size;
		if (size = mpm->mem_size(address)) { return size; }
		return dhm->mem_size(address);
	}

	void __fastcall nvhr_free(void* address)
	{
		if (mpm->free(address)) { return; }
		dhm->free(address);
	}

	void* __fastcall game_heap_allocate(TFPARAM(void* self, size_t size))
	{
		return nvhr_malloc(size);
	}

	void* __fastcall game_heap_reallocate(TFPARAM(void* self, void* address, size_t size))
	{
		return nvhr_realloc(address, size);
	}

	void __fastcall game_heap_free(TFPARAM(void* self, void* address))
	{
		nvhr_free(address);
	}

	void* __cdecl crt_malloc(size_t size)
	{
		return nvhr_malloc(size);
	}

	void* __cdecl crt_calloc(size_t count, size_t size)
	{
		return nvhr_calloc(count, size);
	}

	void* __cdecl crt_realloc(void* address, size_t size)
	{
		return nvhr_realloc(address, size);
	}

	void __cdecl crt_free(void* address)
	{
		nvhr_free(address);
	}

	size_t __cdecl crt_msize(void* address)
	{
		return nvhr_mem_size(address);
	}

	/*void patch_old_NVSE()
	{
		HMODULE base = GetModuleHandleA("nvse_1_4.dll");
		if (!base) { return; }
		printf("NVHR - Found NVSE at %p\n", base);
		BYTE* address;
		address = (BYTE*)((DWORD)base + 0x38887);
		if (*address == 0x4)
		{
			printf("NVHR - Patched stable NVSE\n");
			patch_BYTE((DWORD)address, 0x8);
			return;
		}
		address = (BYTE*)((DWORD)base + 0x24FF7);
		if (*address == 0x4)
		{
			printf("NVHR - Patched unsupported releasefast NVSE\n");
			patch_BYTE((DWORD)address, 0x8);
			return;
		}
	}*/

	void apply_heap_hooks()
	{

		mpm = new memory_pool_manager();
		dhm = new default_heap_manager();


		Util::Mem::patch_jmp(0xECD1C7, &crt_malloc);
		Util::Mem::patch_jmp(0xED0CDF, &crt_malloc);
		Util::Mem::patch_jmp(0xEDDD7D, &crt_calloc);
		Util::Mem::patch_jmp(0xED0D24, &crt_calloc);
		Util::Mem::patch_jmp(0xECCF5D, &crt_realloc);
		Util::Mem::patch_jmp(0xED0D70, &crt_realloc);
		Util::Mem::patch_jmp(0xECD291, &crt_free);
		Util::Mem::patch_jmp(0xECD31F, &crt_msize);


		Util::Mem::patch_jmp(0xAA3E40, &game_heap_allocate);
		Util::Mem::patch_jmp(0xAA4150, &game_heap_reallocate);
		Util::Mem::patch_jmp(0xAA4060, &game_heap_free);

		Util::Mem::patch_ret(0x866E00);
		Util::Mem::patch_BYTE(0xAA39D9, 0);

		Util::Mem::patch_ret(0x866770);
		Util::Mem::patch_ret(0xAA7030);
		Util::Mem::patch_ret(0xAA7290);


		Util::Mem::patch_jmp(0x40FBF0, &enter_light_critical_section);
		Util::Mem::patch_jmp(0x40FBA0, &leave_light_critical_section);


		Util::Mem::patch_jmp(0xAA47E0, &ScrapHeap::shm_create_mt);
		Util::Mem::patch_jmp(0xAA42E0, &ScrapHeap::shm_get_scrap_heap);

		Util::Mem::patch_jmp(0x866D10, &ScrapHeap::shm_get_singleton);

		Util::Mem::patch_jmp(0xAA5860, &ScrapHeap::shm_ctor);
		Util::Mem::patch_jmp(0xAA58D0, &ScrapHeap::shm_init);

		Util::Mem::patch_jmp(0xAA5F50, &ScrapHeap::shm_swap_buffers);
		Util::Mem::patch_jmp(0xAA5EC0, &ScrapHeap::shm_create_buffer);
		Util::Mem::patch_jmp(0xAA59B0, &ScrapHeap::shm_request_buffer);
		Util::Mem::patch_jmp(0xAA5F30, &ScrapHeap::shm_free_buffer);
		Util::Mem::patch_jmp(0xAA5B70, &ScrapHeap::shm_release_buffer);
		Util::Mem::patch_jmp(0xAA5C80, &ScrapHeap::shm_free_all_buffers);


		Util::Mem::patch_jmp(0xAA57B0, &ScrapHeap::sh_init);
		Util::Mem::patch_jmp(0xAA53F0, &ScrapHeap::sh_init_0x10000);
		Util::Mem::patch_jmp(0xAA5410, &ScrapHeap::sh_init_var);

		Util::Mem::patch_jmp(0xAA5E30, &ScrapHeap::sh_grow);
		Util::Mem::patch_jmp(0xAA5E90, &ScrapHeap::sh_shrink);

		Util::Mem::patch_jmp(0xAA54A0, &ScrapHeap::sh_add_chunk);
		Util::Mem::patch_jmp(0xAA5610, &ScrapHeap::sh_remove_chunk);


		Util::Mem::patch_nops(0xAA3060, 5);

		Util::Mem::patch_bytes(0xC42EA4, (BYTE*)"\xEB", 1);
		Util::Mem::patch_bytes(0x86C563, (BYTE*)"\xEB\x12", 2);
		Util::Mem::patch_bytes(0xEC16F8, (BYTE*)"\xEB\x0F", 2);


		/*patch_call(0x86CF64, &patch_old_NVSE);
		patch_nops(0x86CF69, 2);*/

		printf("NVHR - Hooks applied.\n");

	}

}
