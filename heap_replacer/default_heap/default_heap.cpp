#include "default_heap.h"

default_heap::default_heap()
{
	this->size_clist = new cell_list();
	this->addr_clist = (cell_list**)util::winapi_calloc(default_heap_block_count, sizeof(cell_list*));

	this->size_array = (mem_cell**)util::winapi_calloc(default_heap_cell_count, sizeof(mem_cell*));
	this->addr_array = (mem_cell***)util::winapi_calloc(default_heap_block_count, sizeof(mem_cell**));

	this->block_desc = (cell_desc*)util::winapi_calloc(default_heap_block_count, sizeof(cell_desc));

	this->used_cells = (volatile size_t**)util::winapi_calloc(default_heap_block_count, sizeof(volatile size_t*));

	InitializeCriticalSectionEx(&this->critical_section, ~RTL_CRITICAL_SECTION_ALL_FLAG_BITS, RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO);
}

default_heap::~default_heap()
{
	delete this->size_clist;

	for (size_t i = 0; i < default_heap_block_count; i++)
	{
		delete this->addr_clist[i];
	}
	util::winapi_free(this->addr_clist);

	util::winapi_free(this->size_array);

	for (size_t i = 0; i < default_heap_block_count; i++)
	{
		for (size_t j = 0; j < default_heap_cell_count; j++)
		{
			delete this->addr_array[i][j];
		}
		util::winapi_free(this->addr_array[i]);
	}
	util::winapi_free(this->addr_array);

	util::winapi_free(this->block_desc);

	for (size_t i = 0; i < default_heap_block_count; i++)
	{
		util::winapi_free((void*)this->used_cells[i]);
	}
	util::winapi_free(this->used_cells);

	DeleteCriticalSection(&this->critical_section);
}

size_t default_heap::get_size_index(size_t size)
{
	return (size / default_heap_cell_size) - 1;
}

size_t default_heap::get_size_index(mem_cell* cell)
{
	return this->get_size_index(cell->desc.size);
}

size_t default_heap::get_addr_index(void* address)
{
	return UPTRDIFF(address, this->block_desc[this->get_block_index(address)].addr) / default_heap_cell_size;
}

size_t default_heap::get_addr_index(mem_cell* cell)
{
	return UPTRDIFF(cell->desc.addr, this->block_desc[cell->desc.index].addr) / default_heap_cell_size;
}

void default_heap::add_size_array(mem_cell* cell)
{
	cell_node* curr;
	for (curr = cell->size_node->prev; curr->is_valid() && cell->swap_by_size(curr->cell); curr = curr->prev);
	for (size_t i = 0u; i < cell->size_node->array_index - curr->array_index; this->size_array[curr->array_index + 1 + i++] = cell);
}

void default_heap::add_addr_array(mem_cell* cell)
{
	cell_node* curr;
	for (curr = cell->addr_node->prev; curr->is_valid() && cell->swap_by_addr(curr->cell); curr = curr->prev);
	for (size_t i = 0u; i < cell->addr_node->array_index - curr->array_index; this->addr_array[cell->desc.index][curr->array_index + 1 + i++] = cell);
}

void default_heap::rmv_size_array(mem_cell* cell)
{
	cell_node* curr = cell->size_node->prev;
	for (size_t i = 0u; i < cell->size_node->array_index - curr->array_index; this->size_array[curr->array_index + 1 + i++] = cell->size_node->next->cell);
}

void default_heap::rmv_addr_array(mem_cell* cell)
{
	cell_node* curr = cell->addr_node->prev;
	for (size_t i = 0u; i < cell->addr_node->array_index - curr->array_index; this->addr_array[cell->desc.index][curr->array_index + 1 + i++] = cell->addr_node->next->cell);
}

cell_node* default_heap::insert_size_dlist(mem_cell* cell)
{
	mem_cell* elem = this->size_array[this->get_size_index(cell)];
	if (!elem) [[unlikely]] { return this->size_clist->add_tail(cell); }
	cell_node* curr;
	for (curr = elem->size_node; curr->is_valid() && !cell->swap_by_size(curr->cell); curr = curr->next);
	return this->size_clist->insert_before(curr, cell);
}

