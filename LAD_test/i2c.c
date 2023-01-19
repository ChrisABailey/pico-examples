#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "pico/sync.h"
#include "hardware/i2c.h"
#include "IDC_IO.h"
#include "i2c.h"

// I2C reserves some addresses for special purposes. We exclude these from the scan.
// These are any addresses of the form 000 0xxx or 111 1xxx
bool reserved_addr(uint8_t addr) {
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

void i2c_init_all(){
    i2c_init(i2c0, 10 * 1000);
    i2c_init(i2c1, 10 * 1000);
    gpio_set_function(I2C_CH1_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_CH1_SCL_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_CH2_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_CH2_SCL_PIN, GPIO_FUNC_I2C);
    //gpio_pull_up(I2C_CH1_SDA_PIN);
    //gpio_pull_up(I2C_CH1_SCL_PIN);
    //gpio_pull_up(I2C_CH2_SDA_PIN);
    //gpio_pull_up(I2C_CH2_SCL_PIN);
    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(I2C_CH1_SDA_PIN, I2C_CH1_SCL_PIN, GPIO_FUNC_I2C));
    bi_decl(bi_2pins_with_func(I2C_CH2_SDA_PIN, I2C_CH2_SCL_PIN, GPIO_FUNC_I2C));
}

void i2c_scan(int channel) {
    printf("\nI2C Bus Scan CH%d\n",channel);
    printf("   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

    for (int addr = 0; addr < (1 << 7); ++addr) {
        if (addr % 16 == 0) {
            printf("%02x ", addr);
        }

        // Perform a 1-byte dummy read from the probe address. If a slave
        // acknowledges this address, the function returns the number of bytes
        // transferred. If the address byte is ignored, the function returns
        // -1.

        // Skip over any reserved addresses.
        int ret;
        uint8_t rxdata;
        if (reserved_addr(addr))
            ret = PICO_ERROR_GENERIC;
        else
        {
            critical_section_enter_blocking(&critsec);
            ret = i2c_read_timeout_us((channel == CH1)?i2c0:i2c1, addr, &rxdata, 1, false,16000);
            critical_section_exit(&critsec);
        }
        printf(ret < 0 ? "." : "@");
        printf(addr % 16 == 15 ? "\n" : "  ");
    }
}

// Write nbytes byte in buf to the specified register
// return number of bytes written
int i2c_reg_write(  int channel, 
                const uint addr, 
                const uint8_t reg, 
                uint8_t *buf,
                const uint8_t nbytes) {

    uint8_t msg[nbytes + 1];

    // Check to make sure caller is sending 1 or more bytes
    if (nbytes < 1) {
        return 0;
    }

    // Append register address to front of data packet
    msg[0] = reg;
    for (int i = 0; i < nbytes; i++) {
        msg[i + 1] = buf[i];
    }

    critical_section_enter_blocking(&critsec);
    // Write data to register(s) over I2C
    int br = i2c_write_timeout_us((channel==CH1)? i2c0:i2c1, addr, msg, (nbytes + 1), false,16000)-1;
    critical_section_exit(&critsec);
    return br;
}

// Read byte(s) from specified register. If nbytes > 1, read from consecutive
// registers.
int i2c_reg_read(  int channel,
                const uint addr,
                const uint8_t reg,
                uint8_t *buf,
                const uint8_t nbytes) {

    int num_bytes_read = 0;

    // Check to make sure caller is asking for 1 or more bytes
    if (nbytes < 1) {
        return 0;
    }

    // we get an error if the read status interupt occurs mid write/read
    // this makes sure they dont conflict
    critical_section_enter_blocking (&critsec);

    // Read data from register(s) over I2C
    int rc = i2c_write_timeout_us((channel==CH1)? i2c0:i2c1, addr, &reg, 1, true,16000);
    if (rc == PICO_ERROR_GENERIC || rc == PICO_ERROR_TIMEOUT)
    {
        printf ("Error reading from I2C CH%d, ADDR=0x%x, Reg=0x%x\r\n",channel,addr,reg);
        return 0;
    }
    
    num_bytes_read = i2c_read_blocking((channel==CH1)? i2c0:i2c1, addr, buf, nbytes, false);
    critical_section_exit (&critsec);
    if (num_bytes_read == PICO_ERROR_GENERIC)
    {
        printf("Error reading CH%d Addr(0x%x) Reg(5x%x)",channel,addr,reg);
    }

    return num_bytes_read;
}