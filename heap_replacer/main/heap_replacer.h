#pragma once

#include "definitions.h"
#include "types.h"
#include "util.h"

#include "initial_allocator/initial_allocator.h"

#include "memory_pools/memory_pool_manager.h"
#include "default_heap/default_heap_manager.h"

#include "scrap_heap/scrap_heap.h"

#include "locks/reentrant_lock.h"
#include "locks/nonreentrant_lock.h"

#include "ui/ui.h"

namespace hr
{

	initial_allocator ina(INITIAL_ALLOCATOR_SIZE);

	memory_pool_manager* mpm;
	default_heap_manager* dhm;
#ifdef HR_USE_GUI
	ui* uim;
#endif

	memory_pool_manager* get_mpm() { return mpm; }
	default_heap_manager* get_dhm() { return dhm; }
#ifdef HR_USE_GUI
	ui* get_uim() { return uim; }
#endif

	void* hr_malloc(size_t size)
	{
		if (size <= pool_max_size) [[likely]] { if (void* address = mpm->malloc(size)) [[likely]] { return address; } }
		if (size >= default_heap_block_size) [[unlikely]] { return util::winapi_malloc(size); }
		if (void* address = dhm->malloc(size)) [[likely]] { return address; }
		return util::winapi_malloc(size);
	}

	void* hr_calloc(size_t count, size_t size)
	{
		size *= count;
		if (size <= pool_max_size) [[likely]] { if (void* address = mpm->calloc(size)) [[likely]] { return address; } }
		if (size >= default_heap_block_size) [[unlikely]] { return util::winapi_malloc(size); }
		if (void* address = dhm->calloc(size)) [[likely]] { return address; }
		return util::winapi_malloc(size);
	}

	void* hr_realloc(void* address, size_t size)
	{
		if (!address) [[unlikely]] { return hr_malloc(size); }
		size_t old_size, new_size;
		if ((old_size = hr_mem_size(address)) >= (new_size = size)) { return address; }
		void* new_address = hr_malloc(size);
		util::cmemcpy(new_address, address, new_size < old_size ? new_size : old_size);
		hr_free(address);
		return new_address;
	}

	void* hr_recalloc(void* address, size_t count, size_t size)
	{
		if (!address) [[unlikely]] { return hr_calloc(count, size); }
		size_t old_size, new_size;;
		if ((old_size = hr_mem_size(address)) >= (new_size = size * count))
		{
			util::cmemset8(address, 0u, new_size);
			return address;
		}
		void* new_address = hr_calloc(count, size);
		hr_free(address);
		return new_address;
	}

	size_t hr_mem_size(void* address)
	{
		if (!address) [[unlikely]] { return 0u; }
		if (size_t size = mpm->mem_size(address)) [[likely]] { return size; }
		if (size_t size = dhm->mem_size(address)) [[likely]] { return size; }
		return 0u;
	}

	void hr_free(void* address)
	{
		if (!address) [[unlikely]] { return; }
		if (mpm->free(address)) [[likely]] { return; }
		if (dhm->free(address)) [[likely]] { return; }
		util::winapi_free(address);
	}

	void* game_heap_allocate(TFPARAM(size_t size))
	{
		return hr_malloc(size);
	}

	void* game_heap_reallocate(TFPARAM(void* address, size_t size))
	{
		return hr_realloc(address, size);
	}

	size_t game_heap_msize(TFPARAM(void* address))
	{
		return hr_mem_size(address);
	}

	void game_heap_free(TFPARAM(void* address))
	{
		hr_free(address);
	}

	void* __cdecl crt_malloc(size_t size)
	{
		return hr_malloc(size);
	}

	void* __cdecl crt_calloc(size_t count, size_t size)
	{
		return hr_calloc(count, size);
	}

	void* __cdecl crt_realloc(void* address, size_t size)
	{
		return hr_realloc(address, size);
	}

	void* __cdecl crt_recalloc(void* address, size_t count, size_t size)
	{
		return hr_recalloc(address, count, size);
	}

	size_t __cdecl crt_msize(void* address)
	{
		return hr_mem_size(address);
	}

	void __cdecl crt_free(void* address)
	{
		hr_free(address);
	}

	void* hr_ina_malloc(size_t size)
	{
		return ina.malloc(size);
	}

	void* hr_ina_calloc(size_t count, size_t size)
	{
		return ina.calloc(count, size);
	}

	size_t hr_ina_mem_size(void* address)
	{
		return ina.mem_size(address);
	}

	void hr_ina_free(void* address)
	{
		ina.free(address);
	}

