#pragma once

#include "main/util.h"

#include "cell_list.h"

class default_heap
{

private:

	cell_list* size_dlist;
	cell_list* addr_dlist;

	mem_cell** size_array;
	//mem_cell** addr_array;

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
		this->heap_desc = new cell_desc(try_valloc(nullptr, HEAP_MAX_SIZE, MEM_RESERVE, PAGE_READWRITE, 1), HEAP_MAX_SIZE);
		if (!this->heap_desc->addr)
		{
			MessageBox(NULL, "NVHR - Failed to alloc!", "Error", NULL);
		}

		this->last_addr = this->heap_desc->addr;

		this->size_dlist = new cell_list();
		this->addr_dlist = new cell_list();

		this->cell_count = HEAP_MAX_SIZE / HEAP_CELL_SIZE;

		this->used_cells = (size_t*)winapi_alloc(this->cell_count * sizeof(size_t));

		this->size_array = (mem_cell**)winapi_alloc(this->cell_count * sizeof(mem_cell*));
		//this->addr_array = (mem_cell**)winapi_alloc(this->item_count * sizeof(mem_cell*));

		InitializeCriticalSectionEx(&this->critical_section, ~RTL_CRITICAL_SECTION_ALL_FLAG_BITS, RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO);
	}

	~default_heap()
	{
		delete this->size_dlist;
		delete this->addr_dlist;

		VirtualFree(this->used_cells, NULL, MEM_RELEASE);
		VirtualFree(this->size_array, NULL, MEM_RELEASE);
		// VirtualFree(this->addr_array, NULL, MEM_RELEASE);

		VirtualFree(this->heap_desc->addr, NULL, MEM_RELEASE);

		DeleteCriticalSection(&this->critical_section);
	}

	int get_size_index(size_t size)
	{
		return (size / HEAP_CELL_SIZE) - 1;
	}

	int get_addr_index(void* address)
	{
		return UPTRDIFF(address, this->heap_desc->addr) / HEAP_CELL_SIZE;
	}

	void add_size_array(mem_cell* cell)
	{
		for (int i = this->get_size_index(cell->desc.size); (i > -1) && (!this->size_array[i] || cell->swap_by_size(this->size_array[i])); this->size_array[i--] = cell);
	}

	void rmv_size_array(mem_cell* cell)
	{
		for (int i = this->get_size_index(cell->desc.size); (i > -1) && (this->size_array[i] == cell); this->size_array[i--] = cell->size_node->next->cell);
	}

	cell_node* insert_free_size(mem_cell* cell)
	{
		mem_cell* elem = this->size_array[this->get_size_index(cell->desc.size)];
		if (!elem) { return this->size_dlist->add_tail(cell); }
		cell_node* curr;
		for (curr = elem->size_node; curr->is_valid() && !cell->swap_by_size(curr->cell); curr = curr->next);
		return this->size_dlist->insert_before(curr, cell);
	}

	cell_node* insert_free_addr(mem_cell* cell)
	{
		int i = 0;
		cell_node* curr;
		for (curr = this->addr_dlist->get_head(); curr->is_valid() && !cell->swap_by_addr(curr->cell); curr = curr->next, i++);
		return this->addr_dlist->insert_before(curr, cell);
	}

	void rmv_free_cell(mem_cell* cell)
	{
		this->size_dlist->remove_node(cell->size_node);
		this->addr_dlist->remove_node(cell->addr_node);
	}

	void add_free_cell(mem_cell* cell)
	{
		ECS(&this->critical_section);
		cell->addr_node = this->insert_free_addr(cell);
		mem_cell* temp;
		if ((temp = cell->addr_node->next->cell) && cell->is_adjacent_to(temp))
		{
			this->rmv_size_array(temp);
			cell->join(temp);
			this->rmv_free_cell(temp);
			delete temp;
		}
		if ((temp = cell->addr_node->prev->cell) && cell->is_adjacent_to(temp))
		{
			this->rmv_size_array(temp);
			cell->join(temp);
			this->rmv_free_cell(temp);
			delete temp;
		}
		cell->size_node = this->insert_free_size(cell);
		this->add_size_array(cell);
		LCS(&this->critical_section);
	}

	mem_cell* get_free_cell(size_t size)
	{
		size = align(size, HEAP_CELL_SIZE);
		ECS(&this->critical_section);
		mem_cell* cell;
		while (!(cell = this->size_array[this->get_size_index(size)]))
		{
			this->add_free_cell(this->commit());
		}
		this->rmv_size_array(cell);
		if (cell->desc.size == size)
		{
			this->rmv_free_cell(cell);
		}
		else
		{
			mem_cell* split = cell->split(size);
			this->size_dlist->remove_node(cell->size_node);
			cell->size_node = this->insert_free_size(cell);
			this->add_size_array(cell);
			cell = split;
		}
		LCS(&this->critical_section);
		return cell;
	}

	mem_cell* commit()
	{
		if (this->last_addr == this->heap_desc->get_end())
		{
			MessageBox(NULL, "NVHR - Out of memory!", "Error", NULL);
			return nullptr;
		}
		void* address;
		if (!(address = try_valloc(this->last_addr, HEAP_COMMIT_SIZE, MEM_COMMIT, PAGE_READWRITE, 1)))
		{
			MessageBox(NULL, "NVHR - Commit fail!", "Error", NULL);
			return nullptr;
		}
		mem_cell* cell = new mem_cell(address, HEAP_COMMIT_SIZE);
		this->last_addr = cell->desc.get_end();
		return cell;
	}

	void add_used(mem_cell* cell)
	{
		InterlockedExchange(&this->used_cells[this->get_addr_index(cell->desc.addr)], cell->desc.size);
	}

	size_t rmv_used(void* address)
	{
		return InterlockedExchange(&this->used_cells[this->get_addr_index(address)], 0);
	}

	size_t get_used(void* address)
	{
		return this->used_cells[this->get_addr_index(address)];
	}

	bool free_is_empty()
	{
		return (this->size_dlist->is_empty() | this->addr_dlist->is_empty());
	}

	bool is_in_range(void* address)
	{
		return this->heap_desc->is_in_range(address);
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
