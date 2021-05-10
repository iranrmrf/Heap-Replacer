#pragma once

#include "main/util.h"

#include "default_heap_constants.h"

class cell_desc
{

public:

	void* addr;
	size_t size : size_bits, index : index_bits;

public:

	void* get_end();

	bool is_in_range(void* address);

};
