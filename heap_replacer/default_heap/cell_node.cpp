#include "cell_node.h"

cell_node::cell_node(mem_cell* cell) : next(nullptr), prev(nullptr), cell(cell), array_index(-1)
{

}

cell_node::~cell_node()
{

}

cell_node* cell_node::link(cell_node* prev, cell_node* next)
{
	this->prev = prev;
	prev->next = this;
	this->next = next;
	next->prev = this;
	return this;
}

bool cell_node::is_valid()
{
	return cell;
}
