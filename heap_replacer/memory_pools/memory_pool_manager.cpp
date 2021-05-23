#include "memory_pool_manager.h"

memory_pool_manager::memory_pool_manager()
{
	util::cmemset8(this->pools_by_size, 0u, pool_size_array_length * sizeof(memory_pool*));
	util::cmemset8(this->pools_by_addr, 0u, pool_addr_array_length * sizeof(memory_pool*));
	util::cmemset8(this->pools_by_indx, 0u, pool_indx_array_length * sizeof(memory_pool*));
	this->init_all_pools();

#ifdef HR_USE_GUI
	this->allocs = 0u;
	this->frees = 0u;
#endif

}

memory_pool_manager::~memory_pool_manager()
{
	for (size_t i = 0u; i < pool_addr_array_length; i++)
	{
		if (this->pools_by_addr[i])
		{
			delete this->pools_by_addr[i];
		}
	}
}

void memory_pool_manager::init_all_pools()
{
	struct pool_data { size_t item_size; size_t max_size; } pool_desc[pool_count] =
	{
#if defined(FNV)
			{ 4u	, 4u * MB },
			{ 8u	, 32u * MB },
			{ 16u	, 64u * MB },
			{ 32u	, 128u * MB },
			{ 64u	, 64u * MB },
			{ 128u	, 256u * MB },
			{ 256u	, 128u * MB },
			{ 512u	, 128u * MB },
			{ 1024u	, 128u * MB },
			{ 2048u	, 128u * MB },
			// 1060 MB
#elif defined(FO3)
			{ 4u	, 4u * MB },
			{ 8u	, 32u * MB },
			{ 16u	, 64u * MB },
			{ 32u	, 128u * MB },
			{ 64u	, 64u * MB },
			{ 128u	, 256u * MB },
			{ 256u	, 128u * MB },
			{ 512u	, 128u * MB },
			{ 1024u	, 128u * MB },
			{ 2048u	, 128u * MB },
			// 1060 MB
#endif
	};
	for (size_t i = 0u; i < pool_count; i++)
	{
		pool_data* pd = &pool_desc[i];
		memory_pool* pool = new memory_pool(pd->item_size, pd->max_size, i);
		void* address = pool->memory_pool_init();
		this->pools_by_indx[i] = pool;
		for (size_t j = pd->item_size >> 2u; j && !this->pools_by_size[j]; j--)
		{
			this->pools_by_size[j] = pool;
		}
		for (size_t j = 0u; j < ((pd->max_size + (pool_alignment - 1u)) / pool_alignment); j++)
		{
			this->pools_by_addr[((uintptr_t)address / pool_alignment) + j] = pool;
		}
	}
}

memory_pool* memory_pool_manager::pool_from_size(size_t size)
{
	return this->pools_by_size[(size + 3u) >> 2u];
}

memory_pool* memory_pool_manager::pool_from_addr(void* address)
{
	return this->pools_by_addr[(uintptr_t)address / pool_alignment];
}

memory_pool* memory_pool_manager::pool_from_indx(size_t index)
{
	return this->pools_by_indx[index];
}

void* memory_pool_manager::malloc(size_t size)
{
#ifdef HR_ZERO_MEM
	return this->calloc(size);
#endif
#ifdef HR_USE_GUI
	this->allocs++;
#endif
	memory_pool* pool = this->pool_from_size(size);
	if (void* address = pool->malloc()) [[likely]] { return address; }
	size_t index = pool->get_index();
	while (++index < pool_count)
	{
		if (void* address = this->pool_from_indx(index)->malloc()) [[likely]] { return address; }
	}
	[[unlikely]]
	HR_PRINTF("Memory pools out of memory!");
	return nullptr;
}

void* memory_pool_manager::calloc(size_t size)
{
#ifdef HR_USE_GUI
	this->allocs++;
#endif
	memory_pool* pool = this->pool_from_size(size);
	if (void* address = pool->calloc()) [[likely]] { return address; }
	size_t index = pool->get_index();
	while (++index < pool_count)
	{
		if (void* address = this->pool_from_indx(index)->calloc()) [[likely]] { return address; }
	}
	[[unlikely]]
	HR_PRINTF("Memory pools out of memory!");
	return nullptr;
}

size_t memory_pool_manager::mem_size(void* address)
{
	memory_pool* pool = this->pool_from_addr(address);
	if (!pool) [[unlikely]] { return 0u; }
	return pool->mem_size(address);
}

bool memory_pool_manager::free(void* address)
{
	memory_pool* pool = this->pool_from_addr(address);
	if (!pool) [[unlikely]] { return false; }
	pool->free(address);
#ifdef HR_USE_GUI
	this->frees++;
#endif
	return true;
}

void* memory_pool_manager::operator new(size_t size)
{
	return hr::hr_ina_malloc(size);
}

void memory_pool_manager::operator delete(void* address)
{
	hr::hr_ina_free(address);
}
