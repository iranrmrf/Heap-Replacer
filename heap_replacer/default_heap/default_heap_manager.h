#pragma once

#include "main/util.h"

#define HEAP_CELL_SIZE (4u * KB)
#define HEAP_COMMIT_SIZE (64u * MB)
#define HEAP_MAX_SIZE (1u * GB)

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
		if (!cell) [[unlikely]] { return nullptr; }
		this->heap->add_used(cell);
		void* address = cell->desc.addr;
		delete cell;
		return address;
	}

	void* calloc(size_t size)
	{
		void* address = this->malloc(size);
		if (address) [[likely]]	{ util::mem::memset32(address, NULL, (size + 3) >> 2); }
		return address;
	}

	size_t mem_size(void* address)
	{
		return this->heap->get_used(address);
	}

	bool free(void* address)
	{
		if (!this->heap->is_in_range(address)) [[unlikely]] { return false; }
		if (size_t size = this->heap->rmv_used(address))
		{
			mem_cell* cell = new mem_cell(address, size);
			this->heap->add_free_cell(cell);
		}
		return true;
	}

};
