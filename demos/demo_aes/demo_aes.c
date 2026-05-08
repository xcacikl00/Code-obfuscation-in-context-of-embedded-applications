/*
 * @filename: demo_aes.c
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


#include "tiny-AES-C/aes.h"
#include <utils.h>

void aes_encrypt_block(uint8_t *data, uint8_t *key)
{
    struct AES_ctx ctx;
    AES_init_ctx(&ctx, key);
    AES_ECB_encrypt(&ctx, data);
}
volatile uint32_t benchmark_result = 0;

uint8_t key[] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
uint8_t data[] = {0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a};


int main()
{
    reset_cycle_counter();
    uint32_t start = get_cycles();

    aes_encrypt_block(data, key);

    // expected 58 215 123 180 13 122 54 96 168 158 202 243 36 102 239 151

    uint32_t end = get_cycles();
    benchmark_result = end - start;

    while (1)
    {
    }
        
}
