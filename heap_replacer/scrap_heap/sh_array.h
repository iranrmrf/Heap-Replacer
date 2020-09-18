#pragma once

#include "main/util.h"

struct scrap_heap;

struct mt_sh
{
	DWORD id;
	scrap_heap* sh;

	void* operator new(size_t size) { return nvhr_malloc(size); }
	void operator delete(void* address) { nvhr_free(address); }
};

class sh_array
{

private:

	mt_sh* data;
	size_t size;
	size_t alloc;

private:

	CRITICAL_SECTION critical_section;

public:

	sh_array(size_t size)
	{
		this->size = 0;
		this->alloc = size;
		this->data = (mt_sh*)nvhr_calloc(this->alloc, sizeof(mt_sh));

		InitializeCriticalSectionEx(&this->critical_section, ~RTL_CRITICAL_SECTION_ALL_FLAG_BITS, RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO);
	}

	~sh_array()
	{
		nvhr_free(this->data);
		DeleteCriticalSection(&this->critical_section);
	}

	void insert(DWORD id, scrap_heap* sh)
	{
		EnterCriticalSection(&this->critical_section);
		if (this->size >= this->alloc)
		{
			this->alloc <<= 1;
			mt_sh* temp = (mt_sh*)nvhr_realloc(this->data, this->alloc * sizeof(mt_sh));
			memmove(temp, this->data, this->size * sizeof(mt_sh));
			nvhr_free(this->data);
			this->data = temp;
		}
		this->data[size++] = mt_sh { id, sh };
		LeaveCriticalSection(&this->critical_section);
	}

	scrap_heap* find(DWORD id)
	{
		for (size_t i = 0; i < this->size; i++)
		{
			if (this->data[i].id == id)
			{
				return this->data[i].sh;
			}
		}
		return nullptr;
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
