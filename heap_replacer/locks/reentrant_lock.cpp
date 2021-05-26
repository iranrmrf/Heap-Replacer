#include "reentrant_lock.h"

reentrant_lock::reentrant_lock() : thread_id(0u), lock_count(0u)
{

}

void reentrant_lock::lock()
{
	this->lock_game(nullptr);
}

void reentrant_lock::lock_game(const char* msg)
{
	DWORD id = GetCurrentThreadId();
	if (this->thread_id == id)
	{
		this->lock_count++;
	}
	else
	{
		while (InterlockedCompareExchange(&this->thread_id, id, 0u)) { Sleep(0u); }
		this->lock_count = 1u;
	}
}

void reentrant_lock::unlock()
{
	if (!--this->lock_count)
	{
		this->thread_id = 0u;
	}
}
