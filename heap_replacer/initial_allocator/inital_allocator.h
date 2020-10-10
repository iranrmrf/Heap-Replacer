#pragma once

#include "main/util.h"

#define INITIAL_ALLOCATOR_SIZE 0x0001000u

class initial_allocator
{

private:

	size_t size;
	size_t count;
	void* ina_bgn;
	void* ina_end;
	void* last_alloc;

private:

	CRITICAL_SECTION critical_section;

public:

	initial_allocator(size_t size) : size(size), count(0)
	{
		this->ina_bgn = Util::Mem::winapi_calloc(size, 1);
		this->ina_end = VPTRSUM(this->ina_bgn, size);

		this->last_alloc = this->ina_bgn;

		InitializeCriticalSectionEx(&this->critical_section, ~RTL_CRITICAL_SECTION_ALL_FLAG_BITS, RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO);
	}

	~initial_allocator()
	{
		Util::Mem::winapi_free(this->ina_bgn);

		DeleteCriticalSection(&this->critical_section);
	}

	void* malloc(size_t size)
	{
		void* new_last_alloc = VPTRSUM(this->last_alloc, size + 4);
		if (new_last_alloc > this->ina_end) { return nullptr; }
		ECS(&this->critical_section);
		*(size_t*)this->last_alloc = size;
		void* address = VPTRSUM(this->last_alloc, 4);
		this->last_alloc = Util::align(new_last_alloc, 4);
		++this->count;
		LCS(&this->critical_section);
		return address;
	}

	void* calloc(size_t count, size_t size)
	{
		return malloc(count * size);
	}

	void free(void* address)
	{
		if (this->is_in_range(address) && !(--this->count))
		{
			Util::Mem::winapi_free(this->ina_bgn);
		}
	}

	size_t mem_size(void* address)
	{
		return (this->is_in_range(address)) ? *((size_t*)address - 1) : 0;
	}

	bool is_in_range(void* address)
	{
		return ((this->ina_bgn <= address) & (address < this->ina_end));
	}

} ina(INITIAL_ALLOCATOR_SIZE);
