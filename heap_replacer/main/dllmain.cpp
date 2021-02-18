#include "util.h"

#include "heap_replacer.h"

namespace nvhr
{
	void nvhr() { }
}

BOOL WINAPI qpc_hook(LARGE_INTEGER* lpPerformanceCount)
{
	HR_PRINTF("Applying hooks.");
	nvhr::apply_heap_hooks();
	BYTE* base = (BYTE*)GetModuleHandle(nullptr);
	void* address = util::get_IAT_address(base, "kernel32.dll", "QueryPerformanceCounter");
	HR_PRINTF("Cleaning QPC hook...");
	util::patch_DWORD((uintptr_t)address, (uintptr_t)&QueryPerformanceCounter);
	return QueryPerformanceCounter(lpPerformanceCount);
}

void create_loader_hook()
{
	BYTE* base = (BYTE*)GetModuleHandle(nullptr);
	void* address = util::get_IAT_address(base, "kernel32.dll", "QueryPerformanceCounter");
	if (address == HR_GAME_QPC_HOOK)
	{
		if (util::is_LAA(base))
		{
			if (util::file_exists("d3dx9_38.tmp")) { util::create_console(); }
			HR_PRINTF("Creating QPC hook...");
			util::patch_DWORD((uintptr_t)address, (uintptr_t)&qpc_hook);
		}
		else
		{
			HR_MSGBOX("Your game is not LAA, please apply a 4GB patcher");
		}
	}
	else if (address != HR_GECK_QPC_HOOK)
	{
		HR_MSGBOX("Incompatible game executable. Please use version (" HR_VERSION ")");
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hModule);
		create_loader_hook();
	}
	return TRUE;
}
