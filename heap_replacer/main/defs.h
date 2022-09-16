#pragma once

#define FNV

// ONE OF THESE NEED TO BE DEFINED!
#if defined(FNV)

#define HR_NAME L"NVHR"
#define HR_GAME_VERSION L"1.4.0.525"
#define HR_GAME_QPC_HOOK (0xF0A0)
#define HR_GECK_QPC_HOOK (0x320C)

#elif defined(FO3)

#define HR_NAME "F3HR"
#define HR_GAME_VERSION "1.7.0.3"
#define HR_GAME_QPC_HOOK (0xB0E4)
#define HR_GECK_QPC_HOOK (0x3208)

#endif

#define HR_MSGBOX(msg, type)                                                   \
    MessageBox(NULL, HR_NAME L" - " msg, L"Error", type)
