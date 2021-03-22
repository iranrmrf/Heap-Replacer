#pragma once

#include "main/util.h"

class light_critical_section
{

private:

	DWORD thread_id;
	size_t lock_count;

public:

	light_critical_section();
	~light_critical_section();

	void lock();
	void lock_game(const char* msg);
	void unlock();

};
