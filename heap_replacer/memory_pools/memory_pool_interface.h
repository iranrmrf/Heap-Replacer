#pragma once

class memory_pool_interface
{

public:

	virtual void* memory_pool_init() = 0;

public:

	virtual void* malloc() = 0;
	virtual void* calloc() = 0;
	virtual size_t mem_size(void* address) = 0;
	virtual void free(void* address) = 0;

public:

	virtual size_t item_size() = 0;
	virtual size_t max_size() = 0;

};
