#include <intrin.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "util.h"

#define btos(b) (b) ? "Y" : "N"

enum
{
    EAX,
    EBX,
    ECX,
    EDX,
};

enum
{
    IA32,
    SSE,
    SSE2,
    AVX,
    AVX2,
    AVX512,
};

const char *iset_names[] = {
    [IA32] = "IA32.dll", [SSE] = "SSE.dll",   [SSE2] = "SSE2.dll",
    [AVX] = "AVX.dll",   [AVX2] = "AVX2.dll", [AVX512] = "AVX512.dll"};

int get_min_iset(void)
{
    int f1_ecx = 0, f1_edx = 0, f7_ebx = 0;
    int regs[4];
    char brand[0x30];
    int i, c;
    int ret;

    __cpuid(regs, 0);
    c = regs[EAX];
    for (i = 0; i <= c; i++)
    {
        __cpuidex(regs, i, 0);
        if (i == 1)
        {
            f1_ecx = regs[ECX];
            f1_edx = regs[EDX];
        }
        if (i == 7)
        {
            f7_ebx = regs[EBX];
        }
    }

    __cpuid(regs, 0x80000000);
    c = regs[EAX];
    memset(brand, 0, sizeof(brand));
    for (i = 0x80000000; i <= c; i++)
    {
        __cpuidex(regs, i, 0);
        if (i == 0x80000002)
        {
            memcpy(brand + 0x00, regs, sizeof(regs));
        }
        if (i == 0x80000003)
        {
            memcpy(brand + 0x10, regs, sizeof(regs));
        }
        if (i == 0x80000004)
        {
            memcpy(brand + 0x20, regs, sizeof(regs));
        }
    }

    printf("%s\n", brand);
    printf("\n");

    printf("SSE\t: %s\n", btos(f1_edx & (1 << 25)));
    printf("SSE2\t: %s\n", btos(f1_edx & (1 << 26)));
    printf("SSE3\t: %s\n", btos(f1_ecx & (1 << 0)));
    printf("SSSE3\t: %s\n", btos(f1_ecx & (1 << 9)));
    printf("SSE4.1\t: %s\n", btos(f1_ecx & (1 << 19)));
    printf("SSE4.2\t: %s\n", btos(f1_ecx & (1 << 20)));
    printf("AVX\t: %s\n", btos(f1_ecx & (1 << 28)));
    printf("AVX2\t: %s\n", btos(f7_ebx & (1 << 5)));
    printf("AVX512\t: %s\n", btos(f7_ebx & (1 << 16)));

    printf("\n => ");
    if (f7_ebx & (1 << 16))
    {
        printf("Use AVX512");
        ret = AVX512;
        goto end;
    }
    else if (f7_ebx & (1 << 05))
    {
        printf("Use AVX2");
        ret = AVX2;
        goto end;
    }
    else if (f1_ecx & (1 << 28))
    {
        printf("Use AVX");
        ret = AVX;
        goto end;
    }
    else if (f1_edx & (1 << 26))
    {
        printf("Use SSE2");
        ret = SSE2;
        goto end;
    }
    else if (f1_edx & (1 << 25))
    {
        printf("Use SSE");
        ret = SSE;
        goto end;
    }
    else
    {
        printf("Use IA32");
        ret = IA32;
        goto end;
    }

end:
    printf(" <= \n");
    return ret;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved)
{
    int iset;
    char buff[256] = "Data\\" HR_NAME "\\";

    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        if (!file_exists("d3dx9_38.tmp"))
        {
            create_console();
        }

        DisableThreadLibraryCalls(hModule);

        iset = get_min_iset();
        strcat(buff, iset_names[iset]);
        printf("\nsearching for %s... ", buff);
        if (file_exists(buff))
        {
            printf("found!\n\n");
            LoadLibraryA(buff);
        }
        else
        {
            printf("not found!\n\n");
            HR_MSGBOX("No valid HR binary found in Data/" HR_NAME "!",
                      MB_ICONERROR);
        }
    }

    return TRUE;
}
