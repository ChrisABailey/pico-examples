

#include <stdio.h>
#include "pico/float.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "i2c.h"
#include "IDC_IO.h"
#include "led.h"

bool dayMode = true;
uint16_t pwm = 0;
int connectedChannels=0;

#define PWM_REG 0x00
#define CURRENT_REG 0x02
#define SUPPLY_REG 0x0E // SUPLY_STATUS (supply voltage info)
#define BOOST_REG 0x10  // Reg with BOOST_STATUS (boost circuit info)
#define LED_REG   0x12  // reg weith LED_STATUS (led info)
#define THERM_VOLTAGE_REG 0x00

#define SUPPLY_VIN_OVER_CURRENT  0x80U
#define SUPPLY_VDD_UNDER_VOLTAGE  0x20U
#define SUPPLY_VIN_OVER_VOLTAGE  0x08U
#define SUPPLY_VIN_UNDER_VOLTAGE 0x02U
#define SUPPLY_OTHER             0xFF00U 
#define BOOST_OVP_LOW 0x02U
#define BOOST_OVP_HIGH 0x08U
#define BOOST_OVER_CURRENT 0x20U
#define BOOST_THERMAL_SHUTDOWN 0x80U
#define BOOST_OTHER 0xFF00U
#define LED_STATUS 0x400U
#define LED_SHORT_GND 0x100U
#define LED_OPEN   0x40U
#define LED_SHORT  0x80U
#define LED_FAULT1 0x01U
#define LED_FAULT2 0x02U
#define LED_FAULT3 0x04U
#define LED_FAULT4 0x08U
#define LED_OTHER  0x7800U
uint8_t INIT_VAL[] = {0XFF,0xFF};  // Clear the supply_status or boost_status regs
uint8_t LED_INIT_VAL[] = {0xFE,0x00}; // Clears LED_STATUS Reg
uint8_t OFF_VAL[] = {0X00,0x00}; 
uint8_t SLOPE_VAL[] = {0X03, 0x0A};



int get_connected_channels() 
{
    return connectedChannels;
}

void set_connected_channels(int ch) 
{
    for (int c = 1;c<3;c++){
        if ((ch & c) == 0)
        {  // before we disable the channel, turn off the backlight
            day(0,c);
        }
    }
    
    connectedChannels = ch;
    set_pwm(pwm); // enable any new channels and adj max PWM
}

float get_daymode()
{
    return dayMode;
}

float get_pwm()
{
    return pwm/65535.0f;
}

/// @brief Set the Backlight PWM Percentage (0:1.0)
/// @return new PWM percentage
float set_pwm(float new)
{
    uint8_t pwmAddr;
    if (dayMode)
    {
        if ((connectedChannels == 0x03) && (new > 0.8))
        {  // only drive 2 channels up to 80% pwm
            new = 0.8;
        }
        else if (new > 1.0)
        { // single channels can go to 100%
            new = 1.0;
        }

        pwmAddr = ADDR_LED_DAY;
    }
    else
    {
        if (new > 1.0)
            new = 1.0;
        pwmAddr = ADDR_LED_NIGHT;        
    }

    pwm = 65535 * new;
    uint8_t data[2];
    data[0]=pwm&0x0ff;
    data[1]= pwm>>8;
    for (int ch = 1;ch<3;ch++){
        if (ch & connectedChannels)
        {
            int count = i2c_reg_write(ch,pwmAddr,PWM_REG,data,2);
            if (count != 2)
            {
                printf("wrote %d bytes to CH%d (expected 2)\r\n",ch,count);
            }
        }
    }

    return new;
}

int led_init_all()
{
    connectedChannels = 0;

    for (int ch=1;ch<3;ch++) 
    {
        if ((qScan(ch,ADDR_LED_DAY)== ADDR_LED_DAY)  && (qScan(ch,ADDR_LED_NIGHT)== ADDR_LED_NIGHT))
        {
            connectedChannels |= ch;
            // THese Init values reset bit values that need to be cleared after power on.
            i2c_reg_write(ch,ADDR_LED_DAY,SUPPLY_REG,INIT_VAL,2);
            i2c_reg_write(ch,ADDR_LED_DAY,BOOST_REG,INIT_VAL,2);
            i2c_reg_write(ch,ADDR_LED_DAY,LED_REG,LED_INIT_VAL,2);

            i2c_reg_write(ch,ADDR_LED_NIGHT,SUPPLY_REG,INIT_VAL,2);
            i2c_reg_write(ch,ADDR_LED_NIGHT,BOOST_REG,INIT_VAL,2);
            i2c_reg_write(ch,ADDR_LED_NIGHT,LED_REG,LED_INIT_VAL,2);

            // turn off the leds
            i2c_reg_write(ch,ADDR_LED_DAY,PWM_REG,OFF_VAL,2);
            i2c_reg_write(ch,ADDR_LED_NIGHT,PWM_REG,OFF_VAL,2);
            // configure slope function
            i2c_reg_write(ch,ADDR_LED_DAY,0X04,SLOPE_VAL,2);
            i2c_reg_write(ch,ADDR_LED_NIGHT,0X04,SLOPE_VAL,2);
        }
        
    }
    return connectedChannels;
}

