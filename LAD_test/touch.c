#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "IDC_IO.h"
#include "touch.h"
#include "i2c.h"

int i2c_tchannels = 0;
uint8_t touch_addr_ch1, touch_addr_ch2;

int touch_init_all()
{

    i2c_tchannels = 0;
    for (int i=CH1;i<=CH2;i++)
    {

        if (qScan(i,ADDR_TOUCH_A) == ADDR_TOUCH_A)
        {
            printf("touch CH%d, I2C Addr = 0x%x\r\n",i,ADDR_TOUCH_A);
            i2c_tchannels |= i;
            if (i==CH1)
            {
                touch_addr_ch1 = ADDR_TOUCH_A;
            }
            else if (i==CH2)
            {
                touch_addr_ch2 = ADDR_TOUCH_A;
            }
        }
        else if (qScan(i,ADDR_TOUCH_B) == ADDR_TOUCH_B)
        {
            printf("touch CH%d, I2C Addr = 0x%x\r\n",i,ADDR_TOUCH_B);
            i2c_tchannels |= i;
            if (i==CH1)
            {
                touch_addr_ch1 = ADDR_TOUCH_B;
            }
            else if (i==CH2)
            {
                touch_addr_ch2 = ADDR_TOUCH_B;
            }
        }
    }
    return i2c_tchannels;
}

/// If the touch data indicates a multi-touch return the a bit array indicating the touch numbers 
// the touch number is between 0000 0001 and 0001 1111
/// if it is not a multi-touch, return 0 
int is_multi_touch(uint8_t *result)
{
    if (result[0] != 0x90)
    {
        printf("invalid response (header = 0x%x)\r\n",result[0]);
        return false;
    }   

    if (result[17] == 0)
    {
        return (result[1] & 0x1F);
    }
    return 0;
}

int is_gesture(uint8_t *result)
{
    if (result[0] != 0x90)
    {
        printf("invalid response (header = 0x%x)\r\n",result[0]);
        return 0;
    }   

    if ((result[17]>=63) && (result[17]<=77))
    {
        return 1;
    }
    return 0;
}

/// @brief Check to see if there is touch data for touchNum, and return the
/// pixel coordinits in *x and *y 
/// @param result byte array[TOUCH_RESP_LEN] holding touch info
/// @param touchNum 0-5, touch to get coordiant of
/// @param x pointer to memory receiving x coordinate
/// @param y pointer to memory receiving y coordinate
/// @return true if touchNum was a valid touch
bool get_touch_pixel(uint8_t *result, uint8_t touchNum, uint16_t *x, uint16_t *y)
{
    uint8_t offset = (touchNum*3) -1;
  
    if (result[1] & (1<<(touchNum-1)))
    {
        *x = ((result[offset]&0xF0)<<4) + result[offset+1];
        *y = ((result[offset]&0x0F)<<8) + result[offset+2];
        *x=(2560* *x)/4096;
        *y=(1024* *y)/4096;
        return true;
    }
    return false;
}

