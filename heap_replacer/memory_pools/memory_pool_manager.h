#pragma once

#include "main/util.h"

#include "memory_pool.h"
#include "memory_pool_constants.h"

class memory_pool_manager
{

private:

	memory_pool* pools_by_size[pool_size_array_length];
	memory_pool* pools_by_addr[pool_addr_array_length];

public:

	memory_pool_manager();
	~memory_pool_manager();

private:

	void init_all_pools();

	memory_pool* pool_from_size(size_t size);
	memory_pool* pool_from_addr(void* address);

public:

	void* malloc(size_t size);
	void* calloc(size_t size);
	size_t mem_size(void* address);
	bool free(void* address);

	void* operator new(size_t size);
	void operator delete(void* address);

};
