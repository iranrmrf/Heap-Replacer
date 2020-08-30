#pragma once

#include "sh_array.h"

#include "util.h"

#define BUFFER_COUNT 64

#define BUFFER_MAX_SIZE 0x00800000
#define BUFFER_MIN_SIZE 0x00010000

#define FREE_FLAG 0x80000000

struct light_critical_section
{
	DWORD thread_id;
	size_t lock_count;
};

void __fastcall enter_light_critical_section(light_critical_section* self, void* _, const char* name)
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
			if (++spin <= 10000u)
			{
				Sleep(0u);
			}
			else
			{
				Sleep(1u);
			}
		}
		self->lock_count = 1;
	}
}

void __fastcall leave_light_critical_section(light_critical_section* self)
{
	if (!--self->lock_count)
	{
		self->thread_id = 0;
	}
}

void __fastcall sh_grow(void* self, void* _, void* address, size_t offset, size_t size)
{
	while (VirtualAlloc((void*)((uintptr_t)address + offset), size, MEM_COMMIT, PAGE_READWRITE) == 0)
	{
		// add something here
	}
}

void __fastcall sh_shrink(void* self, void* _, void* address, size_t offset, size_t size)
{
	VirtualFree((void*)((uintptr_t)address + offset - size), size, MEM_DECOMMIT);
}

struct scrap_heap_buffer
{
	void* addr;
	size_t size;
};

struct scrap_heap_manager
{
	scrap_heap_buffer buffers[BUFFER_COUNT];
	size_t scrap_heap_count;
	size_t total_free_bytes;
	light_critical_section critical_section;
};

scrap_heap_manager* shm;

void shm_ctor(scrap_heap_manager* self)
{
	shm = new scrap_heap_manager();
	memset(shm->buffers, 0, BUFFER_COUNT * sizeof(scrap_heap_buffer));
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

void __fastcall shm_swap_buffers(scrap_heap_manager* self, void* _, size_t index)
{
	--self->scrap_heap_count;
	self->total_free_bytes -= self->buffers[index].size;
	if (index < self->scrap_heap_count)
	{
		self->buffers[index].addr = self->buffers[self->scrap_heap_count].addr;
		self->buffers[index].size = self->buffers[self->scrap_heap_count].size;
	}
}

void* __fastcall shm_create_buffer(scrap_heap_manager* self, void* _, size_t size)
{
	void* address = VirtualAlloc(NULL, BUFFER_MAX_SIZE, MEM_RESERVE, PAGE_READWRITE);
	while (VirtualAlloc(address, size, MEM_COMMIT, PAGE_READWRITE) == 0) { }
	return address;
}

void* __fastcall shm_request_buffer(scrap_heap_manager* self, void* _, size_t* size)
{
	if (!self->scrap_heap_count) { return shm_create_buffer(self, nullptr, *size); }
	enter_light_critical_section(&self->critical_section, nullptr, nullptr);
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
				shm_swap_buffers(self, nullptr, i);
				leave_light_critical_section(&self->critical_section);
				return address;
			}
			if (self->buffers[i].size > max_size)
			{
				max_size = self->buffers[i].size;
				max_index = i;
			}
		}
		shm_swap_buffers(self, nullptr, max_index);
		sh_grow(self, nullptr, self->buffers[max_index].addr, max_size, *size - max_size);
		leave_light_critical_section(&self->critical_section);
		return self->buffers[max_index].addr;
	}
	void* address = shm_create_buffer(self, nullptr, *size);
	leave_light_critical_section(&self->critical_section);
	return address;
}

void __fastcall shm_free_buffer(scrap_heap_manager* self, void* _, void* address, size_t size)
{
	VirtualFree(address, 0, MEM_RELEASE);
}

void __fastcall shm_release_buffer(scrap_heap_manager* self, void* _, void* address, size_t size)
{
	if (self->scrap_heap_count >= BUFFER_COUNT)
	{
		shm_free_buffer(self, nullptr, address, size);
	}
	enter_light_critical_section(&self->critical_section, nullptr, nullptr);
	if (self->scrap_heap_count < BUFFER_COUNT)
	{
		self->buffers[self->scrap_heap_count].addr = address;
		self->buffers[self->scrap_heap_count].size = size;
		self->scrap_heap_count++;
		self->total_free_bytes += size;
	}
	else
	{
		shm_free_buffer(self, nullptr, address, size);
	}
	leave_light_critical_section(&self->critical_section);
}

