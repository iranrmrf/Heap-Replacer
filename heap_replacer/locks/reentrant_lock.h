#pragma once

#include "main/util.h"

class reentrant_lock
{

private:

	DWORD thread_id;
	size_t lock_count;

public:

	reentrant_lock();

	void lock();
	void lock_game(const char* msg);
	void unlock();

};
