#pragma once

#include "main/util.h"

struct light_critical_section { DWORD thread_id; size_t lock_count; };

void __fastcall enter_light_critical_section(TFPARAM(light_critical_section* self, const char* msg))
{
	DWORD id = GetCurrentThreadId();
	if (self->thread_id == id) { self->lock_count++; }
	else { while (InterlockedCompareExchange(&self->thread_id, id, NULL)) { } self->lock_count = 1; }
}

void __fastcall leave_light_critical_section(TFPARAM(light_critical_section* self))
{
	--self->lock_count;
	self->thread_id = BITWISE_IF_ELSE(!self->lock_count, NULL, self->thread_id);
}
