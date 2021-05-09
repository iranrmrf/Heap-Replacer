#pragma once

#include "main/util.h"

#include "locks/nonreentrant_lock.h"

#define INITIAL_ALLOCATOR_SIZE (0x00001000u)

class initial_allocator
{

private:

	size_t size;
	size_t count;

private:

	void* ina_bgn;
	void* ina_end;
	void* last_alloc;

private:

	nonreentrant_lock lock;

public:

	initial_allocator(size_t size);
	~initial_allocator();

	void* malloc(size_t size);
	void* calloc(size_t count, size_t size);
	void free(void* address);
	size_t mem_size(void* address);

	bool is_in_range(void* address);

};
