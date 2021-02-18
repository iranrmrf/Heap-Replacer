#include "memory_pool.h"

memory_pool::memory_pool(size_t item_size, size_t max_size) : item_size(item_size), max_size(max_size), pool_bgn(nullptr), pool_cur(nullptr), pool_end(nullptr)
{
	this->block_count = this->max_size / pool_growth;
	this->block_item_count = pool_growth / this->item_size;

	this->max_cell_count = this->block_count * this->block_item_count;

	this->free_cells = (cell*)util::winapi_calloc(this->max_cell_count, sizeof(cell));
	this->next_free = this->free_cells;

	this->lock_id = 0u;
}

memory_pool::~memory_pool()
{
	VirtualFree(this->free_cells, 0u, MEM_RELEASE);

	VirtualFree(this->pool_bgn, 0u, MEM_RELEASE);
}

void* memory_pool::memory_pool_init()
{
	size_t i = 0x10;
	while (!this->pool_bgn)
	{
		this->pool_bgn = VirtualAlloc((void*)(i * pool_alignment), this->max_size, MEM_RESERVE, PAGE_READWRITE);
		if (++i == 0xFF) { i = 0x10; }
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
	for (size_t i = 0; i < this->block_item_count - 1; i++)
	{
		this->free_cells[bank_offset + i + 1].next = &this->free_cells[bank_offset + i];
	}
	this->next_free = &this->free_cells[bank_offset + this->block_item_count - 1];
	this->pool_cur = VPTRSUM(this->pool_cur, pool_growth);
}

void* memory_pool::free_ptr_to_real(void* address)
{
	return VPTRSUM(this->pool_bgn, ((UPTRDIFF(address, this->free_cells) >> 2) * this->item_size));
}

void* memory_pool::real_to_free_ptr(void* address)
{
	return VPTRSUM(((UPTRDIFF(address, this->pool_bgn) / this->item_size) << 2), this->free_cells);
}

bool memory_pool::is_in_range(void* address)
{
	return ((this->pool_bgn <= address) & (address < this->pool_end));
}

void* memory_pool::malloc()
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
	UNLOCK(&this->lock_id);
	old_free->next = nullptr;
	return this->free_ptr_to_real(old_free);
}

void* memory_pool::calloc()
{
	void* address = this->malloc();
	if (address) [[likely]] { util::memset32(address, 0u, this->item_size >> 2); }
	return address;
}

size_t memory_pool::mem_size(void* address)
{
	return this->item_size;
}

void memory_pool::free(void* address)
{
	cell* c = (cell*)this->real_to_free_ptr(address);
	LOCK(&this->lock_id);
	if (!c->next) [[likely]]
	{
		c->next = this->next_free;
		this->next_free = c;
#ifdef HR_ZERO_MEM
			util::memset32(address, 0u, this->item_size >> 2);
#endif
	}
	UNLOCK(&this->lock_id);
}

void* memory_pool::operator new(size_t size)
{
	return nvhr::nvhr_ina_malloc(size);
}

void memory_pool::operator delete(void* address)
{
	nvhr::nvhr_ina_free(address);
}
