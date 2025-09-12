#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include <ctype.h>
#include <stdlib.h>
#include "adv7393.h"
#include "idc.h"

const int MAPSIZE = 4;
const unsigned char  I2CMap[] = { 0x00,0x1e,
                                  0x02,0x10, // 
                                  //0x80,0x10,
                                  0x82,0xC9,  // SQ pixel off
                                  //0x83,0x04, // output voltage levels
                                  0x84,0x40,  // Color Bars, active pixels
                                  //0x86,0x02,
                                  //0x87,0x80,
                                  //0x88,0x10, // 16bit RGB
                                  //0x89,0x00,
                                  //0x8A,0x04,  // SD Timijng Reg 1
                                  //0x8B,0x00, //HSYNC,VSINC Timing
                                  };

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/
//#define PWM_POT              47     //b'00101111



const int  SPEED_100KHZ   =  100000;


adv7393::adv7393(uint8_t sda, uint8_t scl, i2c_inst_t *pi2c_):
pi2c(pi2c_)
{
    advAddr = 0x54>>1; // default address (7 bits)
    i2c_init(pi2c, SPEED_100KHZ);
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);  
    gpio_pull_up(sda);
    gpio_pull_up(scl);
    bi_decl(bi_2pins_with_func(sda, scl, GPIO_FUNC_I2C));
}

bool adv7393::checkForDevice()
{

    int response;
    response = readRegister(0);
    if (response == -1)
    {
        return false;
    }
    else 
    {
        return true;
    }

}


bool adv7393::writeRegister(uint8_t reg, uint8_t value)
{
    uint8_t data[2];

    data[1]=value;
    data[0]=reg; // command = 2 tells chip to write next byte to RDAC

    //debug_print("Write 0X%02X to 0X%02X \r\n",data[1],data[0]);

    if (i2c_write_blocking(pi2c, advAddr, data, 2, false) != 2)
    {
        debug_print(" Error Writing to Register 0x%02X \n\r", reg);
        return false;
    }
    return true;
}

int adv7393::readRegister(uint8_t reg)
{
    uint8_t cmd[1];
    uint8_t value;

    cmd[0]=reg;
    int rc = i2c_write_timeout_us(pi2c, advAddr, cmd, 1, true,16000);
    if (rc == PICO_ERROR_GENERIC ) 
    {
        printf ("PICO_ERROR_GENERIC Error writing/reading from I2C , ADDR=0x%x, Reg=0x%x\r\n",advAddr,reg);
        return 0;
    }
    else if (rc == PICO_ERROR_TIMEOUT)
    {
        printf ("PICO_ERROR_TIMEOUT Error writing/reading from I2C , ADDR=0x%x, Reg=0x%x\r\n",advAddr,reg);
        return 0;
    }

    int num_bytes_read = i2c_read_blocking(pi2c, advAddr, &value, 1, false);
    if (num_bytes_read == PICO_ERROR_GENERIC)
    {
        printf("Error reading Addr(0x%x) Reg(0x%x) (no response)",advAddr,reg);
    }


    return (uint8_t)value;
}


int adv7393::loadArray(const uint8_t *map,int mapSize)
{
    for (int i=0; i<MAPSIZE*2; i+=2)
    {
        if (!writeRegister(map[i],map[i+1]))
            return -1;
    }
    return mapSize;
}

bool adv7393::dumpRegisters()
{
    for (int i=0;i<0x17;i++)
    {
        debug_print("0X%2.2X = 0x%2.2X\r\n",i,readRegister(i));
    }
    for (int i=0x80;i<0xA5;i++)
    {
        debug_print("0X%2.2X = 0x%2.2X\r\n",i,readRegister(i));
    }
    return true;
}
bool adv7393::configureTestPatern()
{
    if (MAPSIZE != loadArray(I2CMap,MAPSIZE))
    {
        //debug_print(" Failed to configure Test Pattern Generator \r\n");
        return false;
    }
    //dumpRegisters();
    return true;
}

