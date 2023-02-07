#ifndef _LED_I2C_H_
#define _LED_I2C_H_

#ifdef __cplusplus
 extern "C" {
#endif


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

/// @brief get the lacklight pwm duty cycle 
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

#ifdef __cplusplus
 }
#endif

#endif 