#pragma once

#include "util.h"

#include "sh_array.h"

#define BUFFER_COUNT 64

#define BUFFER_MAX_SIZE 0x01000000
#define BUFFER_MIN_SIZE 0x00010000

#define FREE_FLAG 0x80000000

struct light_critical_section
{
	DWORD thread_id;
	size_t lock_count;
};

void __fastcall enter_light_critical_section(TtFParam(light_critical_section* self, const char* name))
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

void __fastcall leave_light_critical_section(TtFParam(light_critical_section* self))
{
	if (!--self->lock_count)
	{
		self->thread_id = 0;
	}
}

void __fastcall sh_grow(TtFParam(void* self, void* address, size_t offset, size_t size))
{
	try_valloc((void*)((uintptr_t)address + offset), size, MEM_COMMIT, PAGE_READWRITE, 1);
}

void __fastcall sh_shrink(TtFParam(void* self, void* address, size_t offset, size_t size))
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

void __fastcall shm_swap_buffers(TtFParam(scrap_heap_manager* self, size_t index))
{
	--self->scrap_heap_count;
	self->total_free_bytes -= self->buffers[index].size;
	if (index < self->scrap_heap_count)
	{
		self->buffers[index].addr = self->buffers[self->scrap_heap_count].addr;
		self->buffers[index].size = self->buffers[self->scrap_heap_count].size;
	}
}

void* __fastcall shm_create_buffer(TtFParam(scrap_heap_manager* self, size_t size))
{
	void* address = try_valloc(nullptr, BUFFER_MAX_SIZE, MEM_RESERVE, PAGE_READWRITE, INFINITE);
	try_valloc(address, size, MEM_COMMIT, PAGE_READWRITE, INFINITE);
	return address;
}

void* __fastcall shm_request_buffer(TtFParam(scrap_heap_manager* self, size_t* size))
{
	if (!self->scrap_heap_count)
	{
		return shm_create_buffer(TtFCall(self, *size));
	}
	enter_light_critical_section(TtFCall(&self->critical_section, nullptr));
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
				shm_swap_buffers(TtFCall(self, i));
				leave_light_critical_section(TtFCall(&self->critical_section));
				return address;
			}
			if (self->buffers[i].size > max_size)
			{
				max_size = self->buffers[i].size;
				max_index = i;
			}
		}
		shm_swap_buffers(TtFCall(self, max_index));
		sh_grow(TtFCall(self, self->buffers[max_index].addr, max_size, *size - max_size));
		leave_light_critical_section(TtFCall(&self->critical_section));
		return self->buffers[max_index].addr;
	}
	void* address = shm_create_buffer(TtFCall(self, *size));
	leave_light_critical_section(TtFCall(&self->critical_section));
	return address;
}

void __fastcall shm_free_buffer(TtFParam(scrap_heap_manager* self, void* address, size_t size))
{
	VirtualFree(address, 0, MEM_RELEASE);
}

void __fastcall shm_release_buffer(TtFParam(scrap_heap_manager* self, void* address, size_t size))
{
	if (self->scrap_heap_count >= BUFFER_COUNT)
	{
		shm_free_buffer(TtFCall(self, address, size));
	}
	enter_light_critical_section(TtFCall(&self->critical_section, nullptr));
	if (self->scrap_heap_count < BUFFER_COUNT)
	{
		self->buffers[self->scrap_heap_count].addr = address;
		self->buffers[self->scrap_heap_count].size = size;
		self->scrap_heap_count++;
		self->total_free_bytes += size;
	}
	else
	{
		shm_free_buffer(TtFCall(self, address, size));
	}
	leave_light_critical_section(TtFCall(&self->critical_section));
}

