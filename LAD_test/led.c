

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
#define THERM_VOLTAGE_REG 0x00
const uint16_t INIT_VAL = 0XFFFF;
const uint16_t OFF_VAL = 0X0000; 
const uint16_t SLOPE_VAL = 0X030A;

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
            i2c_reg_write(ch,ADDR_LED_DAY,0X0E,(uint8_t *)&INIT_VAL,2);
            i2c_reg_write(ch,ADDR_LED_DAY,0X10,(uint8_t *)&INIT_VAL,2);
            //i2c_reg_write(ch,ADDR_LED_DAY,0X12,(uint8_t *)&INIT_VAL,2);
            i2c_reg_write(ch,ADDR_LED_NIGHT,0X0E,(uint8_t *)&INIT_VAL,2);
            i2c_reg_write(ch,ADDR_LED_NIGHT,0X10,(uint8_t *)&INIT_VAL,2);
            //i2c_reg_write(ch,ADDR_LED_NIGHT,0X12,(uint8_t *)&INIT_VAL,2);

            // turn off the leds
            i2c_reg_write(ch,ADDR_LED_DAY,PWM_REG,(uint8_t *)&OFF_VAL,2);
            i2c_reg_write(ch,ADDR_LED_NIGHT,PWM_REG,(uint8_t *)&OFF_VAL,2);
            // configure slope function
            i2c_reg_write(ch,ADDR_LED_DAY,0X04,(uint8_t *)&SLOPE_VAL,2);
            i2c_reg_write(ch,ADDR_LED_NIGHT,0X04,(uint8_t *)&SLOPE_VAL,2);
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

float read_temperature(int rail, int ch)
{
    if (ch & connectedChannels)
    {
        uint8_t buf[2];
        uint16_t value; 
        float volt,temp;
        const int samples = 60;
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