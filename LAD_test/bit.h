#ifndef _IDC_BIT_H_
#define _IDC_BIT_H_
#include <stdlib.h>

#ifdef __cplusplus
 extern "C" {
#endif

// initialize I2C BIT register
// return bit array with detected channels (typically CH1 | CH2)
int bit_init_all();
void print_bit(uint8_t result);
uint8_t read_bit(int ch);

#ifdef __cplusplus
 }
#endif

#endif 