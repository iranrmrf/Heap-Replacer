#pragma once

#include "main/util.h"

#include "memory_pool_interface.h"
#include "memory_pool_constants.h"

template <size_t S, size_t M>
class memory_pool : public memory_pool_interface
{

private:

	struct cell { cell* next; };

private:

	size_t cell_count;
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

	DWORD lock_id;

public:

	memory_pool() : cell_count(0u), pool_bgn(nullptr), pool_cur(nullptr), pool_end(nullptr)
	{
		this->block_count = M / pool_growth;
		this->block_item_count = pool_growth / S;

		this->max_cell_count = this->block_count * this->block_item_count;

		this->free_cells = (cell*)util::winapi_calloc(this->max_cell_count, sizeof(cell));
		this->next_free = this->free_cells;

		this->lock_id = 0u;
	}

	~memory_pool()
	{
		VirtualFree(this->free_cells, 0u, MEM_RELEASE);

		VirtualFree(this->pool_bgn, 0u, MEM_RELEASE);
	}

	virtual void* memory_pool_init()
	{
		size_t i = 0x10u;
		while (!this->pool_bgn)
		{
			this->pool_bgn = VirtualAlloc((void*)(i * pool_alignment), M, MEM_RESERVE, PAGE_READWRITE);
			if (++i == 0xFFu) { i = 0x10u; }
		}
		this->pool_cur = this->pool_bgn;
		this->pool_end = VPTRSUM(this->pool_bgn, M);
		return this->pool_bgn;
	}

private:

	void setup_new_block()
	{
		this->pool_cur = VirtualAlloc(this->pool_cur, pool_growth, MEM_COMMIT, PAGE_READWRITE);
		size_t bank_offset = UPTRDIFF(this->pool_cur, this->pool_bgn) / pool_growth * this->block_item_count;
		this->free_cells[bank_offset].next = nullptr;
		for (size_t i = 0u; i < this->block_item_count - 1u; i++)
		{
			this->free_cells[bank_offset + i + 1u].next = &this->free_cells[bank_offset + i];
		}
		this->next_free = &this->free_cells[bank_offset + this->block_item_count - 1u];
		this->pool_cur = VPTRSUM(this->pool_cur, pool_growth);
		this->cell_count += this->block_item_count;
	}

	void* free_ptr_to_real(void* address)
	{
		return VPTRSUM(this->pool_bgn, ((UPTRDIFF(address, this->free_cells) >> 2u) * S));
	}

	void* real_to_free_ptr(void* address)
	{
		return VPTRSUM(((UPTRDIFF(address, this->pool_bgn) / S) << 2u), this->free_cells);
	}

	bool is_in_range(void* address)
	{
		return ((this->pool_bgn <= address) & (address < this->pool_end));
	}

public:

	virtual void* malloc()
	{
		LOCK(&this->lock_id);
		if (!this->next_free->next) [[unlikely]]
		{
			if (this->pool_cur == this->pool_end) [[unlikely]]
			{
				UNLOCK(&this->lock_id);
				return nullptr;
			}
			this->setup_new_block();
		}
		cell* old_free = this->next_free;
		this->next_free = this->next_free->next;
		this->cell_count--;
		UNLOCK(&this->lock_id);
		old_free->next = nullptr;
		return this->free_ptr_to_real(old_free);
	}

	virtual void* calloc()
	{
		void* address = this->malloc();
		if (address) [[likely]] { util::memset32(address, 0u, S >> 2u); }
		return address;
	}

	virtual size_t mem_size(void* address)
	{
		return S;
	}

	virtual void free(void* address)
	{
		cell* c = (cell*)this->real_to_free_ptr(address);
		LOCK(&this->lock_id);
		if (!c->next) [[likely]]
		{
			this->cell_count++;
			c->next = this->next_free;
			this->next_free = c;
	#ifdef HR_ZERO_MEM
			util::memset32(address, 0u, this->item_size >> 2u);
	#endif
		}
		UNLOCK(&this->lock_id);
	}
	
public:

	virtual size_t item_size() { return S; }
	virtual size_t max_size() { return M; }

public:

	size_t get_cell_count() { return this->cell_count; }
	size_t get_max_cell_count() { return this->max_cell_count; }
	size_t get_max_size() { return M; }

public:

	void* operator new(size_t size)
	{
		return hr::hr_ina_malloc(size);
	}

	void operator delete(void* address)
	{
		hr::hr_ina_free(address);
	}

};
