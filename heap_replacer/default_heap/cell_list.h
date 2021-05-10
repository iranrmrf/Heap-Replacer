#pragma once

#include "main/util.h"

#include "cell_node.h"
#include "mem_cell.h"

class cell_list
{

private:

	cell_node* fake_head;
	cell_node* fake_tail;

#ifdef HR_USE_GUI

	size_t size;

#endif

public:

	cell_list();

	cell_node* get_head();
	cell_node* get_tail();

	cell_node* add_head(mem_cell* cell);
	cell_node* add_tail(mem_cell* cell);

	cell_node* insert_before(cell_node* position, mem_cell* cell);
	cell_node* insert_after(cell_node* position, mem_cell* cell);

	void remove_node(cell_node* node);

	bool is_empty();

#ifdef HR_USE_GUI

	size_t get_size();

#endif

};
