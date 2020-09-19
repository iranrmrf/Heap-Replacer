#pragma once

#include "main/util.h"

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

	bool is_valid()
	{
		return cell;
	}

	void* operator new(size_t size)
	{
		return NVHR::nvhr_malloc(size);
	}

	void operator delete(void* address)
	{
		NVHR::nvhr_free(address);
	}

};

class cell_list
{

private:

	cell_node* fake_head;
	cell_node* fake_tail;
	size_t size;

public:

	cell_list() : fake_head(nullptr), fake_tail(nullptr), size(0)
	{
		this->fake_head = new cell_node(nullptr);
		this->fake_tail = new cell_node(nullptr);
		this->fake_head->next = this->fake_tail;
		this->fake_tail->prev = this->fake_head;
	}

	~cell_list()
	{

	}

	cell_node* get_head()
	{
		return this->fake_head->next;
	}

	cell_node* get_tail()
	{
		return this->fake_tail->prev;
	}

	cell_node* add_head(mem_cell* cell)
	{
		this->size++;
		cell_node* new_node = new cell_node(cell);
		new_node->link(this->fake_head, this->fake_head->next);
		return new_node;
	}

	cell_node* add_tail(mem_cell* cell)
	{
		this->size++;
		cell_node* new_node = new cell_node(cell);
		new_node->link(this->fake_tail->prev, this->fake_tail);
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

	size_t get_size()
	{
		return this->size;
	}

	bool is_empty()
	{
		return !this->size;
	}

	void* operator new(size_t size)
	{
		return NVHR::nvhr_malloc(size);
	}

	void operator delete(void* address)
	{
		NVHR::nvhr_free(address);
	}

};
