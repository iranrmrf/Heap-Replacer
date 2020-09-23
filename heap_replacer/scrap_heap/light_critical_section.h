#pragma once

#include "main/util.h"

struct light_critical_section
{
	DWORD thread_id;
	size_t lock_count;
};

void __fastcall enter_light_critical_section(TFPARAM(light_critical_section* self, const char* name))
{
	DWORD id = GetCurrentThreadId();
	if (self->thread_id == id)
	{
		++self->lock_count;
	}
	else
	{
		size_t spin = 0;
		while (InterlockedCompareExchange(&self->thread_id, id, 0u))
		{
			Sleep(++spin > 10000u);
		}
		self->lock_count = 1;
	}
}

void __fastcall leave_light_critical_section(TFPARAM(light_critical_section* self))
{
	if (!--self->lock_count)
	{
		self->thread_id = 0;
	}
}
