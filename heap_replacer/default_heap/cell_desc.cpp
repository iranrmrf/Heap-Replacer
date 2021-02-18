#include "cell_desc.h"

cell_desc::cell_desc(void* addr, size_t size, size_t index) : addr(addr), size(size), index(index)
{

}

cell_desc::~cell_desc()
{

}

void* cell_desc::get_end()
{
	return VPTRSUM(this->addr, this->size);
}

bool cell_desc::is_in_range(void* address)
{
	return ((this->addr <= address) & (address < this->get_end()));
}
