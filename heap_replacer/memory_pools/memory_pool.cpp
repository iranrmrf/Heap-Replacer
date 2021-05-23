#include "memory_pool.h"

memory_pool::memory_pool(size_t item_size, size_t max_size, size_t index) : item_size(item_size), max_size(max_size), cell_count(0u), pool_bgn(nullptr), pool_cur(nullptr), pool_end(nullptr)
{
	this->block_count = this->max_size / pool_growth;
	this->block_item_count = pool_growth / this->item_size;

	this->max_cell_count = this->block_count * this->block_item_count;

	this->free_cells = (cell*)util::winapi_calloc(this->max_cell_count, sizeof(cell));
	this->next_free = this->free_cells;

	this->index = index;
}

memory_pool::~memory_pool()
{
	util::winapi_free(this->free_cells);

	util::winapi_free(this->pool_bgn);
}

void* memory_pool::memory_pool_init()
{
	size_t i = 0u;
	while (!this->pool_bgn)
	{
		this->pool_bgn = VirtualAlloc((void*)(++i * pool_alignment), this->max_size, MEM_RESERVE, PAGE_READWRITE);
		if (i == 0xFFu) { i = 0u; }
	}
	this->pool_cur = this->pool_bgn;
	this->pool_end = VPTRSUM(this->pool_bgn, this->max_size);
	return this->pool_bgn;
}

void memory_pool::setup_new_block()
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
}

void* memory_pool::free_ptr_to_real(void* address)
{
	return VPTRSUM(this->pool_bgn, ((UPTRDIFF(address, this->free_cells) >> 2u) * this->item_size));
}

void* memory_pool::real_to_free_ptr(void* address)
{
	return VPTRSUM(((UPTRDIFF(address, this->pool_bgn) / this->item_size) << 2u), this->free_cells);
}

bool memory_pool::is_in_range(void* address)
{
	return ((this->pool_bgn <= address) & (address < this->pool_end));
}

void* memory_pool::malloc()
{
	this->lock.lock();
	if (!this->next_free->next) [[unlikely]]
	{
		if (this->pool_cur == this->pool_end) [[unlikely]]
		{
			this->lock.unlock();
			return nullptr;
		}
		this->setup_new_block();
	}
	cell* old_free = this->next_free;
	this->next_free = this->next_free->next;
	this->cell_count++;
	this->lock.unlock();
	old_free->next = nullptr;
	return this->free_ptr_to_real(old_free);
}

void* memory_pool::calloc()
{
	void* address = this->malloc();
	if (address) [[likely]] { util::cmemset32(address, 0u, this->item_size >> 2u); }
	return address;
}

size_t memory_pool::mem_size(void* address)
{
	return this->item_size;
}

void memory_pool::free(void* address)
{
	cell* c = (cell*)this->real_to_free_ptr(address);
	this->lock.lock();
	if (!c->next) [[likely]]
	{
		this->cell_count--;
		c->next = this->next_free;
		this->next_free = c;
	}
	this->lock.unlock();
}

size_t memory_pool::get_index()
{
	return this->index;
}

void* memory_pool::operator new(size_t size)
{
	return hr::hr_ina_malloc(size);
}

void memory_pool::operator delete(void* address)
{
	hr::hr_ina_free(address);
}
