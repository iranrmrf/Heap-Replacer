#pragma once

#include "heap_replacer.h"

#include "util.h"

BOOL WINAPI qpc_hook(LARGE_INTEGER* lpPerformanceCount)
{
	printf("NVHR - Applying hooks...\n");
	apply_heap_hooks();
	BYTE* base = (BYTE*)GetModuleHandle(NULL);
	void* address = get_IAT_address(base, "kernel32.dll", "QueryPerformanceCounter");
	printf("NVHR - Cleaning QPC hook...\n");
	patch_DWORD((DWORD)address, (DWORD)&QueryPerformanceCounter);
	return QueryPerformanceCounter(lpPerformanceCount);
}

void create_loader_hook()
{
	BYTE* base = (BYTE*)GetModuleHandle(NULL);
	void* address = get_IAT_address(base, "kernel32.dll", "QueryPerformanceCounter");
	if (address == (void*)0x00FDF0A0)
	{
		if (is_LAA(base))
		{
			if (exists("d3dx9_38.tmp")) { create_console(); }
			printf("NVHR - Creating QPC hook...\n");
			patch_DWORD((DWORD)address, (DWORD)&qpc_hook);
		}
		else
		{
			MessageBox(NULL, "NVHR - Your game is not LAA, please apply a 4GB patcher", "Error", NULL);
		}
	}
}
