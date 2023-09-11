#ifndef _LED_I2C_H_
#define _LED_I2C_H_

#ifdef __cplusplus
 extern "C" {
#endif

#define FAULT_DAY_INPUT_POWER   0x01U
#define FAULT_DAY_OTHER_POWER   0x02U
#define FAULT_DAY_BOOST_THERMAL 0x04U
#define FAULT_DAY_BOOST_OVP     0x08U
#define FAULT_DAY_BOOST_OTHER   0x10U
#define FAULT_DAY_LED_OPEN      0x20U
#define FAULT_DAY_LED_SHORT     0x40U
#define FAULT_DAY_LED1          0x80U
#define FAULT_DAY_LED2          0x100U
#define FAULT_DAY_LED3          0x200U
#define FAULT_DAY_LED4          0x400U
#define FAULT_DAY_LED_OTHER     0X800U
#define FAULT_NIGHT_INPUT_POWER FAULT_DAY_INPUT_POWER << 16
#define FAULT_NIGHT_OTHER_POWER FAULT_DAY_OTHER_POWER << 16
#define FAULT_NIGHT_BOOST_THERMAL FAULT_DAY_BOOST_THERMAL << 16
#define FAULT_NIGHT_BOOST_OVP   FAULT_DAY_BOOST_OVP << 16
#define FAULT_NIGHT_BOOST_OTHER FAULT_DAY_BOOST_OTHER << 16
#define FAULT_NIGHT_LED_OPEN    FAULT_DAY_LED_OPEN << 16
#define FAULT_NIGHT_LED_SHORT   FAULT_DAY_LED_SHORT << 16
#define FAULT_NIGHT_LED1        FAULT_DAY_LED1  << 16 
#define FAULT_NIGHT_LED2        FAULT_DAY_LED2  << 16 
#define FAULT_NIGHT_LED3        FAULT_DAY_LED3  << 16 
#define FAULT_NIGHT_LED4        FAULT_DAY_LED4  << 16 
#define FAULT_NIGHT_LED_OTHER   FAULT_DAY_LED_OTHER << 16

// Set up backlight on CH1
// Return bit array of led channels successfully found (CH1 | CH2)
int led_init_all();


/// @brief retrurns current Backlight mode
/// @return true = day, false=night
float get_daymode();

/// @brief Set the backlight to day mode with pwm percentage between 0 and 0.80
/// @param ch = 1 or 2
/// @return new pwm percentage
float day(float pwm_percent,int ch);

// Set the backlight to night mode with pwm percentage between 0 and 1.0
// ch = 1 or 2
float night(float pwm_percent,int ch);

/// @brief return the LED Current (amperage) setting (should bootstrap to 4095 which is max)
/// @param ch channel (CH1,CH2)
/// @return Current value 0 to 4095
uint16_t read_current(int ch);

/// @brief get the backlight pwm duty cycle 
/// @return percent of duty cycle (0 to 1.0)
float get_pwm();


/// @brief set the pwm duty cycle
/// @param new PWM percentage 0 to 1.0 (clamped to 80% in day mode)
/// @return new pwm value(float)
float set_pwm(float new);

/// @brief read LED rail Thermistor Temperature
/// @param rail (RAIL1=1,RAIL2=2) Which of the 2 LED rails to read
/// @param ch (CH1=2, CH2=2) Which Channel to read from
/// @return temterature in Deg Kelvin
float read_temperature(int rail, int ch);

int get_connected_channels();
void set_connected_channels(int ch);

uint32_t read_led_bit(int ch);
void print_led_bit(uint32_t result);
void isolate_led_bit(uint32_t result1,uint32_t result2);

#ifdef __cplusplus
 }
#endif

#endif 