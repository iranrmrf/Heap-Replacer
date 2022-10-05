#pragma once

#include <stddef.h>

#include "defs.h"
#include "locks/nlock.h"

FILE *log_file;
struct nlock log_lock;
char time_buff[32];

void *hr_malloc(size_t size);
void *hr_calloc(size_t count, size_t size);
void *hr_realloc(void *addr, size_t size);
void *hr_recalloc(void *addr, size_t size, size_t count);
size_t hr_mem_size(void *addr);
void hr_free(void *addr);

#include "dheap/dheap.h"
#include "mheap/mheap.h"
#include "sheap/sheap.h"

struct mheap m;
struct dheap d;

void *hr_malloc(size_t size)
{
    void *addr;

    if (size <= POOL_MAX_ALLOC_SIZE)
    {
        if (addr = mheap_malloc(&m, size))
        {
            goto end;
        }
    }

    if (size <= DHEAP_BLOCK_SIZE)
    {
        if (addr = dheap_malloc(&d, size))
        {
            goto end;
        }
    }

    addr = hr_winapi_malloc(size);

    if (!addr)
    {
        HR_LOG("%s returned NULL\n", __FUNCTION__);
    }

end:
    return addr;
}

void *hr_calloc(size_t count, size_t size)
{
    void *addr;

    size *= count;

    if (size <= POOL_MAX_ALLOC_SIZE)
    {
        if (addr = mheap_calloc(&m, size))
        {
            goto end;
        }
    }

    if (size <= DHEAP_BLOCK_SIZE)
    {
        if (addr = dheap_calloc(&d, size))
        {
            goto end;
        }
    }

    addr = hr_winapi_malloc(size);

    if (!addr)
    {
        HR_LOG("%s returned NULL\n", __FUNCTION__);
    }

end:
    return addr;
}

void *hr_realloc(void *addr, size_t size)
{
    void *new_addr;
    size_t old_size, new_size;

    if (!addr)
    {
        new_addr = hr_malloc(size);
        goto end;
    }

    if ((old_size = hr_mem_size(addr)) >= (new_size = size))
    {
        new_addr = addr;
        goto end;
    }

    new_addr = hr_malloc(size);
    memcpy(new_addr, addr, old_size);
    hr_free(addr);

end:
    return new_addr;
}

void *hr_recalloc(void *addr, size_t count, size_t size)
{
    void *new_addr;
    size_t old_size, new_size;

    if (!addr)
    {
        new_addr = hr_calloc(count, size);
        goto end;
    }

    if ((old_size = hr_mem_size(addr)) >= (new_size = size * count))
    {
        hr_memset8(VSUM(addr, new_size), 0, old_size - new_size);
        new_addr = addr;
        goto end;
    }

    new_addr = hr_calloc(count, size);
    memcpy(new_addr, addr, old_size);
    hr_free(addr);

end:
    return new_addr;
}

size_t hr_mem_size(void *addr)
{
    size_t size = 0;

    if (!addr)
    {
        goto end;
    }

    if (size = mheap_mem_size(&m, addr))
    {
        goto end;
    }

    if (size = dheap_mem_size(&d, addr))
    {
        goto end;
    }

end:
    return size;
}

void hr_free(void *addr)
{
    if (!addr)
    {
        goto end;
    }

    if (mheap_free(&m, addr))
    {
        goto end;
    }

    if (dheap_free(&d, addr))
    {
        goto end;
    }

    hr_winapi_free(addr);

end:
    return;
}

void *game_heap_allocate(void *self, void *edx, size_t size)
{
    return hr_malloc(size);
}

void *game_heap_reallocate(void *self, void *edx, void *addr, size_t size)
{
    return hr_realloc(addr, size);
}

size_t game_heap_msize(void *self, void *edx, void *addr)
{
    return hr_mem_size(addr);
}

void game_heap_free(void *self, void *edx, void *addr)
{
    hr_free(addr);
}

void *__cdecl crt_malloc(size_t size)
{
    return hr_malloc(size);
}

void *__cdecl crt_calloc(size_t count, size_t size)
{
    return hr_calloc(count, size);
}

void *__cdecl crt_realloc(void *addr, size_t size)
{
    return hr_realloc(addr, size);
}

void *__cdecl crt_recalloc(void *addr, size_t count, size_t size)
{
    return hr_recalloc(addr, count, size);
}

size_t __cdecl crt_msize(void *addr)
{
    return hr_mem_size(addr);
}

void __cdecl crt_free(void *addr)
{
    hr_free(addr);
}

