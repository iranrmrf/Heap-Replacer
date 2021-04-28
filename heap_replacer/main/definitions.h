#pragma once

// ONE OF THESE NEED TO BE DEFINED!
#if defined(FNV)

#define HR_NAME "NVHR"
#define HR_VERSION "1.4.0.525"
#define HR_WINDOW_NAME "Fallout: New Vegas"
#define HR_GAME_QPC_HOOK ((void*)(0x00FDF0A0))
#define HR_GECK_QPC_HOOK ((void*)(0x00D2320C))
#define HR_MAIN_WINDOW (*(HWND*)0x11C6FC0)
#define HR_SUB_WINDOW (*(HWND*)0x11C6FBC)
#define HR_D3DEVICE (*(LPDIRECT3DDEVICE9*)(*(uintptr_t*)0x11C73B4 + 0x288))

#elif defined(FO3)

#define HR_NAME "F3HR"
#define HR_VERSION "1.7.0.3"
#define HR_WINDOW_NAME "Fallout3"
#define HR_GAME_QPC_HOOK ((void*)(0x00D9B0E4))
#define HR_GECK_QPC_HOOK ((void*)(0x00D03208))
#define HR_MAIN_WINDOW (*(HWND*)0x106F4F8)
#define HR_SUB_WINDOW (*(HWND*)0x106F4F4)
#define HR_D3DEVICE (*(LPDIRECT3DDEVICE9*)(*(uintptr_t*)0x106F8D8 + 0x288))

#endif

//#define HR_USE_GUI
//#define HR_ZERO_MEM

#define HR_MSGBOX(msg) MessageBox(NULL, HR_NAME " - " msg, "Error", NULL)
#define HR_PRINTF(msg) printf(HR_NAME " - " msg "\n")
