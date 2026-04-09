#include <stdint.h>
#include "utils.h"

void qemu_exit()
{
    // Create a parameter block in memory
    uint32_t args[2];
    args[0] = 0x20026; // ADP_Stopped_ApplicationExit
    args[1] = 0;       // Exit code (0 for success)

    register uint32_t reg0 __asm__("r0") = 0x18;           // angel_SWIreason_ReportException
    register uint32_t reg1 __asm__("r1") = (uint32_t)args; // POINTER to the args

    __asm__ volatile(
        "bkpt 0xAB"
        :
        : "r"(reg0), "r"(reg1)
        : "memory");
}

// (In QEMU lm3s6965evb 0x4000C000 is the UART register)


void print_string( char *s)
{
    volatile unsigned int *uart = (unsigned int *)UARTPTR;

    while (*s)
    {
        *uart = *s++;
    }
}