	void apply_heap_hooks()
	{

		mpm = new memory_pool_manager();
		dhm = new default_heap_manager();

#if defined(FNV)

		util::patch_jmp(0x00ECD1C7, &crt_malloc);
		util::patch_jmp(0x00ED0CDF, &crt_malloc);
		util::patch_jmp(0x00EDDD7D, &crt_calloc);
		util::patch_jmp(0x00ED0D24, &crt_calloc);
		util::patch_jmp(0x00ECCF5D, &crt_realloc);
		util::patch_jmp(0x00ED0D70, &crt_realloc);
		util::patch_jmp(0x00EE1700, &crt_recalloc);
		util::patch_jmp(0x00ED0DBE, &crt_recalloc);
		util::patch_jmp(0x00ECD31F, &crt_msize);
		util::patch_jmp(0x00ECD291, &crt_free);

		util::patch_jmp(0x00AA3E40, &game_heap_allocate);
		util::patch_jmp(0x00AA4150, &game_heap_reallocate);
		util::patch_jmp(0x00AA4200, &game_heap_reallocate);
		util::patch_jmp(0x00AA44C0, &game_heap_msize);
		util::patch_jmp(0x00AA4060, &game_heap_free);

		util::patch_ret(0x00AA6840);
		util::patch_ret(0x00866E00);
		util::patch_ret(0x00866770);

		util::patch_ret(0x00AA6F90);
		util::patch_ret(0x00AA7030);
		util::patch_ret(0x00AA7290);
		util::patch_ret(0x00AA7300);

		util::patch_jmp(0x0040FBF0, util::force_cast<void*>(&reentrant_lock::lock_game));
		util::patch_jmp(0x0040FBA0, util::force_cast<void*>(&reentrant_lock::unlock));

		util::patch_ret(0x00AA58D0);
		util::patch_ret(0x00866D10);
		util::patch_ret(0x00AA5C80);

		util::patch_jmp(0x00AA53F0, util::force_cast<void*>(&scrap_heap::init_0x10000));
		util::patch_jmp(0x00AA5410, util::force_cast<void*>(&scrap_heap::init_var));
		util::patch_jmp(0x00AA54A0, util::force_cast<void*>(&scrap_heap::alloc));
		util::patch_jmp(0x00AA5610, util::force_cast<void*>(&scrap_heap::free));
		util::patch_jmp(0x00AA5460, util::force_cast<void*>(&scrap_heap::purge));

		util::patch_nops(0x00AA38CA, 0x00AA38E8 - 0x00AA38CA);
		util::patch_jmp(0x00AA42E0, util::force_cast<void*>(&scrap_heap::get_thread_scrap_heap));

		util::patch_nop_call(0x00AA3060);

		util::patch_nop_call(0x0086C56F);
		util::patch_nop_call(0x00C42EB1);
		util::patch_nop_call(0x00EC1701);

		util::patch_bytes(0x0086EED4, (BYTE*)"\xEB\x55", 2);

#elif defined(FO3)

		util::patch_jmp(0x00C063F5, &crt_malloc);
		util::patch_jmp(0x00C0AB3F, &crt_malloc);
		util::patch_jmp(0x00C1843C, &crt_calloc);
		util::patch_jmp(0x00C0AB7F, &crt_calloc);
		util::patch_jmp(0x00C06546, &crt_realloc);
		util::patch_jmp(0x00C0ABC7, &crt_realloc);
		util::patch_jmp(0x00C06761, &crt_recalloc);
		util::patch_jmp(0x00C0AC12, &crt_recalloc);
		util::patch_jmp(0x00C067DA, &crt_msize);
		util::patch_jmp(0x00C064B8, &crt_free);

		util::patch_jmp(0x0086B930, &game_heap_allocate);
		util::patch_jmp(0x0086BAE0, &game_heap_reallocate);
		util::patch_jmp(0x0086BB50, &game_heap_reallocate);
		util::patch_jmp(0x0086B8C0, &game_heap_msize);
		util::patch_jmp(0x0086BA60, &game_heap_free);

		util::patch_ret(0x0086D670);
		util::patch_ret(0x006E21F0);
		util::patch_ret(0x006E1E10);

		util::patch_jmp(0x00409A80, util::force_cast<void*>(&reentrant_lock::lock_game));
		//util::patch_jmp(0x00000000, util::force_cast<void*>(&reentrant_lock::unlock));

		util::patch_ret(0x0086C600);
		util::patch_ret(0x006E1CD0);
		util::patch_ret(0x0086CA30);

		util::patch_jmp(0x0086CB70, util::force_cast<void*>(&scrap_heap::init_0x10000));
		util::patch_jmp(0x0086CB90, util::force_cast<void*>(&scrap_heap::init_var));
		util::patch_jmp(0x0086C710, util::force_cast<void*>(&scrap_heap::alloc));
		util::patch_jmp(0x0086C7B0, util::force_cast<void*>(&scrap_heap::free));
		util::patch_jmp(0x0086CAA0, util::force_cast<void*>(&scrap_heap::purge));

		util::patch_nops(0x0086C038, 0x0086C086 - 0x0086C038);
		util::patch_jmp(0x0086BCB0, util::force_cast<void*>(&scrap_heap::get_thread_scrap_heap));

		util::patch_nop_call(0x006E9B30);
		util::patch_nop_call(0x007FACDB);
		util::patch_nop_call(0x00AA6534);

#endif

		HR_PRINTF("Hooks applied.");

	}

#ifdef HR_USE_GUI

	void apply_imgui_hooks()
	{
		
		uim = new ui();

#if defined(FNV)
		
		util::patch_bytes(0x004DA942, (PBYTE)"\xB8\x00\x00\x00\x00", 5);
		util::patch_bytes(0x004DA937, (PBYTE)"\xB8\x00\x00\x00\x00", 5);

		util::patch_detour(0x00FDF2B8, &ui::create_window_hook, (void**)&ui::create_window);
		util::patch_detour(0x00FDF294, &ui::dispatch_message_hook, (void**)&ui::dispatch_message);
		util::patch_detour(0x010EE640, &ui::display_scene_hook, (void**)&ui::display_scene);

#elif defined(FO3)

		util::patch_bytes(0x004905C7, (PBYTE)"\xB8\x00\x00\x00\x00", 5);
		util::patch_bytes(0x004905B7, (PBYTE)"\xBA\x00\x00\x00\x00\x90", 6);

		util::patch_detour(0x00D9B2CC, &ui::create_window_hook, (void**)&ui::create_window);
		util::patch_detour(0x00D9B284, &ui::dispatch_message_hook, (void**)&ui::dispatch_message);
		util::patch_detour(0x00E29188, &ui::display_scene_hook, (void**)&ui::display_scene);

#endif

	}

#endif

}
