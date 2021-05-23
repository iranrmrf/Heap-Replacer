#include "scrap_heap.h"

scrap_heap::scrap_heap()
{
	this->init();
}

scrap_heap::~scrap_heap()
{
	this->purge();
}

void scrap_heap::init()
{
	this->sh_bgn = hr::hr_malloc(sh_buffer_size);
	this->sh_cur = this->sh_bgn;
	this->sh_end = VPTRSUM(this->sh_bgn, sh_buffer_size);
	this->last = nullptr;
}

void scrap_heap::init_0x10000()
{
	this->init();
}

void scrap_heap::init_var(size_t size)
{
	this->init();
}

void* scrap_heap::alloc(size_t size, size_t alignment)
{
	sh_chunk* header = (sh_chunk*)util::align<4u>(this->sh_cur);
	header->size = size;
	header->prev = this->last;
	this->last = header++;
	this->sh_cur = VPTRSUM(header, size);
	if (this->sh_cur > this->sh_end) [[unlikely]] { HR_PRINTF("Scrap heap block out of memory!"); return nullptr; }
	return header;
}

void scrap_heap::free(void* address)
{
	sh_chunk* chunk = (sh_chunk*)VPTRDIFF(address, sizeof(sh_chunk));
	if (address && !(chunk->size & sh_free)) [[likely]]
	{
		for (chunk->size |= sh_free; this->last && (this->last->size & sh_free); this->last = this->last->prev);
		this->sh_cur = this->last ? VPTRSUM(this->last, sizeof(sh_chunk) + this->last->size) : this->sh_bgn;
	} 
}

void scrap_heap::purge()
{
	hr::hr_free(this->sh_bgn);
}

scrap_heap* scrap_heap::get_thread_scrap_heap()
{
	static thread_local scrap_heap* sh = new scrap_heap(); return sh;
}
