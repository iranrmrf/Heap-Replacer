#include "default_heap_manager.h"

default_heap_manager::default_heap_manager() : used_size(0u)
{
	heap = new default_heap();
}

default_heap_manager::~default_heap_manager()
{
	delete this->heap;
}

void* default_heap_manager::malloc(size_t size)
{
	mem_cell* cell = this->heap->get_free_cell(size);
	if (!cell) [[unlikely]] { return nullptr; }
	this->heap->add_used(cell);
	void* address = cell->desc.addr;
	this->used_size += cell->desc.size;
	delete cell;
	return address;
}

void* default_heap_manager::calloc(size_t size)
{
	void* address = this->malloc(size);
	if (address) [[likely]] { util::memset32(address, 0u, (size + 3) >> 2); }
	return address;
}

size_t default_heap_manager::mem_size(void* address)
{
	size_t index = this->heap->get_block_index(address);
	if (index == -1) [[unlikely]] { return 0u; }
	return this->heap->get_used(address, index);
}

bool default_heap_manager::free(void* address)
{
	size_t index = this->heap->get_block_index(address);
	if (index == -1) [[unlikely]] { return false; }
		if (size_t size = this->heap->rmv_used(address, index))
		{
#ifdef HR_ZERO_MEM
			util::mem::memset32(address, 0u, (size + 3) >> 2);
#endif
			this->used_size -= size;
			mem_cell* cell = new mem_cell(address, size, index);
			this->heap->add_free_cell(cell);
		}
	return true;
}
