#pragma once

#include "main/util.h"

#include "light_critical_section.h"
#include "scrap_heap_vector.h"

#define SHM_BUFFER_COUNT	64u

#define SH_BUFFER_MAX_SIZE	0x00400000u
#define SH_BUFFER_MIN_SIZE	0x00040000u

#define SH_FREE_FLAG		0x80000000u

namespace ScrapHeap
{

	struct scrap_heap_buffer { void* addr; size_t size;	};

	struct scrap_heap_chunk { size_t size; scrap_heap_chunk* prev_chunk; };

	struct scrap_heap_manager
	{
		scrap_heap_buffer buffers[SHM_BUFFER_COUNT];
		size_t free_buffer_count;
		light_critical_section critical_section;
	};

	struct scrap_heap
	{
		void* commit_bgn;
		void* unused;
		void* commit_end;
		scrap_heap_chunk* last_chunk;
	};

	scrap_heap_manager* shm;

	scrap_heap_vector* mt_sh_vector;

	void __fastcall shm_ctor(TFPARAM(scrap_heap_manager* self))
	{
		shm = new scrap_heap_manager();
		Util::Mem::memset8(shm->buffers, NULL, SHM_BUFFER_COUNT * sizeof(scrap_heap_buffer));
		shm->free_buffer_count = 0;
		shm->critical_section.thread_id = 0;
		shm->critical_section.lock_count = 0;
		mt_sh_vector = new scrap_heap_vector(16);
	}

	void __fastcall shm_swap_buffers(TFPARAM(scrap_heap_manager* self, size_t index))
	{
		if (index < --self->free_buffer_count) [[likely]]
		{
			self->buffers[index].addr = self->buffers[self->free_buffer_count].addr;
			self->buffers[index].size = self->buffers[self->free_buffer_count].size;
		}
	}

	void* __fastcall shm_create_buffer(TFPARAM(scrap_heap_manager* self, size_t size))
	{
		void* address = VirtualAlloc(nullptr, SH_BUFFER_MAX_SIZE, MEM_RESERVE, PAGE_READWRITE);
		VirtualAlloc(address, size, MEM_COMMIT, PAGE_READWRITE);
		return address;
	}

	void* __fastcall shm_request_buffer(TFPARAM(scrap_heap_manager* self, size_t* size))
	{
		if (!self->free_buffer_count) { return shm_create_buffer(TFCALL(self, *size)); }
		enter_light_critical_section(TFCALL(&self->critical_section, nullptr));
		if (self->free_buffer_count)
		{
			size_t max_index = 0;
			size_t max_size = 0;
			for (size_t i = 0; i < self->free_buffer_count; i++)
			{
				if (self->buffers[i].size >= *size)
				{
					*size = self->buffers[i].size;
					void* address = self->buffers[i].addr;
					shm_swap_buffers(TFCALL(self, i));
					leave_light_critical_section(TFCALL(&self->critical_section));
					return address;
				}
				if (self->buffers[i].size > max_size)
				{
					max_size = self->buffers[i].size;
					max_index = i;
				}
			}
			shm_swap_buffers(TFCALL(self, max_index));
			VirtualAlloc(VPTRSUM(self->buffers[max_index].addr, max_size), *size - max_size, MEM_COMMIT, PAGE_READWRITE);
			leave_light_critical_section(TFCALL(&self->critical_section));
			return self->buffers[max_index].addr;
		}
		void* address = shm_create_buffer(TFCALL(self, *size));
		leave_light_critical_section(TFCALL(&self->critical_section));
		return address;
	}

	void __fastcall shm_free_buffer(TFPARAM(scrap_heap_manager* self, void* address, size_t size))
	{
		VirtualFree(address, NULL, MEM_RELEASE);
	}

	void __fastcall shm_release_buffer(TFPARAM(scrap_heap_manager* self, void* address, size_t size))
	{
		if (self->free_buffer_count >= SHM_BUFFER_COUNT) { shm_free_buffer(TFCALL(self, address, size)); }
		enter_light_critical_section(TFCALL(&self->critical_section, nullptr));
		if (self->free_buffer_count < SHM_BUFFER_COUNT)
		{
			self->buffers[self->free_buffer_count].addr = address;
			self->buffers[self->free_buffer_count++].size = size;
		}
		else
		{
			shm_free_buffer(TFCALL(self, address, size));
		}
		leave_light_critical_section(TFCALL(&self->critical_section));
	}

