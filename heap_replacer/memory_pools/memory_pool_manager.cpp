#include "memory_pool_manager.h"

memory_pool_manager::memory_pool_manager()
{
	util::memset8(this->pools_by_size, 0u, pool_size_array_length * sizeof(memory_pool*));
	util::memset8(this->pools_by_addr, 0u, pool_addr_array_length * sizeof(memory_pool*));
	this->init_all_pools();
}

memory_pool_manager::~memory_pool_manager()
{
	for (size_t i = 0; i < pool_addr_array_length; i++)
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
			{ 4		, 0x01000000u },
			{ 8		, 0x04000000u },
			{ 16	, 0x04000000u },
			{ 32	, 0x08000000u },
			{ 64	, 0x04000000u },
			{ 128	, 0x10000000u },
			{ 256	, 0x08000000u },
			{ 512	, 0x04000000u },
			{ 1024	, 0x08000000u },
			{ 2048	, 0x08000000u },
#elif defined(FO3)
			{ 4		, 0x01000000u },
			{ 8		, 0x04000000u },
			{ 16	, 0x04000000u },
			{ 32	, 0x08000000u },
			{ 64	, 0x04000000u },
			{ 128	, 0x10000000u },
			{ 256	, 0x08000000u },
			{ 512	, 0x04000000u },
			{ 1024	, 0x08000000u },
			{ 2048	, 0x08000000u },
#endif
	};
	for (size_t i = 0; i < pool_count; i++)
	{
		pool_data* pd = &pool_desc[i];
		memory_pool* pool = new memory_pool(pd->item_size, pd->max_size);
		void* address = pool->memory_pool_init();
		this->pools_by_size[util::get_highest_bit(pd->item_size)] = pool;
		for (size_t j = 0; j < ((pd->max_size + (pool_alignment - 1)) / pool_alignment); j++)
		{
			this->pools_by_addr[((uintptr_t)address / pool_alignment) + j] = pool;
		}
	}
}

memory_pool* memory_pool_manager::pool_from_size(size_t size)
{
	return this->pools_by_size[util::get_highest_bit(size)];
}

memory_pool* memory_pool_manager::pool_from_addr(void* address)
{
	return this->pools_by_addr[(uintptr_t)address / pool_alignment];
}

void* memory_pool_manager::malloc(size_t size)
{
	for (size = util::round_pow2(size); size <= 2 * KB; size <<= 1)
	{
		if (void* address = this->pool_from_size(size)->malloc()) [[likely]] { return address; }
	}
	[[unlikely]]
	return nullptr;
}

void* memory_pool_manager::calloc(size_t size)
{
	for (size = util::round_pow2(size); size <= 2 * KB; size <<= 1)
	{
		if (void* address = this->pool_from_size(size)->calloc()) [[likely]] { return address; }
	}
	[[unlikely]]
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
	return true;
}

void* memory_pool_manager::operator new(size_t size)
{
	return nvhr::nvhr_ina_malloc(size);
}

void memory_pool_manager::operator delete(void* address)
{
	nvhr::nvhr_ina_free(address);
}
