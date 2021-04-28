#include "heap_replacer.h"

namespace hr
{
	void hr() { }
}

void* old_qpc;

BOOL WINAPI qpc_hook(LARGE_INTEGER* lpPerformanceCount)
{
	HR_PRINTF("Applying hooks.");
	hr::apply_heap_hooks();
	BYTE* base = (BYTE*)GetModuleHandle(nullptr);

#ifdef HR_USE_GUI

	HR_PRINTF("Applying ImGui hooks.");
	hr::apply_imgui_hooks();

	HR_PRINTF("Creating DI8C hook...");

	util::patch_detour(util::get_IAT_address(base, "dinput8.dll", "DirectInput8Create"), &ui::direct_input_8_create_hook, (void**)&ui::direct_input_8_create);

#endif

	HR_PRINTF("Cleaning QPC hook...");
	util::patch_func_ptr(util::get_IAT_address(base, "kernel32.dll", "QueryPerformanceCounter"), old_qpc);

	return ((decltype(qpc_hook)*)(old_qpc))(lpPerformanceCount);
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

			util::patch_detour(address, &qpc_hook, &old_qpc);
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
	//while (!IsDebuggerPresent()) { Sleep(1); }

	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hModule);
		create_loader_hook();
	}
	return TRUE;
}
