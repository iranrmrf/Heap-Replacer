#pragma once

#include "main/util.h"

#include "memory_pool_constants.h"

class memory_pool
{

private:

	struct cell { cell* next; };

private:

	size_t item_size;
	size_t max_size;
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

	memory_pool(size_t item_size, size_t max_size);
	~memory_pool();

	void* memory_pool_init();

private:

	void setup_new_block();

	void* free_ptr_to_real(void* address);
	void* real_to_free_ptr(void* address);

	bool is_in_range(void* address);

public:

	void* malloc();
	void* calloc();
	size_t mem_size(void* address);
	void free(void* address);

	void* operator new(size_t size);
	void operator delete(void* address);

};
