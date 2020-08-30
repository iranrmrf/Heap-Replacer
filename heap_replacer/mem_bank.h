#pragma once

#include "list.h"
#include "mem_cell.h"

#include "util.h"

class mem_bank
{

private:

	list<mem_cell*>* free_size;
	list<mem_cell*>* free_addr;

	list<mem_cell*>* dead_size;
	list<mem_cell*>* dead_addr;

	mem_cell** cache_array;

private:

	size_t item_size;
	size_t commit_size;
	size_t reserve_size;
	size_t max_size;

private:

	size_t max_desc;
	size_t desc_item_count;
	cell_desc** used_cell_desc;
	mem_cell*** used_mem_cells;

public:

	mem_bank(size_t item_size, size_t commit_size, size_t reserve_size, size_t max_size)
		: item_size(item_size), commit_size(commit_size), reserve_size(reserve_size), max_size(max_size)
	{
		this->free_size = new list<mem_cell*>();
		this->free_addr = new list<mem_cell*>();

		this->dead_size = new list<mem_cell*>();
		this->dead_addr = new list<mem_cell*>();

		this->max_desc = this->max_size / this->reserve_size;
		this->desc_item_count = this->reserve_size / this->item_size;

		this->used_cell_desc = (cell_desc**)winapi_alloc(this->max_desc * sizeof(cell_desc*));
		this->used_mem_cells = (mem_cell***)winapi_alloc(this->max_desc * sizeof(mem_cell**));

		this->cache_array = (mem_cell**)winapi_alloc(this->reserve_size / this->item_size * sizeof(mem_cell*));
	}

	~mem_bank()
	{

	}

	void add_cache_array(mem_cell* cell)
	{
		int index = (cell->desc.size / this->item_size) - 1;
		while (index >= 0)
		{
			mem_cell* elem = this->cache_array[index];	
			if ((elem && (cell->desc.size >= elem->desc.size) && ((cell->desc.size != elem->desc.size) || (cell->desc.addr >= elem->desc.addr)))) { return; }
			this->cache_array[index--] = cell;
		}
	}

	void rmv_cache_array(mem_cell* cell)
	{
		int index, pos;
		if (cell != this->cache_array[index = (cell->desc.size / this->item_size) - 1]) { return; }
		mem_cell* data = cell->size_node->next ? cell->size_node->next->data : nullptr;
		for (pos = index; (pos >= 0) && (this->cache_array[pos] == cell); this->cache_array[pos--] = data);
		for (pos = index + 1; (pos < (int)(this->reserve_size / this->item_size)) && (this->cache_array[pos] == cell); this->cache_array[pos++] = data);
	}

	node<mem_cell*>* best_free_fit(size_t size)
	{		
		if (auto elem = this->cache_array[(size / this->item_size) - 1]) { return elem->size_node; }
		for (auto curr = this->free_size->get_head(); curr; curr = curr->next)
		{
			if (curr->data->desc.size >= size) { return curr; }
		}
		return nullptr;
	}

	node<mem_cell*>* best_dead_fit(size_t size)
	{
		for (auto curr = this->dead_size->get_head(); curr; curr = curr->next)
		{
			if (curr->data->desc.size >= size) { return curr; }
		}
		return nullptr;
	}

	node<mem_cell*>* insert_free_size(mem_cell* cell)
	{
		mem_cell* elem = this->cache_array[(cell->desc.size / this->item_size) - 1];
		for (auto curr = elem ? elem->size_node : this->free_size->get_head(); curr; curr = curr->next)
		{
			if (((curr->data->desc.size == cell->desc.size) && (curr->data->desc.addr > cell->desc.addr)) || (curr->data->desc.size > cell->desc.size))
			{
				return this->free_size->insert_before(curr, cell);
			}		
		}
		return this->free_size->add_tail(cell);
	}

	node<mem_cell*>* insert_free_addr(mem_cell* cell)
	{
		for (auto curr = this->free_addr->get_head(); curr; curr = curr->next)
		{
			if (((curr->data->desc.size == cell->desc.size) && (curr->data->desc.addr > cell->desc.addr)) || (curr->data->desc.size > cell->desc.size))
			{
				return this->free_addr->insert_before(curr, cell);
			}	
		}
		return this->free_addr->add_tail(cell);
	}

