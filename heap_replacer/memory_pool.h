#pragma once

#include "util.h"

#define POOL_GROWTH 0x00010000
#define POOL_ALIGNMENT 0x01000000

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
	size_t max_item_count;

private:

	size_t block_size;
	size_t block_count;
	size_t block_item_count;

	void* block_bgn;
	void* block_end;

private:

	cell* free_cells;
	cell* next_free;

	CRITICAL_SECTION critical_section;

public:

	memory_pool(size_t item_size, size_t max_size) : item_size(item_size), max_size(max_size), block_bgn(nullptr), block_end(nullptr)
	{
		this->block_size = POOL_GROWTH;
		this->block_count = this->max_size / this->block_size;
		this->block_item_count = this->block_size / this->item_size;

		this->max_item_count = this->block_count * this->block_item_count;

		this->free_cells = (cell*)winapi_alloc(this->max_item_count * sizeof(cell));
		this->next_free = this->free_cells;

		InitializeCriticalSectionAndSpinCount(&this->critical_section, INFINITE);
	}

	~memory_pool()
	{
		
	}

	void* memory_pool_init()
	{
		size_t i = 0x80;
		while (!this->block_bgn)
		{
			this->block_bgn = try_valloc((void*)(i * POOL_ALIGNMENT), this->max_size, MEM_RESERVE, PAGE_READWRITE, 1);
			if (++i == 0xFF) { i = 0x80; }
		}
		return this->block_bgn;
	}

private:

	void setup_new_block(void* address)
	{
		this->block_end = try_valloc(address, this->block_size, MEM_COMMIT, PAGE_READWRITE, 1);
		size_t bank_offset = ((uintptr_t)this->block_end - (uintptr_t)this->block_bgn) / this->block_size * this->block_item_count;
		this->free_cells[bank_offset].next = nullptr;
		for (size_t i = 0; i < this->block_item_count - 1; i++)
		{
			this->free_cells[bank_offset + i + 1].next = &this->free_cells[bank_offset + i];
		}
		this->block_end = (void*)((uintptr_t)this->block_end + this->block_size);
		this->next_free = &this->free_cells[bank_offset + this->block_item_count - 1];
	}

	void* free_ptr_to_real(cell* address)
	{
		return (void*)((uintptr_t)this->block_bgn + ((((uintptr_t)address - (uintptr_t)this->free_cells) >> 2) * this->item_size));
	}

	cell* real_to_free_ptr(void* address)
	{
		return (cell*)(((((uintptr_t)address - (uintptr_t)this->block_bgn) / this->item_size) << 2) + (uintptr_t)this->free_cells);
	}

	bool is_in_range(void* address)
	{
		return ((this->block_bgn <= address) && (address < this->block_end));
	}

public:

	void* malloc()
	{
		ECS(&this->critical_section);
		if (!this->next_free->next)
		{
			if ((uintptr_t)this->block_bgn + this->max_size <= (uintptr_t)this->block_end)
			{
				LCS(&this->critical_section); return nullptr;
			}
			this->setup_new_block(this->block_end ? this->block_end : this->block_bgn);
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
		if (address) { memset(address, 0, this->item_size); }
		return address;
	}

	size_t mem_size(void* address)
	{
		return (this->is_in_range(address)) ? this->item_size : 0;
	}

	bool free(void* address)
	{
		if (!this->is_in_range(address)) { return false; }
		ECS(&this->critical_section);
		cell* cell;
		if (!((cell = this->real_to_free_ptr(address))->next))
		{
			cell->next = this->next_free;
			this->next_free = cell;
		}
		LCS(&this->critical_section);
		return true;
	}

};
