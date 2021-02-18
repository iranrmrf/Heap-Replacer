#pragma once

#include "main/util.h"

#include "scrap_heap_manager.h"
#include "scrap_heap_constants.h"

class scrap_heap_block
{

private:

	struct scrap_heap_chunk { size_t size; scrap_heap_chunk* prev_chunk; };

	void* commit_bgn;
	void* unused;
	void* commit_end;
	scrap_heap_chunk* last_chunk;

public:
	
	void init(size_t size);
	void init_0x10000();
	void init_var(size_t size);

	void* alloc(size_t size, size_t alignment);
	void free(void* address);
	void purge();

	static scrap_heap_block* get_thread_scrap_heap();

};