	node<mem_cell*>* insert_dead_size(mem_cell* cell)
	{
		for (auto curr = this->dead_size->get_head(); curr; curr = curr->next)
		{
			if (curr->data->desc.size > cell->desc.size)
			{
				return this->dead_size->insert_before(curr, cell);
			}
		}
		return this->dead_size->add_tail(cell);
	}

	node<mem_cell*>* insert_dead_addr(mem_cell* cell)
	{
		for (auto curr = this->dead_addr->get_head(); curr; curr = curr->next)
		{
			if (curr->data->desc.size > cell->desc.size)
			{
				return this->dead_addr->insert_before(curr, cell);
			}
		}
		return this->dead_addr->add_tail(cell);
	}

	void rmv_free_cell(mem_cell* cell)
	{
		this->free_size->remove_node(cell->size_node);
		this->free_addr->remove_node(cell->addr_node);
	}

	void rmv_dead_cell(mem_cell* cell)
	{
		this->dead_size->remove_node(cell->size_node);
		this->dead_addr->remove_node(cell->addr_node);
	}

	void add_free_cell(mem_cell* cell)
	{
		if (this->free_is_empty())
		{
			cell->size_node = this->free_size->add_head(cell);
			cell->addr_node = this->free_addr->add_head(cell);
		}
		else
		{
			cell->addr_node = this->insert_free_addr(cell);
			if (cell->addr_node->next)
			{
				mem_cell* next_data = cell->addr_node->next->data;
				if (cell->addr_node->data->is_adjacent_to(next_data) && (this->get_desc(cell->desc.addr) == this->get_desc(next_data->desc.addr)))
				{
					this->rmv_cache_array(next_data);
					cell->addr_node->data->join(next_data);
					this->rmv_free_cell(next_data);
					delete next_data;
				}
			}
			if (cell->addr_node->prev)
			{
				mem_cell* prev_data = cell->addr_node->prev->data;
				if (cell->addr_node->data->is_adjacent_to(prev_data) && (this->get_desc(cell->desc.addr) == this->get_desc(prev_data->desc.addr)))
				{
					this->rmv_cache_array(prev_data);
					cell->addr_node->data->join(prev_data);
					this->rmv_free_cell(prev_data);
					delete prev_data;
				}
			}
			cell->size_node = this->insert_free_size(cell);			
		}
		this->add_cache_array(cell);
	}

	void add_dead_cell(mem_cell* cell)
	{
		if (this->dead_is_empty())
		{
			cell->size_node = this->dead_size->add_head(cell);
			cell->addr_node = this->dead_addr->add_head(cell);
		}
		else
		{
			cell->addr_node = this->insert_dead_addr(cell);
			if (cell->addr_node->next)
			{
				mem_cell* next_data = cell->addr_node->next->data;
				if (cell->addr_node->data->is_adjacent_to(next_data) && (this->get_desc(cell->desc.addr) == this->get_desc(next_data->desc.addr)))
				{
					cell->addr_node->data->join(next_data);
					this->rmv_dead_cell(next_data);
					delete next_data;
				}
			}
			if (cell->addr_node->prev)
			{
				mem_cell* prev_data = cell->addr_node->prev->data;
				if (cell->addr_node->data->is_adjacent_to(prev_data) && (this->get_desc(cell->desc.addr) == this->get_desc(prev_data->desc.addr)))
				{
					cell->addr_node->data->join(prev_data);
					this->rmv_dead_cell(prev_data);
					delete prev_data;
				}
			}
			cell->size_node = this->insert_dead_size(cell);
		}
	}

	mem_cell* get_free_cell(size_t size)
	{
		if (size & (this->item_size - 1))
		{
			size += (this->item_size - 1);
			size &= ~(this->item_size - 1);
		}
		node<mem_cell*>* curr;
		if (this->free_is_empty())
		{
			mem_cell* cell = this->get_dead_cell(size);
			this->add_free_cell(cell);
			curr = this->free_size->get_head();
		}
		else if (this->free_size->get_tail()->data->desc.size < size)
		{
			do
			{
				mem_cell* cell = this->get_dead_cell(size);
				this->add_free_cell(cell);
				curr = this->free_size->get_tail();
			}
			while (curr->data->desc.size < size);
		}
		else
		{
			curr = this->best_free_fit(size);
		}
		mem_cell* cell;
		if ((curr->data->desc.size - size) < this->item_size)
		{
			this->rmv_cache_array(curr->data);
			this->rmv_free_cell(cell = curr->data);
		}
		else
		{
			this->rmv_cache_array(curr->data);
			cell = curr->data->split(size);
			mem_cell* old_cell = curr->data;
			this->free_size->remove_node(curr->data->size_node);
			old_cell->size_node = this->insert_free_size(old_cell);
			this->add_cache_array(old_cell);
		}
		this->add_used(cell);
		return cell;
	}

