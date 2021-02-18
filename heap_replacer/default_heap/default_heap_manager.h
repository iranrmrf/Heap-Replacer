#pragma once

#include "main/util.h"

#include "default_heap.h"

class default_heap_manager
{

private:

	default_heap* heap;
	size_t used_size;

public:

	default_heap_manager();
	~default_heap_manager();

public:

	void* malloc(size_t size);
	void* calloc(size_t size);
	size_t mem_size(void* address);
	bool free(void* address);

};
