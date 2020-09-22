#pragma once

#include "main/util.h"

#define HEAP_CELL_SIZE (4 * KB)
#define HEAP_COMMIT_SIZE (64 * MB)
#define HEAP_MAX_SIZE (1 * GB)

#include "default_heap.h"

class default_heap_manager
{

private:

	default_heap* heap;

public:

	default_heap_manager()
	{
		this->heap = new default_heap();
	}

	~default_heap_manager()
	{
		delete this->heap;
	}

public:

	void* malloc(size_t size)
	{
		mem_cell* cell = this->heap->get_free_cell(size);
		if (!cell) { return nullptr; }
		this->heap->add_used(cell);
		void* address = cell->desc.addr;
		delete cell;
		return address;
	}

	void* calloc(size_t size)
	{
		void* address = this->malloc(size);
		memset(address, 0, size);
		return address;
	}

	size_t mem_size(void* address)
	{
		return this->heap->get_used(address);
	}

	bool free(void* address)
	{
		if (!this->heap->is_in_range(address)) { return false; }
		if (size_t size = this->heap->rmv_used(address))
		{
			mem_cell* cell = new mem_cell(address, size);
			this->heap->add_free_cell(cell);
		}
		return true;
	}

	void* operator new(size_t size)
	{
		return NVHR::nvhr_malloc(size);
	}

	void operator delete(void* address)
	{
		NVHR::nvhr_free(address);
	}

};