	mem_cell* get_dead_cell(size_t size)
	{
		if (size & (this->commit_size - 1))
		{
			size += (this->commit_size - 1);
			size &= ~(this->commit_size - 1);
		}
		node<mem_cell*>* curr;
		if (this->dead_is_empty())
		{
			mem_cell* cell = this->reserve(size);
			this->add_dead_cell(cell);
			curr = this->dead_size->get_head();
		}
		else if (this->dead_size->get_tail()->data->desc.size < size)
		{
			do
			{
				mem_cell* cell = this->reserve(size);
				this->add_dead_cell(cell);
				curr = this->dead_size->get_tail();
			}
			while (curr->data->desc.size < size);
		}
		else
		{
			curr = this->best_dead_fit(size);
		}
		mem_cell* cell;
		if ((curr->data->desc.size - size) < this->commit_size)
		{
			this->rmv_dead_cell(cell = curr->data);
		}
		else
		{
			cell = curr->data->split(size);
			mem_cell* old_cell = curr->data;
			this->rmv_dead_cell(curr->data);
			this->add_dead_cell(old_cell);
		}
		this->commit(cell);
		return cell;
	}

	void commit(mem_cell* cell)
	{
		if (!VirtualAlloc(cell->desc.addr, cell->desc.size, MEM_COMMIT, PAGE_READWRITE))
		{
			MessageBox(NULL, "VirtualAlloc commit fail!", "Error", NULL);
		}
	}

	mem_cell* reserve(size_t size)
	{
		void* address;
		if (!(address = VirtualAlloc(nullptr, this->reserve_size, MEM_RESERVE, PAGE_READWRITE)))
		{
			MessageBox(NULL, "VirtualAlloc reserve fail!", "Error", NULL);
			return nullptr;
		}
		for (size_t i = 0; i < this->max_desc; i++)
		{
			if (!this->used_cell_desc[i])
			{
				this->used_cell_desc[i] = new cell_desc(address, this->reserve_size);;
				this->used_mem_cells[i] = (mem_cell**)winapi_alloc(this->desc_item_count * sizeof(mem_cell*));
				return new mem_cell(address, this->reserve_size);
			}
		}
		MessageBox(NULL, "No more descriptors!", "Error", NULL);
		return nullptr;
	}

	size_t get_desc(void* address)
	{
		for (size_t i = 0; i < this->max_desc; i++)
		{
			if (this->used_cell_desc[i] && this->used_cell_desc[i]->is_in_range(address)) { return i; }
		}
		return -1;
	}

	void add_used(mem_cell* cell)
	{
		size_t desc = this->get_desc(cell->desc.addr);
		this->used_mem_cells[desc][((uintptr_t)cell->desc.addr - (uintptr_t)this->used_cell_desc[desc]->addr) / this->item_size] = cell;
	}

	mem_cell* get_used(void* address)
	{
		size_t desc = this->get_desc(address);
		return this->used_mem_cells[desc][((uintptr_t)address - (uintptr_t)this->used_cell_desc[desc]->addr) / this->item_size];
	}

	mem_cell* rmv_used(void* address)
	{
		size_t desc = this->get_desc(address);
		mem_cell* cell = this->used_mem_cells[desc][((uintptr_t)address - (uintptr_t)this->used_cell_desc[desc]->addr) / this->item_size];
		this->used_mem_cells[desc][((uintptr_t)address - (uintptr_t)this->used_cell_desc[desc]->addr) / this->item_size] = nullptr;
		return cell;
	}

	bool free_is_empty()
	{
		return (this->free_size->is_empty() && this->free_addr->is_empty());
	}

	bool dead_is_empty()
	{
		return (this->dead_size->is_empty() && this->dead_addr->is_empty());
	}

};
