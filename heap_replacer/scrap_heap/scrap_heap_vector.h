#pragma once

#include "main/util.h"

#include "scrap_heap_block.h"

class scrap_heap_block;

class scrap_heap_vector
{

private:

	struct mt_sh { DWORD id; scrap_heap_block* sh; };

	size_t size;
	size_t alloc;
	mt_sh* data;

private:

	DWORD lock_id;

public:

	scrap_heap_vector(size_t size);
	~scrap_heap_vector();

	void insert(DWORD id, scrap_heap_block* sh);

	scrap_heap_block* find(DWORD id);

};