void apply_hr_hooks()
{
    HR_LOG("initializing mheap");
    mheap_init(&m);

    HR_LOG("initializing dheap");
    dheap_init(&d);

#if defined(FNV)

    patch_jmp((void *)0x00ECD1C7, &crt_malloc);
    patch_jmp((void *)0x00ED0CDF, &crt_malloc);
    patch_jmp((void *)0x00EDDD7D, &crt_calloc);
    patch_jmp((void *)0x00ED0D24, &crt_calloc);
    patch_jmp((void *)0x00ECCF5D, &crt_realloc);
    patch_jmp((void *)0x00ED0D70, &crt_realloc);
    patch_jmp((void *)0x00EE1700, &crt_recalloc);
    patch_jmp((void *)0x00ED0DBE, &crt_recalloc);
    patch_jmp((void *)0x00ECD31F, &crt_msize);
    patch_jmp((void *)0x00ECD291, &crt_free);

    patch_jmp((void *)0x00AA3E40, &game_heap_allocate);
    patch_jmp((void *)0x00AA4150, &game_heap_reallocate);
    patch_jmp((void *)0x00AA4200, &game_heap_reallocate);
    patch_jmp((void *)0x00AA44C0, &game_heap_msize);
    patch_jmp((void *)0x00AA4060, &game_heap_free);

    patch_ret((void *)0x00AA6840);
    patch_ret((void *)0x00866E00);
    patch_ret((void *)0x00866770);

    patch_ret((void *)0x00AA6F90);
    patch_ret((void *)0x00AA7030);
    patch_ret((void *)0x00AA7290);
    patch_ret((void *)0x00AA7300);

    patch_ret((void *)0x00AA58D0);
    patch_ret((void *)0x00866D10);
    patch_ret((void *)0x00AA5C80);

    patch_jmp((void *)0x00AA53F0, &sheap_init_fix);
    patch_jmp((void *)0x00AA5410, &sheap_init_var);
    patch_jmp((void *)0x00AA54A0, &sheap_alloc);
    patch_jmp((void *)0x00AA5610, &sheap_free);
    patch_jmp((void *)0x00AA5460, &sheap_purge);

    patch_nops((void *)0x00AA38CA, 0x00AA38E8 - 0x00AA38CA);
    patch_jmp((void *)0x00AA42E0, &sheap_get_thread_local);

    patch_nop_call((void *)0x00AA3060);

    patch_nop_call((void *)0x0086C56F);
    patch_nop_call((void *)0x00C42EB1);
    patch_nop_call((void *)0x00EC1701);

    patch_bytes((void *)0x0086EED4, (BYTE *)"\xEB\x55", 2);

#elif defined(FO3)

    patch_jmp((void *)0x00C063F5, &crt_malloc);
    patch_jmp((void *)0x00C0AB3F, &crt_malloc);
    patch_jmp((void *)0x00C1843C, &crt_calloc);
    patch_jmp((void *)0x00C0AB7F, &crt_calloc);
    patch_jmp((void *)0x00C06546, &crt_realloc);
    patch_jmp((void *)0x00C0ABC7, &crt_realloc);
    patch_jmp((void *)0x00C06761, &crt_recalloc);
    patch_jmp((void *)0x00C0AC12, &crt_recalloc);
    patch_jmp((void *)0x00C067DA, &crt_msize);
    patch_jmp((void *)0x00C064B8, &crt_free);

    patch_jmp((void *)0x0086B930, &game_heap_allocate);
    patch_jmp((void *)0x0086BAE0, &game_heap_reallocate);
    patch_jmp((void *)0x0086BB50, &game_heap_reallocate);
    patch_jmp((void *)0x0086B8C0, &game_heap_msize);
    patch_jmp((void *)0x0086BA60, &game_heap_free);

    patch_ret((void *)0x0086D670);
    patch_ret((void *)0x006E21F0);
    patch_ret((void *)0x006E1E10);

    patch_ret((void *)0x0086C600);
    patch_ret((void *)0x006E1CD0);
    patch_ret((void *)0x0086CA30);

    patch_jmp((void *)0x0086CB70, &sheap_init_fix);
    patch_jmp((void *)0x0086CB90, &sheap_init_var);
    patch_jmp((void *)0x0086C710, &sheap_alloc);
    patch_jmp((void *)0x0086C7B0, &sheap_free);
    patch_jmp((void *)0x0086CAA0, &sheap_purge);

    patch_nops((void *)0x0086C038, 0x0086C086 - 0x0086C038);
    patch_jmp((void *)0x0086BCB0, &sheap_get_thread_local);

    patch_nop_call((void *)0x006E9B30);
    patch_nop_call((void *)0x007FACDB);
    patch_nop_call((void *)0x00AA6534);

#endif
}
