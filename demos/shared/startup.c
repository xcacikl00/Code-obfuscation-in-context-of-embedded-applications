/*
 * @filename: startup.c
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

// use symbols from linker script
extern uint32_t _la_data;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
extern uint32_t _stack_top;

extern int main();

void HardFault_Handler(void)
{
    while (1)
        ; // Loop forever on hard fault
}

void UsageFault_Handler(void)
{
    while (1)
        ;
}

void BusFault_Handler(void)
{
    while (1)
        ;
}

void MemManage_Handler(void)
{
    while (1)
        ;
}

void Reset_Handler(void)
{

    // copy .data section from flash to ram
    uint32_t *pSrc = &_la_data;
    uint32_t *pDest = &_sdata;
    while (pDest < &_edata)
    {
        *pDest++ = *pSrc++;
    }

    // init .bss section to zero
    uint32_t *pBss = &_sbss;
    while (pBss < &_ebss)
    {
        *pBss++ = 0;
    }
    volatile uint32_t *cpacr = (volatile uint32_t *)0xE000ED88;
    *cpacr |= (0xF << 20);

    main();
    while (1)
    {
    }
}

__attribute__((section(".vector_table")))
const void *vectors[] = {
    &_stack_top,
    Reset_Handler,
    0,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,

};
