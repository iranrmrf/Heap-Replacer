#include "nonreentrant_lock.h"

nonreentrant_lock::nonreentrant_lock() : locked(false)
{

}

void nonreentrant_lock::lock()
{
	while (InterlockedCompareExchange(&this->locked, true, false)) { Sleep(0u); };
}

void nonreentrant_lock::unlock()
{
	this->locked = false;
}
