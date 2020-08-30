#include "loader.h"

#include "util.h"

void nvhr() { }

BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{	
		DisableThreadLibraryCalls(hModule);
		create_loader_hook();
	}
	return TRUE;
}
