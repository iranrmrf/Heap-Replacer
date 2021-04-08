#pragma once

#include "main/util.h"

#include "light_critical_section/light_critical_section.h"
#include "scrap_heap_constants.h"

class scrap_heap_manager
{

private:

	struct scrap_heap_buffer { void* addr; size_t size; };

	static scrap_heap_manager* instance;

	scrap_heap_buffer buffers[scrap_heap_manager_buffer_count];

	size_t free_buffer_count;

#ifdef HR_USE_GUI

	size_t used_buffer_count;

	size_t used_size;
	size_t free_size;

#endif

	light_critical_section critical_section;

public:

	scrap_heap_manager();
	~scrap_heap_manager();

	void replace_with_last_buffer(size_t index);

	void* create_buffer(size_t size);
	void free_buffer(void* address, size_t size);

	void* request_buffer(size_t* size);
	void release_buffer(void* address, size_t size);

#ifdef HR_USE_GUI

public:

	size_t get_used_buffer_count() { return this->used_buffer_count; }
	size_t get_free_buffer_count() { return this->free_buffer_count; }
	size_t get_max_buffer_count() { return scrap_heap_manager_buffer_count; }
	size_t get_used_size() { return 0u; }
	size_t get_free_size() { return 0u; }

#endif

};
