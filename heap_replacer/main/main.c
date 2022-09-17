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

    HR_LOG("QPC hook executing...");

    apply_hr_hooks();

    HR_LOG("hooks applied.");

    HR_LOG("cleaning QPC hook...");

    addr = get_import_address(hmd, "kernel32.dll", "QueryPerformanceCounter");

    patch_func_ptr(addr, old_qpc);
    return old_qpc(lpPerformanceCount);
}

void create_loader_hook(void)
{
    HMODULE hmd = GetModuleHandle(NULL);

    HR_LOG("module base is at 0x%08X", (DWORD)hmd);

    void *addr =
        get_import_address(hmd, "kernel32.dll", "QueryPerformanceCounter");

    HR_LOG("kernel32.dll::QueryPerformanceCounter is at 0x%08X", (DWORD)addr);

    if (((DWORD)addr & HR_GAME_QPC_HOOK) == HR_GAME_QPC_HOOK)
    {
        if (is_large_addr_aware(hmd))
        {
            HR_LOG("creating QPC hook at 0x%08X...", (DWORD)addr);
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
    /* while (!IsDebuggerPresent())
    {
        Sleep(1);
    } */

    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);

        log_file = fopen("hr.log", "w");
        nlock_init(&log_lock);

        create_loader_hook();
    }

    return TRUE;
}
