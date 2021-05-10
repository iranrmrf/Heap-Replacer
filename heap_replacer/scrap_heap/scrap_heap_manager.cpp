#include "scrap_heap_manager.h"

scrap_heap_manager::scrap_heap_manager()
{
	util::cmemset8(this->buffers, 0u, scrap_heap_manager_buffer_count * sizeof(scrap_heap_buffer));

	this->free_buffer_count = 0u;

#ifdef HR_USE_GUI
	this->used_buffer_count = 0u;

	this->used_size = 0u;
	this->free_size = 0u;
#endif

}

scrap_heap_manager::~scrap_heap_manager()
{

}

void scrap_heap_manager::replace_with_last_buffer(size_t index)
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

void scrap_heap_manager::free_buffer(void* address, size_t size)
{
	VirtualFree(address, 0u, MEM_RELEASE);
}

void* scrap_heap_manager::request_buffer(size_t* size)
{
	this->lock.lock();
#ifdef HR_USE_GUI
	this->used_buffer_count++;
#endif
	if (!this->free_buffer_count)
	{
		this->lock.unlock();
		return this->create_buffer(*size);
	}
	size_t max_index = 0u;
	size_t max_size = 0u;
	for (size_t i = 0u; i < this->free_buffer_count; i++)
	{
		if (this->buffers[i].size >= *size)
		{
			*size = this->buffers[i].size;
			void* address = this->buffers[i].addr;
			this->replace_with_last_buffer(i);
			this->lock.unlock();
			return address;
		}
		if (this->buffers[i].size > max_size)
		{
			max_size = this->buffers[i].size;
			max_index = i;
		}
	}
	void* address = this->buffers[max_index].addr;
	VirtualAlloc(VPTRSUM(address, max_size), *size - max_size, MEM_COMMIT, PAGE_READWRITE);
	this->replace_with_last_buffer(max_index);
	this->lock.unlock();
	return address;
}

void scrap_heap_manager::release_buffer(void* address, size_t size)
{
	this->lock.lock();
#ifdef HR_USE_GUI
	this->used_buffer_count--;
#endif
	if (this->free_buffer_count >= scrap_heap_manager_buffer_count)
	{
		this->free_buffer(address, size);
	}
	else
	{
		this->buffers[this->free_buffer_count].addr = address;
		this->buffers[this->free_buffer_count].size = size;
		this->free_buffer_count++;
	}
	this->lock.unlock();
}
