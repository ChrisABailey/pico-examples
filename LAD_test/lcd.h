#ifndef _IDC_LCD_H_
#define _IDC_LCD_H_
#include "pico/stdlib.h"

#ifdef __cplusplus
 extern "C" {
#endif

typedef struct  { 
                        uint32_t hours;
                        uint32_t mins;
                        uint32_t secs;
                        } elapsed_time_t;

typedef struct  { 
                uint8_t r;
                uint8_t g;
                uint8_t b;
                } cop_data_t;                        

typedef struct {
                int failedReads;
                elapsed_time_t elapsedTime;
                float   temperature;
                bool    lcdGood;
                bool    lvdsTimeout;
                bool    drpSource;
                bool    drpGate;
                bool    vddsa;
                uint16_t   vddsaI;
                cop_data_t copR1;
                cop_data_t copR2;
                cop_data_t copL1;
                cop_data_t copL2;
                bool sourceEnable;
                bool sourceDisable;
                bool enable;
                bool gatePchEnable;
                bool gatePchDisable;
                bool gateNchEnable;
                bool gateNchDisable;
                bool ESDHigh;
                bool ESDLow;
                uint8_t bitLVDS;
                uint8_t bitLCD1;
                uint8_t bitLCD2;
                uint8_t fpga;
                } LCD_BIT_t;

extern LCD_BIT_t last_LCD_BIT_CH1;
extern LCD_BIT_t last_LCD_BIT_CH2;

void lcd_init_all();
void print_status(LCD_BIT_t* BIT);

// reads the serial port associated with ch
// returns false if we need to read more data, true if we are complete
bool read_LCD_status(int ch);

#ifdef __cplusplus
 }
#endif

#endif 