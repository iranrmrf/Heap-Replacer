#pragma once

#define POOL_ALIGNMENT	0x01000000u
#define POOL_GROWTH		0x00010000u

#define POOL_COUNT 10u
#define POOL_SIZE_ARRAY_LEN ((2048 >> 2) + 1)
#define POOL_ADDR_ARRAY_LEN ((0x80000000u / POOL_ALIGNMENT) << 1)

#include "main/util.h"

#include "memory_pool.h"

class memory_pool_manager
{

private:

	memory_pool* pools_by_size[POOL_SIZE_ARRAY_LEN];
	memory_pool* pools_by_addr[POOL_ADDR_ARRAY_LEN];

public:

	memory_pool_manager()
	{
		memset(this->pools_by_size, 0, POOL_SIZE_ARRAY_LEN * sizeof(memory_pool*));
		memset(this->pools_by_addr, 0, POOL_ADDR_ARRAY_LEN * sizeof(memory_pool*));
		this->init_all_pools();
	}

	~memory_pool_manager()
	{
		for (size_t i = 0; i < POOL_ADDR_ARRAY_LEN; i++)
		{
			if (this->pools_by_addr[i])
			{
				delete this->pools_by_addr[i];
			}
		}
	}

private:

	void init_all_pools()
	{
		struct pool_data { size_t item_size; size_t max_size; } pool_desc[POOL_COUNT] =
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
		for (size_t i = 0; i < POOL_COUNT; i++)
		{
			pool_data* pd = &pool_desc[i];
			memory_pool* pool = new memory_pool(pd->item_size, pd->max_size);
			void* address = pool->memory_pool_init();
			this->pools_by_size[pd->item_size >> 2] = pool;
			for (size_t j = 0; j < ((pd->max_size + (POOL_ALIGNMENT - 1)) / POOL_ALIGNMENT); j++)
			{
				this->pools_by_addr[((uintptr_t)address / POOL_ALIGNMENT) + j] = pool;
			}
		}
	}

	size_t next_power_of_2(size_t size)
	{
		if (size & (size - 1))
		{
			size--;
			size |= size >> 1;
			size |= size >> 2;
			size |= size >> 4;
			size |= size >> 8;
			size |= size >> 16;
			size++;
		}
		return size;
	}

	memory_pool* pool_from_size(size_t size)
	{
		return this->pools_by_size[size >> 2];
	}

	memory_pool* pool_from_addr(void* address)
	{
		return this->pools_by_addr[(uintptr_t)address / POOL_ALIGNMENT];
	}

public:

	void* malloc(size_t size)
	{
		for (size = this->next_power_of_2(size); size <= 2 * KB; size <<= 1)
		{
			if (void* address = this->pool_from_size(size)->malloc()) [[likely]] { return address; }
		}
		[[unlikely]]
		return nullptr;
	}

	void* calloc(size_t size)
	{
		for (size = this->next_power_of_2(size); size <= 2 * KB; size <<= 1)
		{
			if (void* address = this->pool_from_size(size)->calloc()) [[likely]] { return address; }
		}
		[[unlikely]]
		return nullptr;
	}

	size_t mem_size(void* address)
	{
		memory_pool* pool = this->pool_from_addr(address);
		if (!pool) [[unlikely]] { return 0; }
		return pool->mem_size(address);
	}

	bool free(void* address)
	{
		memory_pool* pool = this->pool_from_addr(address);
		if (!pool) [[unlikely]] { return false; }
		pool->free(address);
		return true;
	}

	void* operator new(size_t size)
	{
		return ina.malloc(size);
	}

	void operator delete(void* address)
	{
		ina.free(address);
	}

};
