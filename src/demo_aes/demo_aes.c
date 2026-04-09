#include <stdio.h>
#include <string.h>
#include "aes.h"


void aes_encrypt_block(uint8_t* data, uint8_t* key) {
    struct AES_ctx ctx;
    AES_init_ctx(&ctx, key);
    AES_ECB_encrypt(&ctx, data);
}

int main(void) {
    uint8_t key[] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
    uint8_t data[] = { 0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a };

    aes_encrypt_block(data, key);

    // Expected  3a d7 7b b4 0d 7a 36 60 a8 9e ca f3 24 66 ef 97
    printf("encrypted: ");
    for(int i = 0; i < 16; i++) {
        printf("%02x ", data[i]);
    }
    printf("\n");

    return 0;
}