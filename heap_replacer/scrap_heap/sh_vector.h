#pragma once

#include "main/util.h"

namespace ScrapHeap
{

	struct scrap_heap;

	struct mt_sh
	{
		DWORD id;
		scrap_heap* sh;

		void* operator new(size_t size) { return NVHR::nvhr_malloc(size); }
		void operator delete(void* address) { NVHR::nvhr_free(address); }
	};

	class sh_vector
	{

	private:

		size_t size;
		size_t alloc;
		mt_sh* data;

	private:

		CRITICAL_SECTION critical_section;

	public:

		sh_vector(size_t size) : size(0), alloc(size)
		{
			this->data = (mt_sh*)NVHR::nvhr_calloc(this->alloc, sizeof(mt_sh));
			InitializeCriticalSectionEx(&this->critical_section, ~RTL_CRITICAL_SECTION_ALL_FLAG_BITS, RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO);
		}

		~sh_vector()
		{
			NVHR::nvhr_free(this->data);
			DeleteCriticalSection(&this->critical_section);
		}

		void insert(DWORD id, scrap_heap* sh)
		{
			ECS(&this->critical_section);
			if (this->size >= this->alloc)
			{
				this->alloc <<= 1;
				mt_sh* temp = (mt_sh*)NVHR::nvhr_realloc(this->data, this->alloc * sizeof(mt_sh));
				memmove(temp, this->data, this->size * sizeof(mt_sh));
				NVHR::nvhr_free(this->data);
				this->data = temp;
			}
			this->data[size++] = mt_sh{ id, sh };
			LCS(&this->critical_section);
		}

		scrap_heap* find(DWORD id)
		{
			ECS(&this->critical_section);
			for (size_t i = 0; i < this->size; i++)
			{
				if (this->data[i].id == id)
				{
					LCS(&this->critical_section);
					return this->data[i].sh;
				}
			}
			LCS(&this->critical_section);
			return nullptr;
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

}
