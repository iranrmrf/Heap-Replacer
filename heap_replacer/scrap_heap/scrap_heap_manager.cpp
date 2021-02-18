#include "scrap_heap_manager.h"

scrap_heap_manager::scrap_heap_manager()
{
	util::memset8(this->buffers, 0u, scrap_heap_manager_buffer_count * sizeof(scrap_heap_buffer));
	this->free_buffer_count = 0u;
	// cs not init
	this->mt_sh_vector = new scrap_heap_vector(16);
}

scrap_heap_manager::~scrap_heap_manager()
{

}

scrap_heap_manager* scrap_heap_manager::get_instance()
{
	static scrap_heap_manager instance;
	return &instance;
}

void scrap_heap_manager::swap_buffers(size_t index)
{
	if (index < --this->free_buffer_count) [[likely]]
	{
		this->buffers[index] = { this->buffers[this->free_buffer_count].addr, this->buffers[this->free_buffer_count].size };
	}
}

void* scrap_heap_manager::create_buffer(size_t size)
{
	void* address = VirtualAlloc(nullptr, scrap_heap_buffer_max_size, MEM_RESERVE, PAGE_READWRITE);
	VirtualAlloc(address, size, MEM_COMMIT, PAGE_READWRITE);
	return address;
}

void* scrap_heap_manager::request_buffer(size_t* size)
{
	if (!this->free_buffer_count)
	{
		return this->create_buffer(*size);
	}
	this->critical_section.lock(nullptr);
	if (this->free_buffer_count)
	{
		size_t max_index = 0;
		size_t max_size = 0;
		for (size_t i = 0; i < this->free_buffer_count; i++)
		{
			if (this->buffers[i].size >= *size)
			{
				*size = this->buffers[i].size;
				void* address = this->buffers[i].addr;
				this->swap_buffers(i);
				this->critical_section.unlock();
				return address;
			}
			if (this->buffers[i].size > max_size)
			{
				max_size = this->buffers[i].size;
				max_index = i;
			}
		}
		this->swap_buffers(max_index);
		VirtualAlloc(VPTRSUM(this->buffers[max_index].addr, max_size), *size - max_size, MEM_COMMIT, PAGE_READWRITE);
		this->critical_section.unlock();
		return this->buffers[max_index].addr;
	}
	void* address = this->create_buffer(*size);
	this->critical_section.unlock();
	return address;
}

void scrap_heap_manager::free_buffer(void* address, size_t size)
{
	VirtualFree(address, 0u, MEM_RELEASE);
}

void scrap_heap_manager::release_buffer(void* address, size_t size)
{
	if (this->free_buffer_count >= scrap_heap_manager_buffer_count)
	{
		this->free_buffer(address, size);
	}
	this->critical_section.lock(nullptr);
	if (this->free_buffer_count < scrap_heap_manager_buffer_count)
	{
		this->buffers[this->free_buffer_count].addr = address;
		this->buffers[this->free_buffer_count].size = size;
		this->free_buffer_count++;
	}
	else
	{
		this->free_buffer(address, size);
	}
	this->critical_section.unlock();
}
