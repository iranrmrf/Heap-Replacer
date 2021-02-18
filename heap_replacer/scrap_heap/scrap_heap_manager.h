#pragma once

#include "main/util.h"

#include "light_critical_section.h"
#include "scrap_heap_vector.h"
#include "scrap_heap_constants.h"

class scrap_heap_vector;

class scrap_heap_manager
{

private:

	struct scrap_heap_buffer { void* addr; size_t size; };

	static scrap_heap_manager* instance;

	scrap_heap_buffer buffers[scrap_heap_manager_buffer_count];
	size_t free_buffer_count;
	light_critical_section critical_section;

public:

	scrap_heap_vector* mt_sh_vector;

private:

	scrap_heap_manager();

public:

	~scrap_heap_manager();

	static scrap_heap_manager* get_instance();

	void swap_buffers(size_t index);

	void* create_buffer(size_t size);
	void* request_buffer(size_t* size);
	void free_buffer(void* address, size_t size);
	void release_buffer(void* address, size_t size);

};
