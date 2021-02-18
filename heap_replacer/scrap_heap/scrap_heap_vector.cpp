#include "scrap_heap_vector.h"

scrap_heap_vector::scrap_heap_vector(size_t size) : size(0), alloc(size)
{
	this->data = (mt_sh*)nvhr::nvhr_calloc(this->alloc, sizeof(mt_sh));
	this->lock_id = 0u;
}

scrap_heap_vector::~scrap_heap_vector()
{
	nvhr::nvhr_free(this->data);
}

void scrap_heap_vector::insert(DWORD id, scrap_heap_block* sh)
{
	LOCK(&this->lock_id);
	if (this->size >= this->alloc)
	{
		this->alloc <<= 1;
		this->data = (mt_sh*)nvhr::nvhr_realloc(this->data, this->alloc * sizeof(mt_sh));
	}
	this->data[size++] = mt_sh({ id, sh });
	UNLOCK(&this->lock_id);
}

scrap_heap_block* scrap_heap_vector::find(DWORD id)
{
	LOCK(&this->lock_id);
	for (size_t i = 0; i < this->size; i++)
	{
		if (this->data[i].id == id)
		{
			UNLOCK(&this->lock_id);
			return this->data[i].sh;
		}
	}
	UNLOCK(&this->lock_id);
	return nullptr;
}
