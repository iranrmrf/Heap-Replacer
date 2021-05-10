#include "mem_cell.h"

mem_cell::mem_cell(void* address, size_t size, size_t index) : size_node(nullptr), addr_node(nullptr), desc{ address, size, index }
{

}

bool mem_cell::precedes(mem_cell* cell)
{
	return (cell->desc.addr == this->desc.get_end());
}

void mem_cell::join(mem_cell* other)
{
	this->desc.addr = (this->desc.addr < other->desc.addr) ? this->desc.addr : other->desc.addr;
	this->desc.size += other->desc.size;
}

mem_cell* mem_cell::split(size_t size)
{
	this->desc.size -= size;
	return new mem_cell(this->desc.get_end(), size, this->desc.index);
}

bool mem_cell::swap_by_size(mem_cell* cell)
{
	return ((this->desc.size < cell->desc.size) | ((this->desc.size == cell->desc.size) & (this->desc.addr < cell->desc.addr)));
}

bool mem_cell::swap_by_addr(mem_cell* cell)
{
	return (this->desc.addr < cell->desc.addr);
}
