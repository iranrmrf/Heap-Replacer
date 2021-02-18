#pragma once

#include "main/util.h"

#include "cell_desc.h"

class cell_node;

class mem_cell
{

public:

	cell_node* size_node;
	cell_node* addr_node;
	cell_desc desc;

public:

	mem_cell(void* address, size_t size, size_t index);
	~mem_cell();

	bool precedes(mem_cell* cell);

	void join(mem_cell* other);
	mem_cell* split(size_t size);

	bool swap_by_size(mem_cell* cell);
	bool swap_by_addr(mem_cell* cell);

};