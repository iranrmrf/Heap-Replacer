#include "scrap_heap_block.h"

void scrap_heap_block::init(size_t size)
{
	this->commit_bgn = scrap_heap_manager::get_instance()->request_buffer(&size);
	this->unused = this->commit_bgn;
	this->commit_end = VPTRSUM(this->commit_bgn, size);
	this->last_chunk = nullptr;
}

void scrap_heap_block::init_0x10000()
{
	this->init(scrap_heap_buffer_min_size);
}

void scrap_heap_block::init_var(size_t size)
{
	this->init(util::align<scrap_heap_buffer_min_size>(size));
}

void* scrap_heap_block::alloc(size_t size, size_t alignment)
{
	// alignment always 4
	void* body = util::align<4>(VPTRSUM(this->unused, sizeof(scrap_heap_chunk)));
	void* desired_end = VPTRSUM(body, size);
	if (desired_end > this->commit_end) [[unlikely]]
	{
		size_t old_size = UPTRDIFF(this->commit_end, this->commit_bgn);
		size_t new_size = old_size << 2;
		while (new_size < UPTRDIFF(desired_end, this->commit_bgn)) [[unlikely]]
		{
			if ((new_size <<= 2) > scrap_heap_buffer_max_size) [[unlikely]]
			{
				if (VPTRSUM(this->commit_bgn, scrap_heap_buffer_max_size) <= desired_end)
				{
					new_size = scrap_heap_buffer_max_size;
				}
				else
				{
					HR_MSGBOX("Scrap heap failed to grow!");
					return nullptr;
				}
			}
		}
		VirtualAlloc(this->commit_end, new_size - old_size, MEM_COMMIT, PAGE_READWRITE);
		this->commit_end = VPTRSUM(this->commit_bgn, new_size);
	}
	scrap_heap_chunk* header = (scrap_heap_chunk*)VPTRDIFF(body, sizeof(scrap_heap_chunk));
	header->size = size;
	header->prev_chunk = this->last_chunk;
	this->last_chunk = header;
	this->unused = desired_end;
	return body;
}

void scrap_heap_block::free(void* address)
{
	scrap_heap_chunk* chunk = (scrap_heap_chunk*)VPTRDIFF(address, sizeof(scrap_heap_chunk));
	if (address && !(chunk->size & scrap_heap_free_flag)) [[likely]]
	{
		for (chunk->size |= scrap_heap_free_flag; this->last_chunk && (this->last_chunk->size & scrap_heap_free_flag); this->last_chunk = this->last_chunk->prev_chunk);
		this->unused = this->last_chunk ? (BYTE*)this->last_chunk + sizeof(scrap_heap_chunk) + this->last_chunk->size : this->commit_bgn;
		size_t old_size = UPTRDIFF(this->commit_end, this->commit_bgn);
		size_t new_size = old_size >> 2;
		if ((VPTRSUM(this->commit_bgn, new_size) >= this->unused) & (new_size >= scrap_heap_buffer_min_size)) [[unlikely]]
		{
			VirtualFree(VPTRDIFF(this->commit_end, old_size - new_size), old_size - new_size, MEM_DECOMMIT);
			this->commit_end = VPTRSUM(this->commit_bgn, new_size);
		}
	}
}

void scrap_heap_block::purge()
{
	scrap_heap_manager::get_instance()->release_buffer(this->commit_bgn, UPTRDIFF(this->commit_end, this->commit_bgn));
	this->commit_bgn = nullptr;
	this->unused = nullptr;
}

scrap_heap_block* scrap_heap_block::get_thread_scrap_heap()
{
	DWORD id = GetCurrentThreadId();
	scrap_heap_block* sh;
	scrap_heap_manager* shm = scrap_heap_manager::get_instance();
	if (sh = shm->mt_sh_vector->find(id)) [[likely]] { return sh; }
	sh = (scrap_heap_block*)nvhr::nvhr_malloc(sizeof(scrap_heap_block));
	sh->init_0x10000();
	shm->mt_sh_vector->insert(id, sh);
	return sh;
}
