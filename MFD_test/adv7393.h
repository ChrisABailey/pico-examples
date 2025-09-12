/*
 * @brief Common digital IO support functions
 *
 * @note
 * Copyright(C) IDC-LCD, 2017
 * All rights reserved.
 *
*/
#ifndef _adv7393_H_
#define _adv7393_H_
#include "hardware/i2c.h"
#include "T38BoardIO.h"



class adv7393
{

    protected:
        i2c_inst_t *pi2c;
        uint8_t advAddr = 0x54;
        
    public:



        adv7393(uint8_t sda, uint8_t scl, i2c_inst_t *pi2c_ = ADV7393_BLOCK);

        // scan and print a table of all I2C devices found
        void i2c_scan();

        // this finction is generally only for debug as the address is static
        void setAddress(uint8_t addr) {advAddr = addr;}
        
        // write one byte of data to reg return false on fail
        bool writeRegister(uint8_t reg, uint8_t value);

        // read one byte of data from reg.  Return -1 on fail
        int readRegister(uint8_t reg);
        bool configureTestPatern();
        
        int loadArray(const uint8_t *MAP,int MAPSIZE);
        bool dumpRegisters();

        bool checkForDevice();      
        
        
    protected:
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

        int i2c_reg_read_n(int channel,
                           const uint addr,
                           const uint8_t *reg,
                           const uint len,
                           uint8_t *buf,
                           const uint8_t nbytes);

        //bool load();
        //bool save();

};
#endif 