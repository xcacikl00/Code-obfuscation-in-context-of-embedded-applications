#include <stdint.h>

extern int main(void);


__attribute__((section(".vector_table")))
const void* vectors[] = {
    (void*)0x20008000, // stack pointer
    (void*)main        // exec start
};