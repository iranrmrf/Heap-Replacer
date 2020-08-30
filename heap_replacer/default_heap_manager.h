#pragma once

#include "list.h"
#include "mem_cell.h"
#include "mem_bank.h"

#include "util.h"

#define CELL_SIZE 4 * KB
#define COMMIT_SIZE 64 * MB
#define RESERVE_SIZE 256 * MB
#define MAX_HEAP_SIZE 2 * GB

class default_heap_manager
{

private:

	mem_bank* bank;

	CRITICAL_SECTION critical_section;

public:

	default_heap_manager()
	{
		this->bank = new mem_bank(CELL_SIZE, COMMIT_SIZE, RESERVE_SIZE, MAX_HEAP_SIZE);

		InitializeCriticalSectionAndSpinCount(&this->critical_section, INFINITE);
	}

	~default_heap_manager()
	{

	}

public:

	void* malloc(size_t size)
	{
		EnterCriticalSection(&this->critical_section);
		mem_cell* cell = this->bank->get_free_cell(size);
		LeaveCriticalSection(&this->critical_section);
		if (!cell) { return nullptr; }
		return cell->desc.addr;
	}

	void* calloc(size_t size)
	{
		void* address = this->malloc(size);
		memset(address, 0, size);
		return address;
	}

	size_t mem_size(void* address)
	{
		mem_cell* cell;
		return (cell = this->bank->get_used(address)) ? cell->desc.size : 0;
	}

	bool free(void* address)
	{
		EnterCriticalSection(&this->critical_section);
		mem_cell* cell;
		if (cell = this->bank->rmv_used(address))
		{
			this->bank->add_free_cell(cell);
		}
		LeaveCriticalSection(&this->critical_section);
		return true;
	}

};
