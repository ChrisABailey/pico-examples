#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "IDC_IO.h"
#include "bit.h"
#include "i2c.h"

int i2c_channels = 0;

int bit_init_all()
{
    for (int ch=CH1;ch<=CH2;ch++)
    {
        if (qScan(ch,ADDR_BIT) == ADDR_BIT)
        {
            i2c_channels |= ch;
        }
    }
}
void print_bit(uint8_t result)
{
    if (result == 0xFF)
    {
        printf("OK");
        return;
    }
    printf("|");
    if ((result & 0x01) == 0)
        printf(" LED_N fault |");
    if ((result & 0x02) == 0)
        printf(" LED_D fault |");
    if ((result & 0x04) == 0)
        printf(" 3.3V fault |");
    if ((result & 0x08) == 0)
        printf(" 5V fault |");
    if ((result & 0xF0) != 0xF0)
        printf("Unexpected BIT encoding 0x%2x |",result);
}

uint8_t read_bit(int ch)
{
    uint8_t value = 0; 
    if (ch & i2c_channels)
    {
        int count = i2c_reg_read(ch,ADDR_BIT,0x00,&value,1);
        if (count!= 1)
        {
            printf("Read returned %d bytes (expected 1)\r\n",count);
            return 0;
        }
        return value;
    }
    return 0;
}