float day(float pwm_percent,int ch)
{
    if (ch & connectedChannels)
    {
        if (pwm_percent > 0.8)
            pwm_percent = 0.8;
        dayMode = true;
        i2c_reg_write(ch,ADDR_LED_NIGHT,PWM_REG,(uint8_t *)&OFF_VAL,2);
        pwm = 65535 * pwm_percent;
        int count = i2c_reg_write(ch,ADDR_LED_DAY,PWM_REG,(uint8_t *)&pwm,2);
        if (count != 2)
        {   
            printf("Failed writing day PWM reg");
            return -1;
        }
        return pwm_percent;
    }
    return 0;
}

float night(float pwm_percent,int ch)
{
    if (ch & connectedChannels)
    {
        if (pwm_percent > 1.0)
            pwm_percent = 1.0;
        dayMode = false;
        i2c_reg_write(ch,ADDR_LED_DAY,PWM_REG,(uint8_t *)&OFF_VAL,2);
        pwm = 65535 * pwm_percent;
        int count = i2c_reg_write(ch,ADDR_LED_NIGHT,PWM_REG,(uint8_t *)&pwm,2);
        if (count != 2)
            return -1;
        return pwm_percent;
    }
    return 0;
}

uint16_t read_current(int ch)
{
    if (ch & connectedChannels)
    {
        uint16_t value; 
        i2c_reg_read(ch,ADDR_LED_DAY,CURRENT_REG,(uint8_t *)&value,2);
        return value;
    }
    return 0;
}

/// @brief Reads the LED Driver ICs status
/// @param ch (CH1 | CH2)
/// @return 16 bit status register
uint32_t read_led_bit(int ch)
{
    if (ch & connectedChannels)
    {
        uint16_t value; 
        uint32_t errors = 0;
        i2c_reg_read(ch,ADDR_LED_DAY,SUPPLY_REG,(uint8_t *)&value,2);
        errors |= (value & SUPPLY_VIN_OVER_CURRENT )?FAULT_DAY_INPUT_POWER:0;
        errors |= (value & SUPPLY_VIN_OVER_VOLTAGE)?FAULT_DAY_INPUT_POWER:0;
        errors |= (value & SUPPLY_VIN_UNDER_VOLTAGE)?FAULT_DAY_INPUT_POWER:0;        
        errors |= (value & (SUPPLY_VDD_UNDER_VOLTAGE | SUPPLY_OTHER))?FAULT_DAY_OTHER_POWER:0;
        
        i2c_reg_read(ch,ADDR_LED_DAY,BOOST_REG,(uint8_t *)&value,2);
        errors |= (value & BOOST_THERMAL_SHUTDOWN)?FAULT_DAY_BOOST_THERMAL:0;
        errors |= (value & (BOOST_OVP_LOW|BOOST_OVP_HIGH))?FAULT_DAY_BOOST_OVP :0;
        errors |= (value & (BOOST_OTHER|BOOST_OVER_CURRENT))?FAULT_DAY_BOOST_OTHER :0;
        
        i2c_reg_read(ch,ADDR_LED_DAY,LED_REG,(uint8_t *)&value,2);
        errors |= (value & (LED_OPEN))?FAULT_DAY_LED_OPEN     :0;
        errors |= (value & (LED_SHORT|LED_SHORT_GND))?FAULT_DAY_LED_SHORT    :0;
        errors |= (value & (LED_OTHER|LED_STATUS))?FAULT_DAY_LED_OTHER:0;
        errors |= (value & (LED_FAULT1))?FAULT_DAY_LED1     :0;
        errors |= (value & (LED_FAULT2))?FAULT_DAY_LED2     :0;
        errors |= (value & (LED_FAULT3))?FAULT_DAY_LED3     :0;
        errors |= (value & (LED_FAULT4))?FAULT_DAY_LED4     :0;

        i2c_reg_read(ch,ADDR_LED_NIGHT,SUPPLY_REG,(uint8_t *)&value,2);
        errors |= (value & SUPPLY_VIN_OVER_CURRENT )?FAULT_NIGHT_INPUT_POWER:0;
        errors |= (value & SUPPLY_VIN_OVER_VOLTAGE)?FAULT_NIGHT_INPUT_POWER:0;
        errors |= (value & SUPPLY_VIN_UNDER_VOLTAGE)?FAULT_NIGHT_INPUT_POWER:0;        
        errors |= (value & (SUPPLY_VDD_UNDER_VOLTAGE | SUPPLY_OTHER))?FAULT_NIGHT_OTHER_POWER:0;
        
        i2c_reg_read(ch,ADDR_LED_NIGHT,BOOST_REG,(uint8_t *)&value,2);
        errors |= (value & BOOST_THERMAL_SHUTDOWN)?FAULT_NIGHT_BOOST_THERMAL:0;
        errors |= (value & (BOOST_OVP_LOW|BOOST_OVP_HIGH))?FAULT_NIGHT_BOOST_OVP    :0;
        errors |= (value & (BOOST_OTHER|BOOST_OVER_CURRENT))?FAULT_NIGHT_BOOST_OTHER  :0;
        
        i2c_reg_read(ch,ADDR_LED_NIGHT,LED_REG,(uint8_t *)&value,2);
        errors |= (value & (LED_OPEN))?FAULT_NIGHT_LED_OPEN     :0;
        errors |= (value & (LED_SHORT|LED_SHORT_GND))?FAULT_NIGHT_LED_SHORT    :0;
        errors |= (value & (LED_OTHER | LED_STATUS))?FAULT_NIGHT_LED_OTHER:0;
        errors |= (value & (LED_FAULT1))?FAULT_NIGHT_LED1     :0;
        errors |= (value & (LED_FAULT2))?FAULT_NIGHT_LED2     :0;
        errors |= (value & (LED_FAULT3))?FAULT_NIGHT_LED3     :0;
        errors |= (value & (LED_FAULT4))?FAULT_NIGHT_LED4     :0;

        return errors;
    }
    return 0;
}

