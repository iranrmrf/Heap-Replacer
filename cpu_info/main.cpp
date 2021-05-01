#include <intrin.h>
#include <stdio.h>
#include <Windows.h>

#define btos(b) (b) ? "Y" : "N"

enum { EAX, EBX, ECX, EDX };

int main(int argc, char** argv)
{
    int f1_ECX = 0, f1_EDX = 0, f7_EBX = 0;
    int regs[4];
    __cpuid(regs, 0);
    int c = regs[EAX];
    for (int i = 0; i <= c; i++)
    {
        __cpuidex(regs, i, 0);
        if (i == 1) { f1_ECX = regs[ECX]; f1_EDX = regs[EDX]; }
        if (i == 7) { f7_EBX = regs[EBX]; }
    }
    __cpuid(regs, 0x80000000);
    c = regs[EAX];
    char brand[0x30];
    memset(brand, 0, sizeof(brand));
    for (int i = 0x80000000; i <= c; i++)
    {
        __cpuidex(regs, i, 0);
        if (i == 0x80000002) { memcpy(brand + 0x00, regs, sizeof(regs)); }
        if (i == 0x80000003) { memcpy(brand + 0x10, regs, sizeof(regs)); }
        if (i == 0x80000004) { memcpy(brand + 0x20, regs, sizeof(regs)); }
    }
    printf("%s\n", brand);
    printf("\n");
    printf("SSE\t: %s\n", btos(f1_EDX & (1 << 25)));
    printf("SSE2\t: %s\n", btos(f1_EDX & (1 << 26)));
    printf("SSE3\t: %s\n", btos(f1_ECX & (1 << 0)));
    printf("SSSE3\t: %s\n", btos(f1_ECX & (1 << 9)));
    printf("SSE4.1\t: %s\n", btos(f1_ECX & (1 << 19)));
    printf("SSE4.2\t: %s\n", btos(f1_ECX & (1 << 20)));
    printf("AVX\t: %s\n", btos(f1_ECX & (1 << 28)));
    printf("AVX2\t: %s\n", btos(f7_EBX & (1 << 05)));
    printf("AVX512\t: %s\n", btos(f7_EBX & (1 << 16)));
    printf("\n => ");
    if (f7_EBX & (1 << 16)) { printf("Use AVX512"); }
    else if (f7_EBX & (1 << 05)) { printf("Use AVX2"); }
    else if (f1_ECX & (1 << 28)) { printf("Use AVX"); }
    else if (f1_EDX& (1 << 26)) { printf("Use SSE2"); }
    else if (f1_EDX& (1 << 25)) { printf("Use SSE"); }
    else { printf("Use IA32"); }
    printf(" <= \n");
    printf("\n");
    system("pause");
}