cell_node* default_heap::insert_addr_dlist(mem_cell* cell)
{
	mem_cell* elem = this->addr_array[cell->desc.index][this->get_addr_index(cell)];
	if (!elem) [[unlikely]] { return this->addr_clist[cell->desc.index]->add_tail(cell); }
	cell_node* curr;
	for (curr = elem->addr_node; curr->is_valid() && !cell->swap_by_addr(curr->cell); curr = curr->next);
	return this->addr_clist[cell->desc.index]->insert_before(curr, cell);
}

void default_heap::add_free_cell_size(mem_cell* cell)
{
	cell->size_node = this->insert_size_dlist(cell);
	cell->size_node->array_index = this->get_size_index(cell);
}

void default_heap::add_free_cell_addr(mem_cell* cell)
{
	cell->addr_node = this->insert_addr_dlist(cell);
	cell->addr_node->array_index = this->get_addr_index(cell);
}

void default_heap::rmv_free_cell(mem_cell* cell)
{
	this->rmv_size_array(cell);
	this->rmv_addr_array(cell);
	this->size_clist->remove_node(cell->size_node);
	this->addr_clist[cell->desc.index]->remove_node(cell->addr_node);
}

void default_heap::add_free_cell(mem_cell* cell)
{
	EnterCriticalSection(&this->critical_section);
	this->add_free_cell_addr(cell);
	mem_cell* temp;
	if ((temp = cell->addr_node->prev->cell) && temp->precedes(cell)) [[unlikely]]
	{
		this->rmv_free_cell(temp);
		cell->join(temp);
		cell->addr_node->array_index = this->get_addr_index(cell);
		delete temp;
	}
	if ((temp = cell->addr_node->next->cell) && cell->precedes(temp)) [[unlikely]]
	{
		this->rmv_free_cell(temp);
		cell->join(temp);
		//cell->addr_node->array_index = this->get_addr_index(cell);
		delete temp;
	}
	this->add_addr_array(cell);
	this->add_free_cell_size(cell);
	this->add_size_array(cell);
	LeaveCriticalSection(&this->critical_section);
}

mem_cell* default_heap::get_free_cell(size_t size)
{
	size = util::align<default_heap_cell_size>(size);
	mem_cell* cell;
	EnterCriticalSection(&this->critical_section);
	while (!(cell = this->size_array[this->get_size_index(size)])) [[unlikely]]
	{
		this->add_free_cell(this->create_new_block());
	}
	if (cell->desc.size == size)
	{
		this->rmv_free_cell(cell);
	}
	else
	{
		this->rmv_size_array(cell);
		this->size_clist->remove_node(cell->size_node);
		mem_cell* split = cell->split(size);
		this->add_free_cell_size(cell);
		this->add_size_array(cell);
		cell = split;
	}
	LeaveCriticalSection(&this->critical_section);
	return cell;
}

size_t default_heap::get_block_index(void* address)
{
	for (size_t i = 0u; i < default_heap_block_count; i++)
	{
		if (this->block_desc[i].is_in_range(address)) { return i; }
	}
	return -1;
}

size_t default_heap::get_block_index(mem_cell* cell)
{
	return this->get_block_index(cell->desc.addr);
}

void default_heap::add_used(mem_cell* cell)
{
	InterlockedExchange(&this->used_cells[cell->desc.index][this->get_addr_index(cell)], cell->desc.size);
}

size_t default_heap::rmv_used(void* address, size_t index)
{
	return InterlockedExchange(&this->used_cells[index][this->get_addr_index(address)], 0u);
}

size_t default_heap::get_used(void* address, size_t index)
{
	return this->used_cells[index][this->get_addr_index(address)];
}

size_t default_heap::get_free_block_index(void* address)
{
	for (size_t i = 0u; i < default_heap_block_count; i++)
	{
		if (!this->block_desc[i].addr) { return i; }
	}
	return -1;
}

mem_cell* default_heap::create_new_block()
{
	void* address = VirtualAlloc(nullptr, default_heap_block_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	size_t index = this->get_free_block_index(address);
	if (index == -1) { HR_MSGBOX("ERROR!"); return nullptr; }

	this->block_desc[index] = { address, default_heap_block_size, index };
	this->addr_clist[index] = new cell_list();
	this->addr_array[index] = (mem_cell**)util::winapi_calloc(default_heap_cell_count, sizeof(mem_cell*));
	this->used_cells[index] = (size_t*)util::winapi_calloc(default_heap_cell_count, sizeof(size_t));

	printf("%p, %08X, %d\n", address, default_heap_block_size, index);

	return new mem_cell(address, default_heap_block_size, index);
}
