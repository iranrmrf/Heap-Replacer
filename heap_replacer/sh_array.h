#pragma once

#include "util.h"

struct scrap_heap;

struct mt_sh
{
	DWORD id;
	scrap_heap* sh;
};

class sh_array
{

private:

	mt_sh* data;
	size_t size;
	size_t allc;

private:

	CRITICAL_SECTION critical_section;

public:

	sh_array(size_t size)
	{
		this->size = 0;
		this->allc = size;
		this->data = (mt_sh*)nvhr_calloc(this->allc, sizeof(mt_sh));

		InitializeCriticalSectionAndSpinCount(&this->critical_section, INFINITE);
	}

	~sh_array()
	{
		nvhr_free(this->data);
		DeleteCriticalSection(&this->critical_section);
	}

	void insert(DWORD id, scrap_heap* sh)
	{
		if (this->size >= this->allc)
		{
			this->allc <<= 1;
			mt_sh* temp = (mt_sh*)nvhr_calloc(this->allc, sizeof(mt_sh));
			memcpy(temp, this->data, this->size * sizeof(mt_sh));
			nvhr_free(this->data);
			this->data = temp;
		}
		this->data[size++] = mt_sh { id, sh };
	}

	scrap_heap* find(DWORD id)
	{
		EnterCriticalSection(&this->critical_section);
		for (size_t i = 0; i < this->size; i++)
		{
			if (this->data[i].id == id)
			{
				LeaveCriticalSection(&this->critical_section);
				return this->data[i].sh;
			}
		}
		LeaveCriticalSection(&this->critical_section);
		return nullptr;
	}

	mt_sh* at(size_t index)
	{
		return (index < this->size) ? &this->data[index] : nullptr;
	}

};
