#include <stdio.h>
#include <windows.h>

#include "defs.h"
#include "heap_replacer.h"
#include "util.h"

BOOL(WINAPI *old_qpc)(LARGE_INTEGER *);

BOOL WINAPI qpc_hook(LARGE_INTEGER *lpPerformanceCount)
{
    void *addr;
    HMODULE hmd = GetModuleHandle(NULL);

    apply_hr_hooks();

    printf("Hooks applied.\n");

    printf("Cleaning QPC hook...\n");

    addr = get_import_address(hmd, "kernel32.dll", "QueryPerformanceCounter");
    patch_func_ptr(addr, old_qpc);
    return old_qpc(lpPerformanceCount);
}

void create_loader_hook(void)
{
    HMODULE hmd = GetModuleHandle(NULL);

    void *addr =
        get_import_address(hmd, "kernel32.dll", "QueryPerformanceCounter");

    if (((DWORD)addr & HR_GAME_QPC_HOOK) == HR_GAME_QPC_HOOK)
    {
        if (is_large_address_aware(hmd))
        {
            if (!file_exists("d3dx9_38.tmp"))
            {
                create_console();
            }
            printf("Creating QPC hook...\n");
            patch_detour(addr, &qpc_hook, (void **)&old_qpc);
        }
        else
        {
            HR_MSGBOX(L"Your game is not LAA, please apply a 4GB patcher",
                      MB_ICONERROR);
        }
    }
    else if (((DWORD)addr & HR_GECK_QPC_HOOK) != HR_GECK_QPC_HOOK)
    {
        HR_MSGBOX(L"Incompatible game executable. Please use version "
                  L"(" HR_GAME_VERSION ")",
                  MB_ICONERROR);
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved)
{
    // while (!IsDebuggerPresent()) { Sleep(1); }

    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        create_loader_hook();
    }

    return TRUE;
}
