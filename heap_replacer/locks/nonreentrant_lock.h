#pragma once

#include "main/util.h"

class nonreentrant_lock
{

private:

	DWORD locked;

public:

	nonreentrant_lock();

	void lock();
	void unlock();

};
