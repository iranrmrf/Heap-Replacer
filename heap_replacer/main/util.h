#pragma once

#define WIN32_LEAN_AND_MEAN

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

#define KB (1024u * 1u)
#define MB (1024u * KB)
#define GB (1024u * MB)

#define USUM(a, b) ((unsigned int)(a) + (unsigned int)(b))
#define USUB(a, b) ((unsigned int)(a) - (unsigned int)(b))

#define VSUM(a, b) ((void *)(USUM(a, b)))
#define VSUB(a, b) ((void *)(USUB(a, b)))

#define ROUND_UP(x, y) (USUM(((USUB((x), 1)) | (USUB((y), 1))), 1))
#define ROUND_DOWN(x, y) ((unsigned int)(x) & ~(USUB((y), 1))))

#define countof(a) (sizeof(a) / sizeof(a[0]))

void *hr_winapi_alloc(size_t size)
{
    return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

void *hr_winapi_malloc(size_t size)
{
    return hr_winapi_alloc(size);
}

void *hr_winapi_calloc(size_t count, size_t size)
{
    return hr_winapi_alloc(count * size);
}

void hr_winapi_free(void *address)
{
    VirtualFree(address, 0, MEM_RELEASE);
}

void hr_memset8(void *dst, BYTE val, size_t cnt)
{
    for (BYTE *pos = (BYTE *)dst; cnt--; *pos++ = val)
    {
    }
}

void hr_memset16(void *dst, WORD val, size_t cnt)
{
    for (WORD *pos = (WORD *)dst; cnt--; *pos++ = val)
    {
    }
}

void hr_memset32(void *dst, DWORD val, size_t cnt)
{
    for (DWORD *pos = (DWORD *)dst; cnt--; *pos++ = val)
    {
    }
}

void patch_bytes(void *addr, BYTE *data, DWORD size)
{
    DWORD p;
    VirtualProtect(addr, size, PAGE_EXECUTE_READWRITE, &p);
    memcpy(addr, data, size);
    VirtualProtect(addr, size, p, &p);
    FlushInstructionCache(GetCurrentProcess(), addr, size);
}

void patch_BYTE(void *addr, BYTE data)
{
    patch_bytes(addr, &data, 1u);
}

void patch_WORD(void *addr, WORD data)
{
    BYTE bytes[2];
    *(WORD *)bytes = data;
    patch_bytes(addr, bytes, 2u);
}

void patch_DWORD(void *addr, DWORD data)
{
    BYTE bytes[4];
    *(DWORD *)bytes = data;
    patch_bytes(addr, bytes, 4u);
}

void patch_func_ptr(void *addr, void *ptr)
{
    patch_DWORD(addr, (DWORD)ptr);
}

void patch_detour(void *addr, void *new_func, void **old_func)
{
    *old_func = *(void **)addr;
    patch_func_ptr(addr, new_func);
}

void patch_call(void *addr, void *dst)
{
    BYTE bytes[5];
    bytes[0] = 0xE8u;
    *(DWORD *)((DWORD)bytes + 1u) = (DWORD)dst - (DWORD)addr - 5u;
    patch_bytes(addr, bytes, 5u);
}

void patch_jmp(void *addr, void *dst)
{
    BYTE bytes[5];
    bytes[0] = 0xE9u;
    *(DWORD *)((DWORD)bytes + 1u) = (DWORD)dst - (DWORD)addr - 5u;
    patch_bytes(addr, bytes, 5u);
}

void patch_ret(void *addr)
{
    BYTE bytes = 0xC3u;
    patch_bytes(addr, &bytes, 1u);
}

void patch_ret_argc(void *addr, size_t argc)
{
    BYTE bytes = 0xC2u;
    patch_bytes(addr, &bytes, 1u);
    patch_WORD(VSUM(addr, 1u), (WORD)(4u * argc));
}

void patch_bp(void *addr)
{
    BYTE bytes = 0xCCu;
    patch_bytes(addr, &bytes, 1u);
}

void patch_nops(void *addr, size_t count)
{
    BYTE *bytes = _alloca(count);
    memset(bytes, 0x90u, count);
    patch_bytes(addr, bytes, count);
}

void patch_nop_call(void *addr)
{
    patch_nops(addr, 5u);
}

void patch_call_nops(void *addr, void *dst, size_t nops)
{
    patch_call(addr, dst);
    patch_nops(VSUM(addr, 5u), nops);
}

void detour_vtable(void *obj, size_t index, void *new_func, void **old_func)
{
    void **vtable = *(void ***)obj;
    *old_func = vtable[index];
    patch_func_ptr(&vtable[index], new_func);
}

void *get_import_address(HMODULE hmd, const char *dll_name,
                         const char *func_name)
{
    char *base = (char *)hmd;
    int i;
    const char *loaded_dll;
    char *import_name;

    IMAGE_THUNK_DATA *name_table;
    IMAGE_THUNK_DATA *import_table;
    IMAGE_DOS_HEADER *dos_header = (IMAGE_DOS_HEADER *)base;
    IMAGE_NT_HEADERS *nt_headers =
        (IMAGE_NT_HEADERS *)(base + dos_header->e_lfanew);
    IMAGE_DATA_DIRECTORY section =
        nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    IMAGE_IMPORT_DESCRIPTOR *import_descriptor =
        (IMAGE_IMPORT_DESCRIPTOR *)(base + section.VirtualAddress);

    for (i = 0; import_descriptor[i].Name; i++)
    {
        loaded_dll = (const char *)(base + import_descriptor[i].Name);

        if (!_stricmp(loaded_dll, dll_name))
        {
            if (!import_descriptor[i].FirstThunk)
            {
                return NULL;
            }
            name_table =
                (IMAGE_THUNK_DATA *)(base +
                                     import_descriptor[i].OriginalFirstThunk);
            import_table =
                (IMAGE_THUNK_DATA *)(base + import_descriptor[i].FirstThunk);
            while (name_table->u1.Ordinal)
            {
                if (!IMAGE_SNAP_BY_ORDINAL(name_table->u1.Ordinal))
                {
                    import_name =
                        ((IMAGE_IMPORT_BY_NAME *)(base + name_table->u1
                                                             .ForwarderString))
                            ->Name;

                    if (!_stricmp(import_name, func_name))
                    {
                        return &import_table->u1.AddressOfData;
                    }
                }
                name_table++;
                import_table++;
            }
        }
    }
    return NULL;
}

int is_large_address_aware(HMODULE hmd)
{
    char *base = (char *)hmd;
    IMAGE_DOS_HEADER *dos_header = (IMAGE_DOS_HEADER *)base;
    IMAGE_NT_HEADERS *nt_headers =
        (IMAGE_NT_HEADERS *)(base + dos_header->e_lfanew);
    return (nt_headers->FileHeader.Characteristics &
            IMAGE_FILE_LARGE_ADDRESS_AWARE) == IMAGE_FILE_LARGE_ADDRESS_AWARE;
}

int file_exists(const char *name)
{
    int ret = 0;
    FILE *file;

    if (file = fopen(name, "r"))
    {
        fclose(file);
        ret = 1;
        goto end;
    }

end:
    return ret;
}

void create_console(void)
{
    FILE *f;
    if (AllocConsole())
    {
        f = freopen("CONIN$", "r", stdin);
        f = freopen("CONOUT$", "w", stdout);
        f = freopen("CONOUT$", "w", stderr);
    }
}
