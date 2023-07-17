#include "pico/sync.h"

#define LED_PIN 25

#define BIT_CH1_PIN         10
#define BIT_CH2_PIN         22

#define TOUCH_CH1_PIN       17
#define TOUCH_CH2_PIN       19

#define TEMP_OK_CH1_PIN     11
#define TEMP_OK_CH2_PIN     14

#define LCD_ALARM_CH1_PIN   6
#define LCD_ALARM_CH2_PIN   21

#define PD_CH1_PIN          8
#define PD_CH2_PIN          26

#define I2C_CH1_SDA_PIN 0
#define I2C_CH1_SCL_PIN 1

#define I2C_CH2_SDA_PIN 2
#define I2C_CH2_SCL_PIN 3

#define LCD_RX_CH1_PIN 5
#define LCD_RX_CH2_PIN 13

#define LCD_BAUD 460800

#define ELAPSED_TIME_CMD 0x00
#define TEMPERATURE_CMD 0x01
#define ET_TP_I2C_CMD 0x02
#define LCD_GOOD_CMD 0x40
#define LVDS_TIMEOUT_CMD 0x41
#define DRP_CMD 0x42
#define VDDSA_CMD 0x43
#define COPR1_CMD 0x50
#define COPR2_CMD 0x51
#define COPL1_CMD 0x52
#define COPL2_CMD 0x53
#define ENABLE_SOURCE_CMD 0x60 
#define ENABLE_GATE_CMD 0x61
#define ESD_BUS_SW_CMD  0x63
#define BIT_CMD 0x70
#define VDDSA_CURRENT_CMD 0x71
#define FPGA_CMD 0xF0

#define ADDR_LED_DAY 0x3a
#define ADDR_LED_NIGHT 0x3b
#define ADDR_THM1 0X50
#define ADDR_THM2 0X55
#define ADDR_BIT  0x70
#define ADDR_TOUCH_B 0x0b
#define ADDR_TOUCH_A 0x0a

#define CH1 1 // Connection to Driver Board 1
#define CH2 2 // Connection to Driver Board 2

#define RAIL1 1 // First LED Rail (when reading thermistor)
#define RAIL2 2 // Second LED Rail (when reading thermistor)

extern critical_section_t critsec;