void print_led_faults(uint16_t result)
{
    if (result & FAULT_DAY_INPUT_POWER)
        printf("Input Power Error ");
    if (result & FAULT_DAY_BOOST_THERMAL)
        printf("Driver IC over-temp ");
    if (result & FAULT_DAY_BOOST_OVP)
        printf("Output Boost OVP(LED Rail issue) ");
    if (result & FAULT_DAY_BOOST_OTHER)
        printf("Output Boost Other ");
    if (result & FAULT_DAY_LED_OPEN)
        printf("LED String open circuit ");
    if (result & FAULT_DAY_LED_SHORT)
        printf("LED string short circuit ");
    if (result & FAULT_DAY_LED_OTHER)
        printf("LED string other error ");
    if (result & FAULT_DAY_LED1) 
        printf("Fault on LED String 1 ");
    if (result & FAULT_DAY_LED2) 
        printf("Fault on LED String 2 ");
    if (result & FAULT_DAY_LED3) 
        printf("Fault on LED String 3 ");
    if (result & FAULT_DAY_LED4) 
        printf("Fault on LED String 4 ");

}

/// @brief print a human redable summary of the 16bit LED driver Status 
/// @param result Driver IC status (returnd from read_led_bit())
void print_led_bit(uint32_t result)
{
    if (result == 0)
    {
        printf(" OK ");
    }
    else 
    {
        if (result & 0x0000FFFF)
        {
            printf("Day: ");
            print_led_faults(result & 0xFFFF);
            printf("\r\n");
        }
        if (result & 0xFFFF0000)
        {
            printf("NGT:");
            print_led_faults(result >> 16);
            printf("\r\n");
        }
    }
    
}



/// @brief Given the status from the LED Driver ICs, try to suggest root causes 
/// @param result1 Driver IC status (returnd from read_led_bit(CH1))
/// @param result2 Driver IC status (returnd from read_led_bit(CH2))
void isolate_led_bit(uint32_t result1,uint32_t result2)
{
    if ((result1 & FAULT_DAY_LED_OPEN) && (result1 & FAULT_NIGHT_LED_OPEN) && ((result2 & FAULT_DAY_LED_OPEN)) && (result2 & FAULT_NIGHT_LED_OPEN))
    {
        printf("LED_OPEN_FAULT on all Channels check Board to Board Cable. \r\n");
    }
}

/// @brief Read the LED rail temperature 
/// @param rail 0 or 1
/// @param ch (CH1 | CH2)
/// @return temperature in Deg C
float read_temperature(int rail, int ch)
{
    if (ch & connectedChannels)
    {
        uint8_t buf[2];
        uint16_t value; 
        float volt,temp;
        const int samples = 30;
        uint32_t total = 0;
        for (int i=0;i<samples;i++)
        {
            i2c_reg_read(ch,(rail==RAIL1)?ADDR_THM1:ADDR_THM2,THERM_VOLTAGE_REG,buf,2);
            value = ((buf[0] << 4) & 0xf0) + ((buf[1] >> 4) & 0x0f);
            sleep_us(5);
            total += value;
        }

        //printf("channel %d Thermistor %d register = 0x%x 0x%x (%d) \n",ch,rail,buf[0],buf[1],value); 
        volt = 0.006445 + (((float)total/samples) * 0.01279); 
        //printf("Thermistor voltage = %2.2fV\n",volt);
        temp = -17.117*powintf(volt,5) + 136.43*powintf(volt,4) - 422.64* powintf(volt,3) + 637.92*powintf(volt,2) - 519.45*volt + 276.24;
        //printf("Thermistor Temp = %f degC",temp);
        return temp;
    }
    return 0;
}