#pragma once

#include "main/util.h"

#include "cell_list.h"

struct cell_desc
{

	void* addr;
	size_t size;

	cell_desc(void* addr, size_t size) : addr(addr), size(size)
	{

	}

	void* get_end()
	{
		return VPTRSUM(this->addr, this->size);
	}

	bool is_in_range(void* address)
	{
		return ((this->addr <= address) & (address < this->get_end()));
	}

	void* operator new(size_t size)
	{
		return nvhr_malloc(size);
	}

	void operator delete(void* address)
	{
		nvhr_free(address);
	}

};

class cell_node;

class mem_cell
{

public:

	cell_node* size_node;
	cell_node* addr_node;
	cell_desc desc;

public:

	mem_cell(void* address, size_t size) : size_node(nullptr), addr_node(nullptr), desc(address, size)
	{

	}

	~mem_cell()
	{

	}

	bool is_adjacent_to(mem_cell* cell)
	{
		return ((this->desc.addr == cell->desc.get_end()) | (cell->desc.addr == this->desc.get_end()));
	}

	void join(mem_cell* other)
	{
		this->desc.addr = VPTRSUM(this->desc.addr, (UPTRDIFF(other->desc.addr, this->desc.addr) & (this->desc.addr < other->desc.addr) - 1));
		this->desc.size += other->desc.size;
	}

	mem_cell* split(size_t size)
	{
		//this->desc.size -= size;
		mem_cell* cell = new mem_cell(this->desc.addr, size);
		this->desc.addr = cell->desc.get_end();
		this->desc.size -= size;
		return cell;
		//return new mem_cell(this->desc.get_end(), size);
	}

	bool swap_by_size(mem_cell* cell)
	{
		return (this->desc.size < cell->desc.size) | ((this->desc.size == cell->desc.size) & (this->desc.addr < cell->desc.addr));
	}

	bool swap_by_addr(mem_cell* cell)
	{
		return (this->desc.addr < cell->desc.addr);
	}

	void* operator new(size_t size)
	{
		return nvhr_malloc(size);
	}

	void operator delete(void* address)
	{
		nvhr_free(address);
	}

};
