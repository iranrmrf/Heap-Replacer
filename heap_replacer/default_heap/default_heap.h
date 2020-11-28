#pragma once

#include "main/util.h"

#include "cell_list.h"

class default_heap
{

private:

	cell_list* size_dlist;
	cell_list* addr_dlist;

	mem_cell** size_array;
	mem_cell** addr_array;

	size_t* used_cells;

private:

	size_t cell_count;
	cell_desc* heap_desc;

private:

	void* last_addr;

private:

	CRITICAL_SECTION critical_section;

public:

	default_heap()
	{
		this->heap_desc = new cell_desc(VirtualAlloc(nullptr, HEAP_MAX_SIZE, MEM_RESERVE, PAGE_READWRITE), HEAP_MAX_SIZE);
		if (!this->heap_desc->addr)
		{
			HR_MSGBOX("Failed to reserve!");
		}

		this->last_addr = this->heap_desc->addr;

		this->size_dlist = new cell_list();
		this->addr_dlist = new cell_list();

		this->cell_count = HEAP_MAX_SIZE / HEAP_CELL_SIZE;

		this->size_array = (mem_cell**)util::mem::winapi_calloc(this->cell_count, sizeof(mem_cell*));
		this->addr_array = (mem_cell**)util::mem::winapi_calloc(this->cell_count, sizeof(mem_cell*));

		this->used_cells = (size_t*)util::mem::winapi_calloc(this->cell_count, sizeof(size_t));

		InitializeCriticalSectionEx(&this->critical_section, ~RTL_CRITICAL_SECTION_ALL_FLAG_BITS, RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO);
	}

	~default_heap()
	{
		delete this->size_dlist;
		delete this->addr_dlist;

		util::mem::winapi_free(this->used_cells);

		util::mem::winapi_free(this->size_array);
		util::mem::winapi_free(this->addr_array);

		util::mem::winapi_free(this->heap_desc->addr);

		delete this->heap_desc;

		DeleteCriticalSection(&this->critical_section);
	}

private:

	size_t get_size_index(size_t size)
	{
		return (size / HEAP_CELL_SIZE) - 1;
	}

	size_t get_size_index(mem_cell* cell)
	{
		return this->get_size_index(cell->desc.size);
	}

	size_t get_addr_index(void* address)
	{
		return UPTRDIFF(address, this->heap_desc->addr) / HEAP_CELL_SIZE;
	}

	size_t get_addr_index(mem_cell* cell)
	{
		return this->get_addr_index(cell->desc.addr);
	}

	void add_size_array(mem_cell* cell)
	{
		cell_node* curr;
		for (curr = cell->size_node->prev; curr->is_valid() && cell->swap_by_size(curr->cell); curr = curr->prev);
		util::mem::memset32(&this->size_array[curr->array_index + 1], (DWORD)cell, cell->size_node->array_index - curr->array_index);
	}

	void add_addr_array(mem_cell* cell)
	{
		cell_node* curr;
		for (curr = cell->addr_node->prev; curr->is_valid() && cell->swap_by_addr(curr->cell); curr = curr->prev);
		util::mem::memset32(&this->addr_array[curr->array_index + 1], (DWORD)cell, cell->addr_node->array_index - curr->array_index);
	}

	void rmv_size_array(mem_cell* cell)
	{
		cell_node* curr = cell->size_node->prev;
		util::mem::memset32(&this->size_array[curr->array_index + 1], (DWORD)cell->size_node->next->cell, cell->size_node->array_index - curr->array_index);
	}

	void rmv_addr_array(mem_cell* cell)
	{
		cell_node* curr = cell->addr_node->prev;
		util::mem::memset32(&this->addr_array[curr->array_index + 1], (DWORD)cell->addr_node->next->cell, cell->addr_node->array_index - curr->array_index);
	}

	cell_node* insert_size_dlist(mem_cell* cell)
	{
		mem_cell* elem = this->size_array[this->get_size_index(cell)];
		if (!elem) [[unlikely]] { return this->size_dlist->add_tail(cell); }
		cell_node* curr;
		for (curr = elem->size_node; curr->is_valid() && !cell->swap_by_size(curr->cell); curr = curr->next);
		return this->size_dlist->insert_before(curr, cell);
	}

