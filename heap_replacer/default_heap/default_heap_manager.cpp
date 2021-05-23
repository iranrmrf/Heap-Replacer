#include "default_heap_manager.h"

default_heap_manager::default_heap_manager()
{

#ifdef HR_USE_GUI
	this->allocs = 0u;
	this->frees = 0u;
#endif

}

default_heap_manager::~default_heap_manager()
{

}

void* default_heap_manager::malloc(size_t size)
{
	mem_cell* cell = this->heap.get_free_cell(size);
	if (!cell) [[unlikely]] { HR_PRINTF("DHM OOM"); return nullptr; }
	this->heap.add_used(cell);
	void* address = cell->desc.addr;
#ifdef HR_USE_GUI
	this->allocs++;
#endif
	delete cell;
	return address;
}

void* default_heap_manager::calloc(size_t size)
{
	void* address = this->malloc(size);
	if (address) [[likely]] { util::cmemset32(address, 0u, (size + 3u) >> 2u); }
	return address;
}

size_t default_heap_manager::mem_size(void* address)
{
	size_t index = this->heap.get_block_index(address);
	if (index == default_heap_block_count) [[unlikely]] { return 0u; }
	return this->heap.get_used(address, index);
}

bool default_heap_manager::free(void* address)
{
	size_t index = this->heap.get_block_index(address);
	if (index == default_heap_block_count) [[unlikely]] { return false; }
	if (size_t size = this->heap.rmv_used(address, index))
	{
#ifdef HR_ZERO_MEM
		util::cmemset32(address, 0u, (size + 3u) >> 2u);
#endif
#ifdef HR_USE_GUI
		this->frees++;
#endif
		mem_cell* cell = new mem_cell(address, size, index);
		this->heap.add_free_cell(cell);
	}
	return true;
}
