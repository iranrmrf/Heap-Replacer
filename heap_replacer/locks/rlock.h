#pragma once

#include <windows.h>

struct rlock
{
    DWORD thread_id;
    size_t lock_count;
};

void rlock_init(struct rlock *lock)
{
    lock->thread_id = 0;
    lock->lock_count = 0;
}

void rlock_lock(struct rlock *lock)
{
    DWORD id = GetCurrentThreadId();
    if (lock->thread_id == id)
    {
        lock->lock_count++;
    }
    else
    {
        while (InterlockedCompareExchange(&lock->thread_id, id, 0))
        {
            Sleep(0);
        };
        lock->lock_count = 1;
    }
}

void rlock_lock_game(struct rlock *lock, void *edx, char *msg)
{
    rlock_lock(lock);
}

void rlock_unlock(struct rlock *lock)
{
    if (!--lock->lock_count)
    {
        lock->thread_id = 0;
    }
}
