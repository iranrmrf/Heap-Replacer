#pragma once

#include "list.h"

#include "util.h"

struct cell_desc
{

	void* addr;
	size_t size;
	
	cell_desc(void* addr, size_t size) : addr(addr), size(size)
	{
	
	}
	
	void* get_end()
	{
		return (void*)((uintptr_t)this->addr + this->size);
	}

	bool is_in_range(void* address)
	{
		return ((this->addr <= address) && (address < this->get_end()));
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

class mem_cell
{

public:

	node<mem_cell*>* size_node;
	node<mem_cell*>* addr_node;

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
		return (this->desc.get_end() == cell->desc.addr) || (cell->desc.get_end() == this->desc.addr);
	}

	void join(mem_cell* other)
	{
		if (this->desc.addr > other->desc.addr)
		{
			this->desc.addr = other->desc.addr;
		}
		this->desc.size += other->desc.size;
	}

	mem_cell* split(size_t size)
	{
		mem_cell* cell = new mem_cell(this->desc.addr, size);
		this->desc.addr = cell->desc.get_end();
		this->desc.size -= size;
		return cell;
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
