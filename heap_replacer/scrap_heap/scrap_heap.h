#pragma once

#include "main/util.h"

#include "scrap_heap_constants.h"

class scrap_heap
{

private:

	struct sh_chunk { size_t size; sh_chunk* prev; };

	void* sh_bgn;
	void* sh_cur;
	void* sh_end;
	sh_chunk* last;

public:
	
	scrap_heap();
	~scrap_heap();

	void init();
	void init_0x10000();
	void init_var(size_t size);

	void* alloc(size_t size, size_t alignment);
	void free(void* address);
	void purge();

	static scrap_heap* get_thread_scrap_heap();

};
