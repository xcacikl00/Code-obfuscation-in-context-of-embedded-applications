#include "stdint.h"

void print_string(char *s) ;
void num_to_string(float f, char *buffer, int precision);
void reset_cycle_counter();
uint32_t get_cycles();
void delay(void);