#pragma once

#include "list.h"
#include "mem_cell.h"
#include "default_heap.h"

#include "util.h"

#define CELL_SIZE 4 * KB
#define COMMIT_SIZE 32 * MB
#define RESERVE_SIZE 128 * MB
#define MAX_HEAP_SIZE 2 * GB

class default_heap_manager
{

private:

	default_heap* heap;

public:

	default_heap_manager()
	{
		this->heap = new default_heap(CELL_SIZE, COMMIT_SIZE, RESERVE_SIZE, MAX_HEAP_SIZE);
	}

	~default_heap_manager()
	{

	}

public:

	void* malloc(size_t size)
	{
		mem_cell* cell = this->heap->get_free_cell(size);
		if (!cell) { return nullptr; }
		return cell->desc.addr;
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
		size_t size;
		if (size = this->heap->rmv_used(address))
		{
			mem_cell* cell = new mem_cell(address, size);
			this->heap->add_free_cell(cell);
		}
		return true;
	}

};
