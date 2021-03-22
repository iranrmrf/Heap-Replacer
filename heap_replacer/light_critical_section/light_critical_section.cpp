#include "light_critical_section.h"

light_critical_section::light_critical_section() : thread_id(0u), lock_count(0u)
{

}

light_critical_section::~light_critical_section()
{

}

void light_critical_section::lock()
{
	this->lock_game(nullptr);
}

void light_critical_section::lock_game(const char* msg)
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
		this->lock_count = 1u;
	}
}

void light_critical_section::unlock()
{
	if (!--this->lock_count)
	{
		this->thread_id = 0u;
	}
}
