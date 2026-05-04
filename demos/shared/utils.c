#include <stdint.h>
#include "utils.h"


void print_string(char *s)
{
    volatile unsigned int *uart = (unsigned int *)0x40011004;

    while (*s)
    {
        *uart = *s++;
    }
}

void num_to_string(float num, char *buffer, int precision)
{
    char *ptr = buffer;

    if (num < 0)
    {
        *ptr++ = '-';
        num = -num;
    }

    int integer_part = (int)num;
    float fractional_part = num - (float)integer_part;

    char reverse_buffer[12];
    int i = 0;
    if (integer_part == 0)
    {
        reverse_buffer[i++] = '0';
    }
    else
    {
        while (integer_part > 0)
        {
            reverse_buffer[i++] = (integer_part % 10) + '0';
            integer_part /= 10;
        }
    }

    while (i > 0)
    {
        *ptr++ = reverse_buffer[--i];
    }

    if (precision > 0)
    {
        *ptr++ = '.';

        while (precision--)
        {
            fractional_part *= 10.0f;
            int digit = (int)fractional_part;
            *ptr++ = digit + '0';
            fractional_part -= (float)digit;
        }
    }

    *ptr = '\0';
}