void __fastcall shm_free_all_buffers(scrap_heap_manager* self)
{
	if (self->scrap_heap_count)
	{
		enter_light_critical_section(TtFCall(&self->critical_section, nullptr));
		for (size_t i = 0; i < self->scrap_heap_count; ++i)
		{
			shm_free_buffer(TtFCall(self, self->buffers[i].addr, self->buffers[i].size));
		}
		self->scrap_heap_count = 0;
		self->total_free_bytes = 0;
		leave_light_critical_section(TtFCall(&self->critical_section));
	}
}

struct scrap_heap_chunk
{
	size_t size;
	scrap_heap_chunk* next_chunk;
};

struct scrap_heap
{
	void* commit_bgn;
	void* unused;
	void* commit_end;
	scrap_heap_chunk* last_chunk;
};

sh_array* mt_sh_array;

void __fastcall sh_init(TtFParam(scrap_heap* self, size_t size))
{
	self->commit_bgn = shm_request_buffer(TtFCall(shm, &size));
	self->unused = self->commit_bgn;
	self->commit_end = (void*)((uintptr_t)self->commit_bgn + size);
	self->last_chunk = nullptr;
}

void __fastcall sh_init_0x10000(scrap_heap* self)
{
	sh_init(TtFCall(self, BUFFER_MIN_SIZE));
}

void __fastcall sh_init_var(TtFParam(scrap_heap* self, size_t size))
{
	if (size & 0x0000FFFF)
	{
		size += 0x10000;
		size &= 0xFFFF0000;
	}
	sh_init(TtFCall(self, size));
}

void* __fastcall sh_add_chunk(TtFParam(scrap_heap* self, size_t size, size_t alignment))
{
	uintptr_t body = (uintptr_t)self->unused + sizeof(scrap_heap_chunk);
	body += alignment - (alignment ? body & (alignment - 1) : alignment);
	scrap_heap_chunk* header = (scrap_heap_chunk*)(body - sizeof(scrap_heap_chunk));
	void* desired_end = (void*)(body + size);
	if (desired_end >= self->commit_end)
	{
		size_t old_size = (uintptr_t)self->commit_end - (uintptr_t)self->commit_bgn;
		size_t new_size = old_size << 1;
		size_t grow_size = old_size;
		while (grow_size < (uintptr_t)desired_end - (uintptr_t)self->commit_end)
		{
			new_size <<= 2;
			grow_size = new_size - old_size;
			if (new_size > BUFFER_MAX_SIZE)
			{
				MessageBox(NULL, "Scrap heap failed to grow!", "Error", NULL);
				return nullptr;
			}
		}
		sh_grow(TtFCall(self, self->commit_bgn, old_size, grow_size));
		self->commit_end = (void*)((uintptr_t)self->commit_end + grow_size);
	}
	header->size = size;
	header->next_chunk = self->last_chunk;
	self->last_chunk = header;
	self->unused = desired_end;
	return (void*)body;
}

void __fastcall sh_remove_chunk(TtFParam(scrap_heap* self, uintptr_t address))
{
	scrap_heap_chunk* chunk = (scrap_heap_chunk*)(address - sizeof(scrap_heap_chunk));
	if (address && !(chunk->size & FREE_FLAG))
	{	
		for (chunk->size |= FREE_FLAG; self->last_chunk && (self->last_chunk->size & FREE_FLAG); self->last_chunk = self->last_chunk->next_chunk);
		self->unused = self->last_chunk ? (BYTE*)self->last_chunk + sizeof(scrap_heap_chunk) + self->last_chunk->size : self->commit_bgn;
		size_t new_size = ((uintptr_t)self->commit_end - (uintptr_t)self->commit_bgn) >> 2;
		if ((((uintptr_t)self->commit_end - (uintptr_t)self->unused) > new_size) && (new_size >= BUFFER_MIN_SIZE))
		{
			sh_shrink(TtFCall(self, self->commit_bgn, new_size << 2, new_size));
			self->commit_end = (void*)((uintptr_t)self->commit_end - new_size);
		}
	}
}

void __fastcall shm_create_mt(TtFParam(void* self, size_t num_buckets))
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
