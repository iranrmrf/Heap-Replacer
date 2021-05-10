#include "cell_desc.h"

void* cell_desc::get_end()
{
	return VPTRSUM(this->addr, this->size);
}

bool cell_desc::is_in_range(void* address)
{
	return ((this->addr <= address) & (address < this->get_end()));
}
