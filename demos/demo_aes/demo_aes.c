
#include "aes.h"
#include <utils.h>

void aes_encrypt_block(uint8_t *data, uint8_t *key)
{
    struct AES_ctx ctx;
    AES_init_ctx(&ctx, key);
    AES_ECB_encrypt(&ctx, data);
}

int main()
{
    uint8_t key[] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
    uint8_t data[] = {0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a};
    char buffer[20];

    print_string("plaintext:\n");

    for (int i = 0; i < 16; i++)
    {
        num_to_string(data[i], buffer, 0);
        print_string(buffer);
        print_string(" ");
    }
    print_string("\nencrypting...\n");

    aes_encrypt_block(data, key);

    // expected 58 215 123 180 13 122 54 96 168 158 202 243 36 102 239 151

    for (int i = 0; i < 16; i++)
    {
        num_to_string(data[i], buffer, 0);
        print_string(buffer);
        print_string(" ");
    }

    return 0;
}