void __fastcall shm_free_all_buffers(scrap_heap_manager* self)
{
	if (self->scrap_heap_count)
	{
		enter_light_critical_section(&self->critical_section, nullptr, nullptr);
		for (size_t i = 0; i < self->scrap_heap_count; ++i)
		{
			shm_free_buffer(self, nullptr, self->buffers[i].addr, self->buffers[i].size);
		}
		self->scrap_heap_count = 0;
		self->total_free_bytes = 0;
		leave_light_critical_section(&self->critical_section);
	}
}

struct sh_chunk_header
{
	size_t size;
	sh_chunk_header* next_chunk;
};

struct scrap_heap
{
	void* commit_bgn;	
	void* unused;
	void* commit_end;
	sh_chunk_header* last_chunk;
};

sh_array* mt_sh_array;

void __fastcall sh_init(scrap_heap* self, void* _, size_t size)
{
	self->commit_bgn = shm_request_buffer(shm, nullptr, &size);
	self->unused = self->commit_bgn;
	self->commit_end = (void*)((uintptr_t)self->commit_bgn + size);
	self->last_chunk = nullptr;
}

void __fastcall sh_init_0x10000(scrap_heap* self)
{
	sh_init(self, nullptr, BUFFER_MIN_SIZE);
}

void __fastcall sh_init_var(scrap_heap* self, void* _, size_t size)
{
	if (size & 0x0000FFFF)
	{
		size += BUFFER_MIN_SIZE;
		size &= 0xFFFF0000;
	}
	sh_init(self, nullptr, size);
}

void* __fastcall sh_add_chunk(scrap_heap* self, void* _, size_t size, size_t alignment)
{
	uintptr_t body = (uintptr_t)self->unused + sizeof(sh_chunk_header);
	body += 4 - (body & 3);
	sh_chunk_header* header = (sh_chunk_header*)(body - sizeof(sh_chunk_header));
	void* desired_end = (void*)(body + size);
	if (desired_end > self->commit_end)
	{
		size_t old_size = (uintptr_t)self->commit_end - (uintptr_t)self->commit_bgn;
		size_t new_size = 2 * old_size;
		size_t grow_size = 0;
		while (grow_size < (uintptr_t)desired_end - (uintptr_t)self->commit_end)
		{
			new_size <<= 1;
			grow_size = new_size - old_size;
			if (new_size > BUFFER_MAX_SIZE)
			{
				MessageBox(NULL, "Scrap heap failed to grow!", "Error", NULL);
				return nullptr;
			}
		}
		sh_grow(self, nullptr, self->commit_bgn, old_size, grow_size);
		self->commit_end = (void*)((uintptr_t)self->commit_end + grow_size);
	}
	header->size = size;
	header->next_chunk = self->last_chunk;
	self->last_chunk = header;
	self->unused = desired_end;
	return (void*)body;
}

void __fastcall sh_remove_chunk(scrap_heap* self, void* _, uintptr_t address)
{
	sh_chunk_header* chunk = (sh_chunk_header*)(address - sizeof(sh_chunk_header));
	if (address && !(chunk->size & FREE_FLAG))
	{
		for (chunk->size |= FREE_FLAG; self->last_chunk && (self->last_chunk->size & FREE_FLAG); self->last_chunk = self->last_chunk->next_chunk);
		self->unused = self->last_chunk ? (BYTE*)self->last_chunk + (sizeof(sh_chunk_header) + self->last_chunk->size) : self->commit_bgn;
		size_t old_size = (uintptr_t)self->commit_end - (uintptr_t)self->commit_bgn;
		size_t new_size = (old_size >> 1);
		if ((((uintptr_t)self->commit_end - (uintptr_t)self->unused) > new_size) && (old_size > BUFFER_MIN_SIZE))
		{
			sh_shrink(self, nullptr, self->commit_bgn, (uintptr_t)self->commit_end - (uintptr_t)self->commit_bgn, new_size);
			self->commit_end = (void*)((uintptr_t)self->commit_end - new_size);
		}
	}
}

void __fastcall shm_create_mt(void* self, void* _, size_t num_buckets)
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