void print_touch(uint8_t *result)
{
    if (result[0] != 0x90)
    {
        printf("Invalid touch response (header = 0x%x)\r\n",result[0]);
        return;
    }
    
    switch(result[17])
    {
        case 0:
            printf("Gesture Type: Multi-Touch\r\n");
            break;
        case 0x60:
            printf("Gesture Type: Single Tap\r\n");
            break;
        case 0x61:
            printf("Gesture Type: Double Tap\r\n");
            break;
        case 0x62:
            printf("Gesture Type: Hold\r\n");
            break;
        case 0x63:
            printf("Gesture Type: Swipe Up\r\n");
            break;
        case 0x64:
            printf("Gesture Type: Swipe Down\r\n");
            break;
        case 0x65:
            printf("Gesture Type: Swipe Left\r\n");
            break;
        case 0x66:
            printf("Gesture Type: Swipe Right\r\n");
            break;
        case 0x70:
            printf("Gesture Type: Zoom In\r\n");
            break;
        case 0x71:
            printf("Gesture Type: Zoom Out\r\n");
            break;
        case 0x72:
            printf("Gesture Type: Rotate Left\r\n");
            break;
        case 0x73:
            printf("Gesture Type: Rotate Right\r\n");
            break;
        case 0x74:
            printf("Gesture Type: Scroll Up\r\n");
            break;
        case 0x75:
            printf("Gesture Type: Scroll Down\r\n");
            break;
        case 0x76:
            printf("Gesture Type: Scroll Left\r\n");
            break;
        case 0x77:
            printf("Gesture Type: Scroll Right\r\n");
            break;
        default:
            printf("Unknown Gesture Reported 0x%x\r\n",result[17]);
    }

    printf("Touches: \t%0b \r\n",result[1]);
    for (int i=1; i<=5;i++)
    {
        uint16_t x,y;
        if (get_touch_pixel(result,i,&x,&y))
        {
            printf("touch %d (x,y):\t x=%d, y=%d \r\n",i,x,y);
        }
    }
    
    if (result[20] != 0)
        printf("%x Stuck Touch(s) reported\r\n",result[20]);
}

uint8_t read_next_touch(uint8_t *result,int ch)
{
    uint int_pin = (ch==CH1)?TOUCH_CH1_PIN:TOUCH_CH2_PIN;
    
    uint32_t startTime;
    startTime = time_us_32();
    while (gpio_get(int_pin)) // interupt goes low when data available
    {
        if (time_us_32() > startTime+(5*1000000)) // give up after 5 sec
        {
            return 1;
        }
    }
    return read_touch(result,ch);
}

/// @brief   read touch info for the next 5 sec looking for a gesture
/// @param result pointer to buffer[TOUCH_RESP_LEN]
/// @param ch channel to read CH1 or CH2
/// @return 1 if found of 0 if not found
uint8_t read_next_gesture(uint8_t *result,int ch)
{
    uint int_pin = (ch==CH1)?TOUCH_CH1_PIN:TOUCH_CH2_PIN;
    
    uint32_t startTime;
    startTime = time_us_32();

    while(true)
    {
        while (gpio_get(int_pin)) // interupt goes low when data available
        {
            if (time_us_32() > startTime+(5*1000000)) // give up after 5 sec
                return 0;
        }
        if (0==read_touch(result,ch))
        {
            uint8_t rc = is_gesture(result);
            if (rc)
            {
                return rc;
            }
        }

    }
}

uint8_t get_touch_addr(int ch)
{
    return (ch==CH1)?touch_addr_ch1:touch_addr_ch2;
}
uint8_t read_touch(uint8_t *result,int ch)
{
    uint16_t reg = 0x0000;
    if (ch & i2c_tchannels)
    {
        
        int count = i2c_reg_read16(ch,get_touch_addr(ch),reg,result,TOUCH_RESP_LEN);
        if (count!= TOUCH_RESP_LEN)
        {
            printf("Read returned %d bytes (expected %d)\r\n",count,TOUCH_RESP_LEN);
            return 2;
        }
        return 0;
    }
    return 3;
}

uint32_t read_touch_fw_version(int ch)
{
    uint16_t reg = 0xA020;
    uint32_t result=0;
    uint8_t ver[5];
    if (ch & i2c_tchannels)
    {  
        uint8_t addr = get_touch_addr(ch);
        int count = i2c_reg_read16(ch,addr,reg,ver,5);  // FW resp is 5 bytes
        if (count!= 5)
        {
            printf("Read returned %d bytes (expected %d)\r\n",count,5);
            return 0;
        }
        if ((ver[0] != 0xA0) || (ver[1] != 0x20))
        {
            printf("Unexpected Version Response, Header = 0x%x%x, expected 0xA020\r\n",ver[0],ver[1]);
            //return 0;
        }
        result = (ver[2]<<16) | (ver[3]<<8) | ver[4];
        return result;
    }
    return 0;
}