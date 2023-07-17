#ifndef _IDC_I2C_H_
#define _IDC_I2C_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "hardware/i2c.h"

void i2c_init_all();
void i2c_scan(int channel);

// Scan the i2C bus
// Return the first addr that responds after the start addr
int qScan(int channel, int start);

int i2c_reg_write(int channel, 
                const uint addr, 
                const uint8_t reg, 
                uint8_t *buf,
                const uint8_t nbytes);

int i2c_reg_read(int channel,
                const uint addr,
                const uint8_t reg,
                uint8_t *buf,
                const uint8_t nbytes);

int i2c_reg_read16(  int channel,
                const uint addr,
                const uint16_t reg,
                uint8_t *buf,
                const uint8_t nbytes);
#ifdef __cplusplus
 }
#endif

#endif                