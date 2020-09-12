#pragma once

#define POOL_GROWTH 0x00010000
#define POOL_ALIGNMENT 0x01000000

#define POOL_COUNT 10
#define POOL_ARRAY_SIZE ((0x80000000 / POOL_ALIGNMENT) << 1)

#include "util.h"

#include "memory_pool.h"

class memory_pools_manager
{

private:

	memory_pool_container* pools_by_size[POOL_ARRAY_SIZE];
	memory_pool_container* pools_by_addr[POOL_ARRAY_SIZE];

public:

	memory_pools_manager()
	{
		memset(this->pools_by_size, 0, POOL_ARRAY_SIZE * sizeof(memory_pool_container*));
		memset(this->pools_by_addr, 0, POOL_ARRAY_SIZE * sizeof(memory_pool_container*));
		this->init_all_pools();
	}

	~memory_pools_manager()
	{

	}

private:

	void init_all_pools()
	{
		//struct pool_data { size_t item_size; size_t pool_size; };

		memory_pool_container* pools[POOL_COUNT] =
		{
			new memory_pool<4, 0x01000000u>(),
			new memory_pool<8, 0x04000000u>(),
			new memory_pool<16, 0x04000000u>(),
			new memory_pool<32, 0x08000000u>(),
			new memory_pool<64, 0x04000000u>(),
			new memory_pool<128, 0x10000000u>(),
			new memory_pool<256, 0x08000000u>(),
			new memory_pool<512, 0x04000000u>(),
			new memory_pool<1024, 0x08000000u>(),
			new memory_pool<2048, 0x08000000u>()
		};

		for (size_t i = 0; i < POOL_COUNT; i++)
		{
			memory_pool_container* pool = pools[i];
			void* address = pool->memory_pool_init();
			this->pools_by_size[(pool->item_size() >> 2) - 1] = pool;
			for (size_t j = 0; j < ((pool->max_size() + (POOL_ALIGNMENT - 1)) / POOL_ALIGNMENT); j++)
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

	memory_pool_container* pool_from_size(size_t size)
	{
		return this->pools_by_size[(size >> 2) - 1];
	}

	memory_pool_container * pool_from_addr(void* address)
	{
		return this->pools_by_addr[(uintptr_t)address / POOL_ALIGNMENT];
	}

public:

	void* malloc(size_t size)
	{
		void* address = nullptr;
		size_t power_size = this->next_power_of_2(size);
		while (size <= 2048)
		{
			memory_pool_container* pool = this->pool_from_size(power_size);
			if (!pool) { return nullptr; }
			if (address = pool->malloc()) { return address; }
			power_size <<= 1;
		}
		return address;
	}

	void* calloc(size_t size)
	{
		void* address = nullptr;
		size_t power_size = this->next_power_of_2(size);
		while (size <= 2048)
		{
			memory_pool_container* pool = this->pool_from_size(power_size);
			if (!pool) { return nullptr; }
			if (address = pool->calloc()) { return address; }
			power_size <<= 1;
		}
		return address;
	}

	size_t mem_size(void* address)
	{
		memory_pool_container* pool = this->pool_from_addr(address);
		if (!pool) { return 0; }
		return pool->mem_size(address);
	}

	bool free(void* address)
	{
		memory_pool_container* pool = this->pool_from_addr(address);
		if (!pool) { return false; }
		return pool->free(address);
	}

};
