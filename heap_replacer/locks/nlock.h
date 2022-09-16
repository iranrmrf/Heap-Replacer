#pragma once

#include <windows.h>

struct nlock
{
    DWORD locked;
};

void nlock_init(struct nlock *lock)
{
    lock->locked = 0;
}

void nlock_lock(struct nlock *lock)
{
    while (InterlockedCompareExchange(&lock->locked, 1, 0))
    {
        Sleep(0);
    };
}

void nlock_unlock(struct nlock *lock)
{
    lock->locked = 0;
}
