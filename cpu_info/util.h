#pragma once

#include <windows.h>

#define HR_MSGBOX(msg, type)                                                   \
    MessageBox(NULL, HR_NAME L" - " msg, L"Error", type)

void create_console(void)
{
    FILE *f;
    if (AllocConsole())
    {
        f = freopen("CONIN$", "r", stdin);
        f = freopen("CONOUT$", "w", stdout);
        f = freopen("CONOUT$", "w", stderr);
    }
}

int file_exists(const char *name)
{
    int ret = 0;
    FILE *file;

    if (file = fopen(name, "r"))
    {
        fclose(file);
        ret = 1;
        goto end;
    }

end:
    return ret;
}
