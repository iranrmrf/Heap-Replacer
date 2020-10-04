#pragma once

#define FNV

#ifdef FNV
	#define HR_NAME "NVHR"
	#define HR_VERSION "1.4.0.525"
#endif

#ifdef FO3
	#define HR_NAME "F3HR"
	#define HR_VERSION "1.7.0.3"
#endif

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
		if (size < 4) [[unlikely]] { size = 4; }
		if (size <= 2048) [[likely]]
		{
			if (void* address = mpm->malloc(size)) [[likely]] { return address; }
		}
		else
		{
			if (void* address = dhm->malloc(size)) [[likely]] { return address; }
		}
		return nullptr;
	}

	void* __fastcall nvhr_calloc(size_t count, size_t size)
	{
		size *= count;
		if (size < 4) [[unlikely]] { size = 4; }
		if (size <= 2048) [[likely]]
		{
			if (void* address = mpm->calloc(size)) [[likely]] { return address; }
		}
		else
		{
			if (void* address = dhm->calloc(size)) [[likely]] { return address; }
		}
		return nullptr;
	}

	void* __fastcall nvhr_realloc(void* address, size_t size)
	{
		if (address == nullptr) [[unlikely]] { return nvhr_malloc(size); }
		size_t old_size = nvhr_mem_size(address);
		size_t new_size = size;
		if (old_size >= new_size) { return address; }
		void* new_address = nvhr_malloc(size);
		memcpy(new_address, address, new_size < old_size ? new_size : old_size);
		nvhr_free(address);
		return new_address;
	}

	void* __fastcall nvhr_recalloc(void* address, size_t count, size_t size)
	{
		if (address == nullptr) [[unlikely]] { return nvhr_calloc(count, size); }
		size_t old_size = nvhr_mem_size(address);
		size_t new_size = size * count;
		if (old_size >= new_size)
		{
			Util::Mem::memset8(address, NULL, new_size);
			return address;
		}
		void* new_address = nvhr_calloc(count, size);
		nvhr_free(address);
		return new_address;
	}

	size_t __fastcall nvhr_mem_size(void* address)
	{
		if (size_t size = mpm->mem_size(address)) [[likely]] { return size; }
		return dhm->mem_size(address);
	}

	void __fastcall nvhr_free(void* address)
	{
		if (mpm->free(address)) [[likely]] { return; }
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

	size_t __fastcall game_heap_msize(TFPARAM(void* self, void* address))
	{
		return nvhr_mem_size(address);
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

	void* __cdecl crt_recalloc(void* address, size_t count, size_t size)
	{
		return nvhr_recalloc(address, count, size);
	}

	void __cdecl crt_free(void* address)
	{
		nvhr_free(address);
	}

	size_t __cdecl crt_msize(void* address)
	{
		return nvhr_mem_size(address);
	}

	void apply_heap_hooks()
	{

		mpm = new memory_pool_manager();
		dhm = new default_heap_manager();

#ifdef FNV

		Util::Mem::patch_jmp(0xECD1C7, &crt_malloc);
		Util::Mem::patch_jmp(0xED0CDF, &crt_malloc);
		Util::Mem::patch_jmp(0xEDDD7D, &crt_calloc);
		Util::Mem::patch_jmp(0xED0D24, &crt_calloc);
		Util::Mem::patch_jmp(0xECCF5D, &crt_realloc);
		Util::Mem::patch_jmp(0xED0D70, &crt_realloc);
		Util::Mem::patch_jmp(0xEE1700, &crt_recalloc);
		Util::Mem::patch_jmp(0xED0DBE, &crt_recalloc);
		Util::Mem::patch_jmp(0xECD291, &crt_free);
		Util::Mem::patch_jmp(0xECD31F, &crt_msize);

		Util::Mem::patch_jmp(0xAA3E40, &game_heap_allocate);
		Util::Mem::patch_jmp(0xAA4150, &game_heap_reallocate);
		Util::Mem::patch_jmp(0xAA4200, &game_heap_reallocate);
		Util::Mem::patch_jmp(0xAA4060, &game_heap_free);
		Util::Mem::patch_jmp(0xAA44C0, &game_heap_msize);

		Util::Mem::patch_ret(0xAA6840);
		Util::Mem::patch_ret(0x866E00);
		Util::Mem::patch_ret(0x866770);

		Util::Mem::patch_ret(0xAA6F90);
		Util::Mem::patch_ret(0xAA7030);
		Util::Mem::patch_ret(0xAA7290);
		Util::Mem::patch_ret(0xAA7300);

		Util::Mem::patch_jmp(0x40FBF0, &enter_light_critical_section);
		Util::Mem::patch_jmp(0x40FBA0, &leave_light_critical_section);
		
		Util::Mem::patch_jmp(0xAA5860, &ScrapHeap::shm_ctor);
		Util::Mem::patch_jmp(0x866D10, &ScrapHeap::shm_get_singleton);
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
		Util::Mem::patch_jmp(0xAA5610, &ScrapHeap::sh_rmv_chunk);
		//Util::Mem::patch_jmp(0x0, &ScrapHeap::sh_release_buffer);

		Util::Mem::patch_nops(0xAA38CA, 0xAA38E8 - 0xAA38CA);
		Util::Mem::patch_jmp(0xAA42E0, &ScrapHeap::get_scrap_heap);

		Util::Mem::patch_nop_call(0xAA3060);

		Util::Mem::patch_nop_call(0x86C56F);
		Util::Mem::patch_nop_call(0xC42EB1);
		Util::Mem::patch_nop_call(0xEC1701);

#endif

#ifdef FO3

		Util::Mem::patch_jmp(0xC063F5, &crt_malloc);
		Util::Mem::patch_jmp(0xC0AB3F, &crt_malloc);
		Util::Mem::patch_jmp(0xC1843C, &crt_calloc);
		Util::Mem::patch_jmp(0xC0AB7F, &crt_calloc);
		Util::Mem::patch_jmp(0xC06546, &crt_realloc);
		Util::Mem::patch_jmp(0xC0ABC7, &crt_realloc);
		Util::Mem::patch_jmp(0xC06761, &crt_recalloc);
		Util::Mem::patch_jmp(0xC0AC12, &crt_recalloc);
		Util::Mem::patch_jmp(0xC064B8, &crt_free);
		Util::Mem::patch_jmp(0xC067DA, &crt_msize); 

		Util::Mem::patch_jmp(0x86B930, &game_heap_allocate);
		Util::Mem::patch_jmp(0x86BAE0, &game_heap_reallocate);
		Util::Mem::patch_jmp(0x86BB50, &game_heap_reallocate);
		Util::Mem::patch_jmp(0x86BA60, &game_heap_free);
		Util::Mem::patch_jmp(0x86B8C0, &game_heap_msize);

		Util::Mem::patch_ret(0x86D670);
		Util::Mem::patch_ret(0x6E21F0);
		Util::Mem::patch_ret(0x6E1E10);

		Util::Mem::patch_jmp(0x409A80, &enter_light_critical_section); 
		//Util::Mem::patch_jmp(0x0, &leave_light_critical_section);

		Util::Mem::patch_ret(0x86C5E0);

		Util::Mem::patch_jmp(0x86C600, &ScrapHeap::shm_ctor);
		Util::Mem::patch_jmp(0x6E1CD0, &ScrapHeap::shm_get_singleton);
		//Util::Mem::patch_jmp(0x0, &ScrapHeap::shm_swap_buffers);
		Util::Mem::patch_jmp(0x86C6A0, &ScrapHeap::shm_create_buffer);
		Util::Mem::patch_jmp(0x86C840, &ScrapHeap::shm_request_buffer);
		//Util::Mem::patch_jmp(0x0, &ScrapHeap::shm_free_buffer);
		Util::Mem::patch_jmp(0x86C9A0, &ScrapHeap::shm_release_buffer);
		Util::Mem::patch_jmp(0x86CA30, &ScrapHeap::shm_free_all_buffers);

		Util::Mem::patch_jmp(0x86CB00, &ScrapHeap::sh_init);
		Util::Mem::patch_jmp(0x86CB70, &ScrapHeap::sh_init_0x10000);
		Util::Mem::patch_jmp(0x86CB90, &ScrapHeap::sh_init_var);
		Util::Mem::patch_jmp(0x86C640, &ScrapHeap::sh_grow);
		//Util::Mem::patch_jmp(0x0, &ScrapHeap::sh_shrink);
		Util::Mem::patch_jmp(0x86C710, &ScrapHeap::sh_add_chunk);
		Util::Mem::patch_jmp(0x86C7B0, &ScrapHeap::sh_rmv_chunk);
		Util::Mem::patch_jmp(0x86CAA0, &ScrapHeap::sh_release_buffer);

		Util::Mem::patch_nops(0x86C038, 0x86C086 - 0x86C038);
		Util::Mem::patch_jmp(0x86BCB0, &ScrapHeap::get_scrap_heap);

		Util::Mem::patch_nop_call(0x6E9B30);
		Util::Mem::patch_nop_call(0x7FACDB);
		Util::Mem::patch_nop_call(0xAA6534);

#endif

		HR_PRINTF("Hooks applied.");

	}

}
