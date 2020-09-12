#pragma once

#include "util.h"

#include "mem_cell.h"

class cell_node
{

public:

	cell_node* next;
	cell_node* prev;
	mem_cell* cell;

	cell_node(mem_cell* cell) : next(nullptr), prev(nullptr), cell(cell)
	{

	}

	~cell_node()
	{

	}

	void link(cell_node* prev, cell_node* next)
	{
		this->prev = prev;
		prev->next = this;
		this->next = next;
		next->prev = this;
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


class cell_list
{

private:

	cell_node* head;
	cell_node* tail;
	size_t size;

public:

	cell_list() : head(nullptr), tail(nullptr), size(0)
	{
		this->head = new cell_node(nullptr);
		this->tail = new cell_node(nullptr);
		this->head->next = this->tail;
		this->tail->prev = this->head;
	}

	~cell_list()
	{

	}

	cell_node* get_head()
	{
		return this->head->next;
	}

	cell_node* get_tail()
	{
		return this->tail->prev;
	}

	cell_node* add_head(mem_cell* cell)
	{
		this->size++;
		cell_node* new_node = new cell_node(cell);
		new_node->link(this->head, this->head->next);
		return new_node;
	}

	cell_node* add_tail(mem_cell* cell)
	{
		this->size++;
		cell_node* new_node = new cell_node(cell);
		new_node->link(this->tail->prev, this->tail);
		return new_node;
	}

	cell_node* insert_before(cell_node* position, mem_cell* cell)
	{
		this->size++;
		cell_node* new_node = new cell_node(cell);
		new_node->link(position->prev, position);
		return new_node;
	}

	void remove_node(cell_node* node)
	{
		this->size--;
		node->prev->next = node->next;
		node->next->prev = node->prev;
		delete node;
	}

	bool is_empty()
	{
		return !this->size;
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
