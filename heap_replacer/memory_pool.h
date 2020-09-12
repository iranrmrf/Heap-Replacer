#pragma once

#include "util.h"

class memory_pool_container
{
public:
	virtual ~memory_pool_container() { }
	virtual void* memory_pool_init() = 0;
	virtual void* malloc() = 0;
	virtual void* calloc() = 0;
	virtual size_t mem_size(void*) = 0;
	virtual bool free(void*) = 0;
	virtual size_t item_size() = 0;
	virtual size_t max_size() = 0;
};

template <size_t S, size_t M>
class memory_pool : public memory_pool_container
{

private:

	typedef BYTE cell[S];

	struct free_cell
	{
		free_cell* next;
	};

private:

	size_t block_count;
	size_t block_cell_count;

	size_t total_cell_count;

	cell* pool_bgn;
	cell* pool_cur;
	cell* pool_end;

private:

	free_cell* free_cells;
	free_cell* next_free;

	CRITICAL_SECTION critical_section;

public:

	memory_pool() : pool_bgn(nullptr), pool_cur(nullptr), pool_end(nullptr)
	{
		this->block_count = M / POOL_GROWTH;
		this->block_cell_count = POOL_GROWTH / S;

		this->total_cell_count = this->block_count * this->block_cell_count;

		this->free_cells = (free_cell*)winapi_alloc(this->total_cell_count * sizeof(free_cell));
		this->next_free = this->free_cells;

		InitializeCriticalSectionAndSpinCount(&this->critical_section, INFINITE);
	}

	~memory_pool()
	{

	}

	void* memory_pool_init()
	{
		size_t i = 0x10;
		while (!this->pool_bgn)
		{
			this->pool_bgn = (cell*)try_valloc((void*)(i * POOL_ALIGNMENT), M, MEM_RESERVE, PAGE_READWRITE, 1);
			if (++i == 0xFF) { i = 0x10; }
		}
		this->pool_cur = this->pool_bgn;
		this->pool_end = this->pool_bgn + this->total_cell_count;
		return this->pool_bgn;
	}

private:

	void setup_new_block()
	{
		this->pool_cur = (cell*)try_valloc(this->pool_cur, POOL_GROWTH, MEM_COMMIT, PAGE_READWRITE, 1);
		size_t bank_offset = this->pool_cur - this->pool_bgn;
		this->free_cells[bank_offset].next = nullptr;
		for (size_t i = 0; i < this->block_cell_count - 1; i++)
		{
			this->free_cells[bank_offset + i + 1].next = &this->free_cells[bank_offset + i];
		}
		this->next_free = &this->free_cells[bank_offset + this->block_cell_count - 1];
		this->pool_cur += this->block_cell_count;
	}

	bool is_in_range(void* address)
	{
		return ((this->pool_bgn <= address) && (address < this->pool_cur));
	}

public:

	void* malloc()
	{
		ECS(&this->critical_section);
		if (!this->next_free->next)
		{
			if (this->pool_cur == this->pool_end)
			{
				LCS(&this->critical_section);
				return nullptr;
			}
			this->setup_new_block();
		}
		free_cell* old_free = this->next_free;
		this->next_free = this->next_free->next;
		old_free->next = nullptr;
		LCS(&this->critical_section);
		return &this->pool_bgn[old_free - this->free_cells];
	}

	void* calloc()
	{
		void* address = this->malloc();
		if (address) { memset(address, 0, S); }
		return address;
	}

	size_t mem_size(void* address)
	{
		return (this->is_in_range(address)) ? S : 0;
	}

	bool free(void* address)
	{
		if (!this->is_in_range(address)) { return false; }
		ECS(&this->critical_section);
		free_cell* fc = &this->free_cells[(cell*)address - this->pool_bgn];
		if (!fc->next)
		{
			fc->next = this->next_free;
			this->next_free = fc;
		}
		LCS(&this->critical_section);
		return true;
	}

	size_t item_size() { return S; }

	size_t max_size() { return M; }

};