	void __fastcall sh_init(TFPARAM(scrap_heap* self, size_t size))
	{
		self->commit_bgn = shm_request_buffer(TFCALL(shm, &size));
		self->unused = self->commit_bgn;
		self->commit_end = VPTRSUM(self->commit_bgn, size);
		self->last_chunk = nullptr;
	}

	void __fastcall sh_init_0x10000(TFPARAM(scrap_heap* self))
	{
		sh_init(TFCALL(self, SH_BUFFER_MIN_SIZE));
	}

	void __fastcall sh_init_var(TFPARAM(scrap_heap* self, size_t size))
	{
		sh_init(TFCALL(self, Util::align(size, SH_BUFFER_MIN_SIZE)));
	}

	void* __fastcall sh_alloc(TFPARAM(scrap_heap* self, size_t size, size_t alignment))
	{
		uintptr_t body = Util::align(UPTRSUM(self->unused, sizeof(scrap_heap_chunk)), alignment);
		void* desired_end = VPTRSUM(body, size);
		if (desired_end > self->commit_end) [[unlikely]]
		{
			size_t old_size = UPTRDIFF(self->commit_end, self->commit_bgn);
			size_t new_size = old_size << 2;
			while (new_size < UPTRDIFF(desired_end, self->commit_bgn)) [[unlikely]]
			{
				new_size <<= 2;
				if (new_size > SH_BUFFER_MAX_SIZE) [[unlikely]]
				{
					HR_MSGBOX("Scrap heap failed to grow!");
					return nullptr;
				}
			}
			VirtualAlloc(self->commit_end, new_size - old_size, MEM_COMMIT, PAGE_READWRITE);
			self->commit_end = VPTRSUM(self->commit_bgn, new_size);
		}
		scrap_heap_chunk* header = (scrap_heap_chunk*)UPTRDIFF(body, sizeof(scrap_heap_chunk));
		header->size = size;
		header->prev_chunk = self->last_chunk;
		self->last_chunk = header;
		self->unused = desired_end;
		return (void*)body;
	}

	void __fastcall sh_free(TFPARAM(scrap_heap* self, void* address))
	{
		scrap_heap_chunk* chunk = (scrap_heap_chunk*)UPTRDIFF(address, sizeof(scrap_heap_chunk));
		if (address && !(chunk->size & SH_FREE_FLAG)) [[likely]]
		{
			for (chunk->size |= SH_FREE_FLAG; self->last_chunk && (self->last_chunk->size & SH_FREE_FLAG); self->last_chunk = self->last_chunk->prev_chunk);
			self->unused = self->last_chunk ? (BYTE*)self->last_chunk + sizeof(scrap_heap_chunk) + self->last_chunk->size : self->commit_bgn;
			size_t old_size = UPTRDIFF(self->commit_end, self->commit_bgn);
			size_t new_size = old_size >> 2;
			if ((VPTRSUM(self->commit_bgn, new_size) >= self->unused) & (new_size >= SH_BUFFER_MIN_SIZE)) [[unlikely]]
			{
				VirtualFree(VPTRDIFF(self->commit_end, old_size - new_size), old_size - new_size, MEM_DECOMMIT);
				self->commit_end = VPTRSUM(self->commit_bgn, new_size);
			}
		}
	}

	void __fastcall sh_purge(TFPARAM(scrap_heap* self))
	{
		shm_release_buffer(TFCALL(shm, self->commit_bgn, UPTRDIFF(self->commit_end, self->commit_bgn)));
		self->commit_bgn = nullptr;
		self->unused = nullptr;
		self->commit_end = nullptr;
		self->last_chunk = nullptr;
	}

	scrap_heap* __fastcall get_thread_scrap_heap(TFPARAM(void* self))
	{
		DWORD id = GetCurrentThreadId();
		scrap_heap* sh;
		if (sh = mt_sh_vector->find(id)) [[likely]] { return sh; }
		sh = (scrap_heap*)NVHR::nvhr_malloc(sizeof(scrap_heap));
		sh_init_0x10000(TFCALL(sh));
		mt_sh_vector->insert(id, sh);
		return sh;
	}

}
