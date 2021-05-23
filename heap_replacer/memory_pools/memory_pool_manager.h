#pragma once

#include "main/util.h"

#include "memory_pool.h"
#include "memory_pool_constants.h"

class memory_pool_manager
{

private:

	memory_pool* pools_by_size[pool_size_array_length];
	memory_pool* pools_by_addr[pool_addr_array_length];
	memory_pool* pools_by_indx[pool_indx_array_length];

#ifdef HR_USE_GUI

private:

	size_t allocs;
	size_t frees;

#endif

public:

	memory_pool_manager();
	~memory_pool_manager();

private:

	void init_all_pools();

	memory_pool* pool_from_size(size_t size);
	memory_pool* pool_from_addr(void* address);
	memory_pool* pool_from_indx(size_t index);

public:

	void* malloc(size_t size);
	void* calloc(size_t size);
	size_t mem_size(void* address);
	bool free(void* address);

#ifdef HR_USE_GUI

public:

	size_t get_allocs() { size_t retval = this->allocs; this->allocs = 0u; return retval; }
	size_t get_frees() { size_t retval = this->frees; this->frees = 0u; return retval; }

	size_t get_pool_cell_count(size_t size) { return this->pool_from_size(size)->get_cell_count(); }
	size_t get_pool_max_cell_count(size_t size) { return this->pool_from_size(size)->get_max_cell_count(); }
	size_t get_pool_max_size(size_t size) { return this->pool_from_size(size)->get_max_size(); }

#endif

public:

	void* operator new(size_t size);
	void operator delete(void* address);

};
