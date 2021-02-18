#include "initial_allocator.h"

initial_allocator::initial_allocator(size_t size) : size(size), count(0)
{
	this->ina_bgn = util::winapi_calloc(1, size);
	this->ina_end = VPTRSUM(this->ina_bgn, size);

	this->last_alloc = this->ina_bgn;

	this->lock_id = 0u;
}

initial_allocator::~initial_allocator()
{
	util::winapi_free(this->ina_bgn);
}

void* initial_allocator::malloc(size_t size)
{
	void* new_last_alloc = VPTRSUM(this->last_alloc, size + sizeof(size_t));
	if (new_last_alloc > this->ina_end) { return nullptr; }
	LOCK(&this->lock_id);
	*(size_t*)this->last_alloc = size;
	void* address = VPTRSUM(this->last_alloc, sizeof(size_t));
	this->last_alloc = util::align<4u>(new_last_alloc);
	++this->count;
	UNLOCK(&this->lock_id);
	return address;
}

void* initial_allocator::calloc(size_t count, size_t size)
{
	return malloc(count * size);
}

void initial_allocator::free(void* address)
{
	if (this->is_in_range(address) && !(--this->count))
	{
		util::winapi_free(this->ina_bgn);
	}
}

size_t initial_allocator::mem_size(void* address)
{
	return (this->is_in_range(address)) ? *((size_t*)address - 1) : 0u;
}

bool initial_allocator::is_in_range(void* address)
{
	return ((this->ina_bgn <= address) & (address < this->ina_end));
}