	cell_node* insert_addr_dlist(mem_cell* cell)
	{
		mem_cell* elem = this->addr_array[this->get_addr_index(cell)];
		if (!elem) [[unlikely]] { return this->addr_dlist->add_tail(cell); }
		cell_node* curr;
		for (curr = elem->addr_node; curr->is_valid() && !cell->swap_by_addr(curr->cell); curr = curr->next);
		return this->addr_dlist->insert_before(curr, cell);
	}

	void add_free_cell_size(mem_cell* cell)
	{
		cell->size_node = this->insert_size_dlist(cell);
		cell->size_node->array_index = this->get_size_index(cell);
	}

	void add_free_cell_addr(mem_cell* cell)
	{
		cell->addr_node = this->insert_addr_dlist(cell);
		cell->addr_node->array_index = this->get_addr_index(cell);
	}

	void rmv_free_cell(mem_cell* cell)
	{
		this->rmv_size_array(cell);
		this->rmv_addr_array(cell);
		this->size_dlist->remove_node(cell->size_node);
		this->addr_dlist->remove_node(cell->addr_node);
		cell->size_node = nullptr;
		cell->addr_node = nullptr;
	}

public:

	void add_free_cell(mem_cell* cell)
	{
		EnterCriticalSection(&this->critical_section);
		this->add_free_cell_addr(cell);
		mem_cell* temp;
		if ((temp = cell->addr_node->prev->cell) && cell->is_adjacent_to(temp)) [[unlikely]]
		{
			this->rmv_free_cell(temp);
			cell->join(temp);
			cell->addr_node->array_index = this->get_addr_index(cell);
			delete temp;
		}
		if ((temp = cell->addr_node->next->cell) && cell->is_adjacent_to(temp)) [[unlikely]]
		{
			this->rmv_free_cell(temp);
			cell->join(temp);
			cell->addr_node->array_index = this->get_addr_index(cell);
			delete temp;
		}
		this->add_addr_array(cell);
		this->add_free_cell_size(cell);
		this->add_size_array(cell);
		LeaveCriticalSection(&this->critical_section);
	}

	mem_cell* get_free_cell(size_t size)
	{
		size = util::align(size, HEAP_CELL_SIZE);
		mem_cell* cell;
		EnterCriticalSection(&this->critical_section);
		while (!(cell = this->size_array[this->get_size_index(size)])) [[unlikely]]
		{
			this->add_free_cell(this->commit());
		}
		if (cell->desc.size == size)
		{
			this->rmv_free_cell(cell);
		}
		else
		{
			this->rmv_size_array(cell);
			this->size_dlist->remove_node(cell->size_node);
			mem_cell* split = cell->split(size);
			this->add_free_cell_size(cell);
			this->add_size_array(cell);
			cell = split;
		}
		LeaveCriticalSection(&this->critical_section);
		return cell;
	}

	void add_used(mem_cell* cell)
	{
		InterlockedExchange(&this->used_cells[this->get_addr_index(cell)], cell->desc.size);
	}

	size_t rmv_used(void* address)
	{
		return InterlockedExchange(&this->used_cells[this->get_addr_index(address)], 0u);
	}

	size_t get_used(void* address)
	{
		return this->used_cells[this->get_addr_index(address)];
	}

	bool is_in_range(void* address)
	{
		return this->heap_desc->is_in_range(address);
	}

private:

	mem_cell* commit()
	{
		if (this->last_addr == this->heap_desc->get_end())
		{
			HR_MSGBOX("Out of memory!");
			return nullptr;
		}
		void* address;
		if (!(address = VirtualAlloc(this->last_addr, HEAP_COMMIT_SIZE, MEM_COMMIT, PAGE_READWRITE)))
		{
			HR_MSGBOX("Commit fail!");
			return nullptr;
		}
		mem_cell* cell = new mem_cell(address, HEAP_COMMIT_SIZE);
		this->last_addr = cell->desc.get_end();
		return cell;
	}

	bool free_is_empty()
	{
		return (this->size_dlist->is_empty() | this->addr_dlist->is_empty());
	}

};
