#pragma once

#include "main/util.h"

#include "sh_array.h"

#define SHM_BUFFER_COUNT	64

#define SH_BUFFER_MAX_SIZE	0x01000000
#define SH_BUFFER_MIN_SIZE	0x00040000

#define SH_FREE_FLAG		0x80000000

struct light_critical_section
{
	DWORD thread_id;
	size_t lock_count;
};

void __fastcall enter_light_critical_section(TFPARAM(light_critical_section* self, const char* name))
{
	DWORD id = GetCurrentThreadId();
	if (self->thread_id == id)
	{
		++self->lock_count;
	}
	else
	{
		size_t spin = 0;
		while (InterlockedCompareExchange(&self->thread_id, id, 0u))
		{
			Sleep(++spin > 10000u);
		}
		self->lock_count = 1;
	}
}

void __fastcall leave_light_critical_section(TFPARAM(light_critical_section* self))
{
	if (!--self->lock_count)
	{
		self->thread_id = 0;
	}
}

void __fastcall sh_grow(TFPARAM(void* self, void* address, size_t offset, size_t size))
{
	Util::try_valloc(VPTRSUM(address, offset), size, MEM_COMMIT, PAGE_READWRITE, 1);
}

void __fastcall sh_shrink(TFPARAM(void* self, void* address, size_t offset, size_t size))
{
	VirtualFree(VPTRDIFF(VPTRSUM(address, offset), size), size, MEM_DECOMMIT);
}

struct scrap_heap_buffer
{
	void* addr;
	size_t size;
};

struct scrap_heap_manager
{
	scrap_heap_buffer buffers[SHM_BUFFER_COUNT];
	size_t scrap_heap_count;
	size_t total_free_bytes;
	light_critical_section critical_section;

	void* operator new(size_t size) { return NVHR::nvhr_malloc(size); }
	void operator delete(void* address) { NVHR::nvhr_free(address); }
};

scrap_heap_manager* shm;

void shm_ctor(scrap_heap_manager* self)
{
	shm = new scrap_heap_manager();
	memset(shm->buffers, 0, SHM_BUFFER_COUNT * sizeof(scrap_heap_buffer));
	shm->scrap_heap_count = 0;
	shm->total_free_bytes = 0;
	shm->critical_section.thread_id = 0;
	shm->critical_section.lock_count = 0;
}

void __cdecl shm_init()
{
	shm_ctor(shm);
}

scrap_heap_manager* shm_get_singleton()
{
	return shm;
}

void __fastcall shm_swap_buffers(TFPARAM(scrap_heap_manager* self, size_t index))
{
	--self->scrap_heap_count;
	self->total_free_bytes -= self->buffers[index].size;
	if (index < self->scrap_heap_count)
	{
		self->buffers[index].addr = self->buffers[self->scrap_heap_count].addr;
		self->buffers[index].size = self->buffers[self->scrap_heap_count].size;
	}
}

void* __fastcall shm_create_buffer(TFPARAM(scrap_heap_manager* self, size_t size))
{
	void* address = Util::try_valloc(nullptr, SH_BUFFER_MAX_SIZE, MEM_RESERVE, PAGE_READWRITE, INFINITE);
	Util::try_valloc(address, size, MEM_COMMIT, PAGE_READWRITE, INFINITE);
	return address;
}

void* __fastcall shm_request_buffer(TFPARAM(scrap_heap_manager* self, size_t* size))
{
	if (!self->scrap_heap_count)
	{
		return shm_create_buffer(TFCALL(self, *size));
	}
	enter_light_critical_section(TFCALL(&self->critical_section, nullptr));
	if (self->scrap_heap_count)
	{
		size_t max_index = 0;
		size_t max_size = 0;
		for (size_t i = 0; i < self->scrap_heap_count; ++i)
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
		sh_grow(TFCALL(self, self->buffers[max_index].addr, max_size, *size - max_size));
		leave_light_critical_section(TFCALL(&self->critical_section));
		return self->buffers[max_index].addr;
	}
	void* address = shm_create_buffer(TFCALL(self, *size));
	leave_light_critical_section(TFCALL(&self->critical_section));
	return address;
}

void __fastcall shm_free_buffer(TFPARAM(scrap_heap_manager* self, void* address, size_t size))
{
	VirtualFree(address, 0, MEM_RELEASE);
}

void __fastcall shm_release_buffer(TFPARAM(scrap_heap_manager* self, void* address, size_t size))
{
	if (self->scrap_heap_count >= SHM_BUFFER_COUNT)
	{
		shm_free_buffer(TFCALL(self, address, size));
	}
	enter_light_critical_section(TFCALL(&self->critical_section, nullptr));
	if (self->scrap_heap_count < SHM_BUFFER_COUNT)
	{
		self->buffers[self->scrap_heap_count].addr = address;
		self->buffers[self->scrap_heap_count].size = size;
		self->scrap_heap_count++;
		self->total_free_bytes += size;
	}
	else
	{
		shm_free_buffer(TFCALL(self, address, size));
	}
	leave_light_critical_section(TFCALL(&self->critical_section));
}

