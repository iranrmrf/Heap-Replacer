#pragma once

#include "definitions.h"

class memory_pool_manager;
class default_heap_manager;
#ifdef HR_USE_GUI
class ui;
#endif

namespace hr
{

	memory_pool_manager* get_mpm();
	default_heap_manager* get_dhm();
#ifdef HR_USE_GUI
	ui* get_uim();
#endif

	void* hr_ina_malloc(size_t size);
	void* hr_ina_calloc(size_t count, size_t size);
	size_t hr_ina_mem_size(void* address);
	void hr_ina_free(void* address);
	void* hr_malloc(size_t size);
	void* hr_calloc(size_t count, size_t size);
	void* hr_realloc(void* address, size_t size);
	void* hr_recalloc(void* address, size_t count, size_t size);
	size_t hr_mem_size(void* address);
	void hr_free(void* address);

}
