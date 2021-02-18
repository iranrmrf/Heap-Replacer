#include "cell_list.h"

cell_list::cell_list()
{
	this->fake_head = new cell_node(nullptr);
	this->fake_tail = new cell_node(nullptr);
	this->fake_head->next = this->fake_tail;
	this->fake_tail->prev = this->fake_head;
}

cell_list::~cell_list()
{

}

cell_node* cell_list::get_head()
{
	return this->fake_head->next;
}

cell_node* cell_list::get_tail()
{
	return this->fake_tail->prev;
}

cell_node* cell_list::add_head(mem_cell* cell)
{
	return (new cell_node(cell))->link(this->fake_head, this->fake_head->next);
}

cell_node* cell_list::add_tail(mem_cell* cell)
{
	return (new cell_node(cell))->link(this->fake_tail->prev, this->fake_tail);
}

cell_node* cell_list::insert_before(cell_node* position, mem_cell* cell)
{
	return (new cell_node(cell))->link(position->prev, position);
}

cell_node* cell_list::insert_after(cell_node* position, mem_cell* cell)
{
	return (new cell_node(cell))->link(position, position->next);
}

void cell_list::remove_node(cell_node* node)
{
	node->prev->next = node->next;
	node->next->prev = node->prev;
	delete node;
}

bool cell_list::is_empty()
{
	return this->fake_head->next == this->fake_tail;
}
