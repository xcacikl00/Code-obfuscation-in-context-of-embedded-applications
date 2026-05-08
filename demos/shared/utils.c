/*
 * @filename: utils.c
 * @author:   Lukáš Čačík
 * @date:     May 09, 26
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

#define DWT_CONTROL (*((volatile uint32_t *)0xE0001000))
#define DWT_CYCCNT (*((volatile uint32_t *)0xE0001004))
#define DEMCR (*((volatile uint32_t *)0xE000EDFC))
#define DEMCR_TRCENA (1 << 24)


#define RCC_BASE      0x40023800
#define RCC_AHB1ENR   (*(volatile uint32_t *)(RCC_BASE + 0x30))

#define GPIOC_BASE    0x40020800
#define GPIOC_MODER   (*(volatile uint32_t *)(GPIOC_BASE + 0x00))
#define GPIOC_ODR     (*(volatile uint32_t *)(GPIOC_BASE + 0x14))

void delay(void) {
    for (volatile int i = 0; i < 500000; i++); 
}


void reset_cycle_counter()
{
    DEMCR |= DEMCR_TRCENA; // Enable trace/debug unit
    DWT_CYCCNT = 0;        // Reset count to 0
    DWT_CONTROL |= 1;      // Enable cycle counter
}

uint32_t get_cycles()
{
    return DWT_CYCCNT;
}