void __fastcall shm_free_all_buffers(scrap_heap_manager* self)
{
	if (self->scrap_heap_count)
	{
		enter_light_critical_section(TFCALL(&self->critical_section, nullptr));
		for (size_t i = 0; i < self->scrap_heap_count; ++i)
		{
			shm_free_buffer(TFCALL(self, self->buffers[i].addr, self->buffers[i].size));
		}
		self->scrap_heap_count = 0;
		self->total_free_bytes = 0;
		leave_light_critical_section(TFCALL(&self->critical_section));
	}
}

struct scrap_heap_chunk
{
	size_t size;
	scrap_heap_chunk* prev_chunk;
};

struct scrap_heap
{
	void* commit_bgn;
	void* unused;
	void* commit_end;
	scrap_heap_chunk* last_chunk;
};

sh_array* mt_sh_array;

void __fastcall sh_init(TFPARAM(scrap_heap* self, size_t size))
{
	self->commit_bgn = shm_request_buffer(TFCALL(shm, &size));
	self->unused = self->commit_bgn;
	self->commit_end = VPTRSUM(self->commit_bgn, size);
	self->last_chunk = nullptr;
}

void __fastcall sh_init_0x10000(scrap_heap* self)
{
	sh_init(TFCALL(self, SH_BUFFER_MIN_SIZE));
}

void __fastcall sh_init_var(TFPARAM(scrap_heap* self, size_t size))
{
	size = Util::align(size, SH_BUFFER_MIN_SIZE);
	sh_init(TFCALL(self, size));
}

void* __fastcall sh_add_chunk(TFPARAM(scrap_heap* self, size_t size, size_t alignment))
{
	uintptr_t body = UPTRSUM(self->unused, sizeof(scrap_heap_chunk));
	body = Util::align(body, alignment);
	scrap_heap_chunk* header = (scrap_heap_chunk*)(body - sizeof(scrap_heap_chunk));
	void* desired_end = VPTRSUM(body, size);
	if (desired_end >= self->commit_end)
	{
		size_t old_size = UPTRDIFF(self->commit_end, self->commit_bgn);
		size_t new_size = old_size << 2;
		size_t grow_size = old_size;
		while (grow_size < UPTRDIFF(desired_end, self->commit_end))
		{
			new_size <<= 2;
			if (new_size > SH_BUFFER_MAX_SIZE)
			{
				MessageBox(NULL, "NVHR - Scrap heap failed to grow!", "Error", NULL);
				return nullptr;
			}
			grow_size = new_size - old_size;
		}
		sh_grow(TFCALL(self, self->commit_bgn, old_size, grow_size));
		self->commit_end = VPTRSUM(self->commit_end, grow_size);
	}
	header->size = size;
	header->prev_chunk = self->last_chunk;
	self->last_chunk = header;
	self->unused = desired_end;
	return (void*)body;
}

void __fastcall sh_remove_chunk(TFPARAM(scrap_heap* self, uintptr_t address))
{
	scrap_heap_chunk* chunk = (scrap_heap_chunk*)(address - sizeof(scrap_heap_chunk));
	if (address && !(chunk->size & SH_FREE_FLAG))
	{
		for (chunk->size |= SH_FREE_FLAG; self->last_chunk && (self->last_chunk->size & SH_FREE_FLAG); self->last_chunk = self->last_chunk->prev_chunk);
		self->unused = self->last_chunk ? (BYTE*)self->last_chunk + sizeof(scrap_heap_chunk) + self->last_chunk->size : self->commit_bgn;
		size_t new_size = (UPTRDIFF(self->commit_end, self->commit_bgn)) >> 2;
		if ((VPTRSUM(self->unused, new_size) < self->commit_end) && (new_size >= SH_BUFFER_MIN_SIZE))
		{
			sh_shrink(TFCALL(self, self->commit_bgn, new_size << 2, 3 * new_size));
			self->commit_end = VPTRDIFF(self->commit_end, 3 * new_size);
		}
	}
}

void __fastcall shm_create_mt(TFPARAM(void* self, size_t num_buckets))
{
	mt_sh_array = new sh_array(16);
}

scrap_heap* shm_get_scrap_heap(void* heap)
{
	DWORD id = GetCurrentThreadId();
	scrap_heap* sh;
	if (sh = mt_sh_array->find(id)) { return sh; }
	sh = (scrap_heap*)nvhr_malloc(sizeof(scrap_heap));
	sh_init_0x10000(sh);
	mt_sh_array->insert(id, sh);
	return sh;
}
