#pragma once

#include "main/util.h"

#include "default_heap.h"

class default_heap_manager
{

private:

	default_heap heap;

#ifdef HR_USE_GUI

private:

	size_t allocs;
	size_t frees;

#endif

public:

	default_heap_manager();
	~default_heap_manager();

public:

	void* malloc(size_t size);
	void* calloc(size_t size);
	size_t mem_size(void* address);
	bool free(void* address);

public:

#ifdef HR_USE_GUI

	size_t get_cell_count() { return default_heap_cell_count; }

	size_t get_allocs() { size_t retval = this->allocs; this->allocs = 0u; return retval; }
	size_t get_frees() { size_t retval = this->frees; this->frees = 0u; return retval; }

	size_t get_used_size() { return this->heap.get_used_size(); }
	size_t get_free_size() { return this->heap.get_free_size(); }
	size_t get_curr_size() { return this->heap.get_curr_size(); }

	size_t get_free_blocks() { return this->heap.get_free_cells(); }

	size_t get_block_count() { return this->heap.get_block_count(); }
	size_t get_addr_size_by_index(size_t block, size_t index) { return this->heap.get_addr_size_by_index(block, index); }

#endif

};
