#pragma once

#if defined(FNV)

#define HR_GAME_VERSION L"1.4.0.525"
#define HR_GAME_QPC_HOOK (0xF0A0)
#define HR_GECK_QPC_HOOK (0x320C)

#elif defined(FO3)

#define HR_GAME_VERSION L"1.7.0.3"
#define HR_GAME_QPC_HOOK (0xB0E4)
#define HR_GECK_QPC_HOOK (0x3208)

#endif

#define HR_MSGBOX(msg, type)                                                   \
    MessageBox(NULL, HR_NAME L" - " msg, L"Error", type)

#define HR_LOG(fmt, ...)                                                       \
    nlock_lock(&log_lock);                                                     \
    get_time(time_buff, sizeof(time_buff));                                    \
    fprintf(log_file, "[%s] - <%s> " fmt "\n", time_buff, __FUNCTION__,        \
            ##__VA_ARGS__);                                                    \
    nlock_unlock(&log_lock);                                                   \
    fflush(log_file);
