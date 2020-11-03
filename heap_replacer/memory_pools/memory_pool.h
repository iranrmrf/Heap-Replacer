#pragma once

#include "util.h"

class memory_pool
{

private:

	struct cell
	{
		cell* next;
	};

private:

	size_t item_size;
	size_t max_size;
	size_t max_cell_count;

private:

	size_t block_count;
	size_t block_item_count;

private:

	void* pool_bgn;
	void* pool_cur;
	void* pool_end;

private:

	cell* free_cells;
	cell* next_free;

private:

	CRITICAL_SECTION critical_section;

public:

	memory_pool(size_t item_size, size_t max_size) : item_size(item_size), max_size(max_size), pool_bgn(nullptr), pool_cur(nullptr), pool_end(nullptr)
	{
		this->block_count = this->max_size / POOL_GROWTH;
		this->block_item_count = POOL_GROWTH / this->item_size;

		this->max_cell_count = this->block_count * this->block_item_count;

		this->free_cells = (cell*)util::mem::winapi_calloc(this->max_cell_count, sizeof(cell));
		this->next_free = this->free_cells;

		InitializeCriticalSectionEx(&this->critical_section, ~RTL_CRITICAL_SECTION_ALL_FLAG_BITS, RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO);
	}

	~memory_pool()
	{
		VirtualFree(this->free_cells, NULL, MEM_RELEASE);

		VirtualFree(this->pool_bgn, NULL, MEM_RELEASE);

		DeleteCriticalSection(&this->critical_section);
	}

	void* memory_pool_init()
	{
		size_t i = 0x10;
		while (!this->pool_bgn)
		{
			this->pool_bgn = VirtualAlloc((void*)(i * POOL_ALIGNMENT), this->max_size, MEM_RESERVE, PAGE_READWRITE);
			if (++i == 0xFF) { i = 0x10; }
		}
		this->pool_cur = this->pool_bgn;
		this->pool_end = VPTRSUM(this->pool_bgn, this->max_size);
		return this->pool_bgn;
	}

private:

	void setup_new_block()
	{
		this->pool_cur = VirtualAlloc(this->pool_cur, POOL_GROWTH, MEM_COMMIT, PAGE_READWRITE);
		size_t bank_offset = UPTRDIFF(this->pool_cur, this->pool_bgn) / POOL_GROWTH * this->block_item_count;
		this->free_cells[bank_offset].next = nullptr;
		for (size_t i = 0; i < this->block_item_count - 1; i++)
		{
			this->free_cells[bank_offset + i + 1].next = &this->free_cells[bank_offset + i];
		}
		this->next_free = &this->free_cells[bank_offset + this->block_item_count - 1];
		this->pool_cur = VPTRSUM(this->pool_cur, POOL_GROWTH);
	}

	void* free_ptr_to_real(void* address)
	{
		return VPTRSUM(this->pool_bgn, ((UPTRDIFF(address, this->free_cells) >> 2) * this->item_size));
	}

	void* real_to_free_ptr(void* address)
	{
		return VPTRSUM(((UPTRDIFF(address, this->pool_bgn) / this->item_size) << 2), this->free_cells);
	}

	bool is_in_range(void* address)
	{
		return ((this->pool_bgn <= address) & (address < this->pool_end));
	}

public:

	void* malloc()
	{
		ECS(&this->critical_section);
		if (!this->next_free->next) [[unlikely]]
		{
			if (this->pool_cur == this->pool_end) [[unlikely]]
			{
				LCS(&this->critical_section);
				return nullptr;
			}
			this->setup_new_block();
		}
		cell* old_free = this->next_free;
		this->next_free = this->next_free->next;
		old_free->next = nullptr;
		LCS(&this->critical_section);
		return this->free_ptr_to_real(old_free);
	}

	void* calloc()
	{
		void* address = this->malloc();
		if (address) [[likely]] { util::mem::memset32(address, NULL, this->item_size >> 2); }
		return address;
	}

	size_t mem_size(void* address)
	{
		return this->item_size;
	}

	void free(void* address)
	{
		cell* c = (cell*)this->real_to_free_ptr(address);
		ECS(&this->critical_section);
		if (!c->next) [[likely]]
		{
			c->next = this->next_free;
			this->next_free = c;
		}
		LCS(&this->critical_section);
	}

	void* operator new(size_t size)
	{
		return ina.malloc(size);
	}

	void operator delete(void* address)
	{
		ina.free(address);
	}

};
