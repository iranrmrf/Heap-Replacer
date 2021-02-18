#include "light_critical_section.h"

light_critical_section::light_critical_section() : thread_id(0), lock_count(0)
{

}

light_critical_section::~light_critical_section()
{

}

void light_critical_section::lock(const char* msg)
{
	DWORD id = GetCurrentThreadId();
	if (this->thread_id == id)
	{
		this->lock_count++;
	}
	else
	{
		while (InterlockedCompareExchange(&this->thread_id, id, 0u))
		{
			Sleep(0u);
		}
		this->lock_count = 1;
	}
}

bool light_critical_section::try_lock()
{
	bool retval = false;
	DWORD id = GetCurrentThreadId();
	if (this->thread_id == id)
	{
		this->lock_count++;
		retval = true;
	}
	else
	{
		retval = !InterlockedCompareExchange(&this->thread_id, id, 0u);
		if (retval)
		{
			this->lock_count = 1;
		}
	}
	return retval;
}

void light_critical_section::unlock()
{
	if (!--this->lock_count)
	{
		this->thread_id = 0u;
	}
}
