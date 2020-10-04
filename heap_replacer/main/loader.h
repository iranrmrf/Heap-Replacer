#pragma once

#include "heap_replacer.h"

#include "util.h"

BOOL WINAPI qpc_hook(LARGE_INTEGER* lpPerformanceCount)
{
	HR_PRINTF("Applying hooks.");
	NVHR::apply_heap_hooks();
	BYTE* base = (BYTE*)GetModuleHandle(NULL);
	void* address = Util::get_IAT_address(base, "kernel32.dll", "QueryPerformanceCounter");
	HR_PRINTF("Cleaning QPC hook...");
	Util::Mem::patch_DWORD((DWORD)address, (DWORD)&QueryPerformanceCounter);
	return QueryPerformanceCounter(lpPerformanceCount);
}

void create_loader_hook()
{
	BYTE* base = (BYTE*)GetModuleHandle(NULL);
	void* address = Util::get_IAT_address(base, "kernel32.dll", "QueryPerformanceCounter");
	if (address == (void*)0x00FDF0A0)
	{
		if (Util::is_LAA(base))
		{
			if (Util::file_exists("d3dx9_38.tmp")) { Util::create_console(); }
			HR_PRINTF("Creating QPC hook...");
			Util::Mem::patch_DWORD((DWORD)address, (DWORD)&qpc_hook);
		}
		else
		{
			HR_MSGBOX("Your game is not LAA, please apply a 4GB patcher");
		}
	}
	else
	{
		HR_MSGBOX("Incompatible game executable. Please use version (" HR_VERSION ")");
	}
}
