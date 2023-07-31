#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "IDC_IO.h"
#include "lcd.h"


#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

LCD_BIT_t last_LCD_BIT_CH1;
LCD_BIT_t last_LCD_BIT_CH2;
int connectedLCDs = 0;

/// @brief zero out BIT structure passed in
void init_BIT_struct(int ch)
{
    LCD_BIT_t *BIT;
    if (ch==CH1)
    {
        BIT = &last_LCD_BIT_CH1;
    }
    else
    {
        BIT = &last_LCD_BIT_CH2;
    }
    BIT->failedReads = 0;
    BIT->elapsedTime.hours = 0;
    BIT->elapsedTime.mins = 0;
    BIT->elapsedTime.secs = 0;

    BIT->temperature  = -99.9f;
    BIT->lcdGood      = false;
    BIT->lvdsTimeout  = false;
    BIT->drpSource    = false;
    BIT->drpGate      = false;

    BIT->vddsa        = false;
    BIT->vddsaI        = 0;

    BIT->copR1.r       = 0x0B;
    BIT->copR1.g       = 0x0A;
    BIT->copR1.b       = 0x0D;
    BIT->copR2.r       = 0x0B;
    BIT->copR2.g       = 0x0A;
    BIT->copR2.b       = 0x0D;
    BIT->copL1.r       = 0x0B;
    BIT->copL1.g       = 0x0A;
    BIT->copL1.b       = 0x0D;
    BIT->copL2.r       = 0x0B;
    BIT->copL2.g       = 0x0A;
    BIT->copL2.b       = 0x0D;

    BIT->sourceEnable  = false;
    BIT->sourceDisable = false;
    BIT->enable        = false;
    BIT->gatePchEnable = false;
    BIT->gatePchDisable= false;
    BIT->gateNchEnable = false;
    BIT->gateNchDisable= false;
    
    BIT->bitLVDS       = 0xBA;
    BIT->bitLCD1       = 0XDE;
    BIT->bitLCD2       = 0xAD;
    BIT->fpga          = 0xff;
}

void lcd_init_all()
{
    connectedLCDs = CH1|CH2;

    // Set up our UART with a basic baud rate.
    uart_init(uart0, LCD_BAUD);
    uart_init(uart1, LCD_BAUD);
    gpio_set_function(LCD_RX_CH1_PIN, GPIO_FUNC_UART);
    gpio_set_function(LCD_RX_CH2_PIN, GPIO_FUNC_UART);
    uart_set_baudrate(uart0, LCD_BAUD);
    uart_set_baudrate(uart1, LCD_BAUD);

    // Set UART flow control CTS/RTS, we don't want these, so turn them off
    uart_set_hw_flow(uart0, false, false);
    uart_set_hw_flow(uart1, false, false);
    // Set our data format
    uart_set_format(uart0, DATA_BITS, STOP_BITS, PARITY);
    uart_set_format(uart1, DATA_BITS, STOP_BITS, PARITY);

    //uart_set_fifo_enabled(uart0, false);
    //uart_set_fifo_enabled(uart1, false);

    // zero out the LCD BIT structures
    init_BIT_struct(CH1);
    init_BIT_struct(CH2);
}

uint8_t get_fpga_ver(int ch) 
{ 
    LCD_BIT_t *BIT;
    if (ch == CH1)
    {
        BIT = &last_LCD_BIT_CH1;
    }
    else
    {
        BIT = &last_LCD_BIT_CH2;
    }
    if (BIT->failedReads > 20) 
        return 0;
    return BIT->fpga;
}

float get_lcd_temp(int ch) 
{ 
    LCD_BIT_t *BIT;
    if (ch == CH1)
    {
        BIT = &last_LCD_BIT_CH1;
    }
    else
    {
        BIT = &last_LCD_BIT_CH2;
    }
    if (BIT->failedReads > 20) 
        return -99.0f;
    return BIT->temperature;
}



//The FPGA returns a series of 6 byte commands, each starts with 0xFF followed by a command ID and 4 bytes of data.
LCD_BIT_t* parseStatus(int ch, uint8_t *lastPacket, int len)
{
    int cur=0,ccount=0;
    uint8_t cmd;
    LCD_BIT_t *BIT;

    if (ch == CH1)
    {
        BIT = &last_LCD_BIT_CH1;
    }
    else
    {
        BIT = &last_LCD_BIT_CH2;
    }
    BIT->failedReads = 0;
    while (cur < len-5)
    {
        if (lastPacket[cur++] == 0xFF) // look for start byte of a cmd
        {
            ccount++;
            cmd = lastPacket[cur];
            switch (cmd)
            {
                case ELAPSED_TIME_CMD:  //elapsed time
                {
                    uint32_t b2,b3;//,b4;
                    b2 = lastPacket[cur+2];
                    b3 = lastPacket[cur+3];
                    //b4 = lastPacket[cur+4];
                    uint32_t eTimeSec = (lastPacket[cur+1] | (b2<<8) | (b3<<16));// | (b4<<24));
                    eTimeSec = eTimeSec/4; // indicator counts in 1/4 sec increments
                    BIT->elapsedTime.hours = eTimeSec / 3600U;
                    BIT->elapsedTime.mins = (eTimeSec%3600U) / 60U;
                    BIT->elapsedTime.secs = eTimeSec %60U;
                    break;
                }
                case TEMPERATURE_CMD: // temperature
                {
                    uint16_t hbit = lastPacket[cur+2];
                    int16_t temp = lastPacket[cur+1] + (hbit<<8);
                    // BIT->temperature = (float)temp / 16.0f;
                    if (temp & 0x0800){
                        temp = temp | 0xf000;
                    }
                    BIT->temperature = (float)temp / 16.0f;
                    //printf("pkt[0x%x][0x%x],temp=0x%x,final=%f\r\n",lastPacket[cur+1],lastPacket[cur+2],temp,BIT->temperature);
                    break;
                }
                case LCD_GOOD_CMD: // LCD Good
                {
                    BIT->lcdGood = (lastPacket[cur+1] == 0x0F);
                    break;
                }
                case LVDS_TIMEOUT_CMD: // LVDS timeout
                {
                    BIT->lvdsTimeout = (lastPacket[cur+1] == 0x0F);
                    break;
                }        
                case DRP_CMD: // DRP timeout
                {
                    BIT->drpSource = (lastPacket[cur+1] == 0x0F);
                    BIT->drpGate = (lastPacket[cur+2] == 0x0F);
                    break;
                }             
                case VDDSA_CMD: // VDDSA Over Current 
                {

                    BIT->vddsa = (lastPacket[cur+1] == 0x0F);;
                    break;
                }            
                case VDDSA_CURRENT_CMD: // VDDSA Current (12 bits)
                {
                    uint16_t hbit = lastPacket[cur+2];
                    BIT->vddsaI = lastPacket[cur+1];
                    BIT->vddsaI += (hbit & 0x0F)<<8;
                    break;
                }
                case COPR1_CMD: // Right LVDS CH1 COP (off screen pixel) color
                {
                    BIT->copR1.r = lastPacket[cur+1];
                    BIT->copR1.g = lastPacket[cur+2];
                    BIT->copR1.b = lastPacket[cur+3];
                    break;
                }
                case COPR2_CMD: // Right LVDS CH2 COP (off screen pixel) color
                {
                    BIT->copR2.r = lastPacket[cur+1];
                    BIT->copR2.g = lastPacket[cur+2];
                    BIT->copR2.b = lastPacket[cur+3];
                    break;
                }
                case COPL1_CMD: // Left LVDS CH1 COP (off screen pixel) color
                {
                    BIT->copL1.r = lastPacket[cur+1];
                    BIT->copL1.g = lastPacket[cur+2];
                    BIT->copL1.b = lastPacket[cur+3];
                    break;
                }
                case COPL2_CMD: // Left LVDS CH2 COP (off screen pixel) color
                {
                    BIT->copL2.r = lastPacket[cur+1];
                    BIT->copL2.g = lastPacket[cur+2];
                    BIT->copL2.b = lastPacket[cur+3];
                    break;
                }
                case ENABLE_SOURCE_CMD: // source Switches enables
                {
                    BIT->sourceEnable = (lastPacket[cur+1] == 0x0F);
                    BIT->sourceDisable = (lastPacket[cur+2] == 0x0F);
                    BIT->enable = (lastPacket[cur+3] == 0x0F);
                    break;
                }
                case ENABLE_GATE_CMD: // gate switch enables
                {
                    BIT->gatePchEnable = (lastPacket[cur+1] == 0x0F);
                    BIT->gatePchDisable = (lastPacket[cur+2] == 0x0F);
                    BIT->gateNchEnable = (lastPacket[cur+3] == 0x0F);
                    BIT->gateNchDisable = (lastPacket[cur+4] == 0x0F);
                    break; 
                }
                case ESD_BUS_SW_CMD: 
                {
                    BIT->ESDHigh = (lastPacket[cur+1] == 0x0F);
                    BIT->ESDLow = (lastPacket[cur+2] == 0x0F);
                    break;
                }
                case BIT_CMD: // pixel -1 (off screen pixel) color
                {
                    BIT->bitLVDS = lastPacket[cur+1];
                    BIT->bitLCD1 = lastPacket[cur+2];
                    BIT->bitLCD2 = lastPacket[cur+3];
                    break;
                }
                case FPGA_CMD: // fpga
                {
                    BIT->fpga = lastPacket[cur+1];
                    break;
                }
                case ET_TP_I2C_CMD:   //Not checked
                    break;
                case 0xFF:  // found another command start so special case
                    cur -= 5;
                    break;
                default:
                {
                    //printf(" unhandled Command = [0x%x] \r\n",cmd);
                }
            }
            cur+=5;
        }

        
    }
    
    if (ccount < 10)
    {
        printf("%d cmds found for CH%d\r\n",ccount,ch);
        printf("%d,%d,%d,%d,%d,%d\r\n",lastPacket[0],lastPacket[1],lastPacket[2],lastPacket[3],lastPacket[4],lastPacket[5]);
    }
    return BIT;
}

#define STATUS_LEN 109  // 102 Bytes in a full status message
#define FRAME_TIME 16667 // Status is returned 5clocks after each VSync so every 16.7mS


///@brief Read Status from CH ch UART and parse it into LCD Status Struct
bool read_LCD_status(int ch) {
    uint8_t lastPacket[STATUS_LEN+10]; // allow for a full packet + a partial cmd from previous pkt 
    uint32_t startTime,currTime;
    int i=0;
    bool start=false;
    uint8_t c;

    // pin GP5 is always associated with UART1 GP13 is assoiated with UART0
    uart_inst_t *uart = (ch==CH2)?uart0:uart1;
    
    if(uart_is_readable_within_us(uart,FRAME_TIME)) // wait up to one frame for data 
    {
        startTime = time_us_32();
    }
    else
    {
        if (ch==CH1)
        {
            if (last_LCD_BIT_CH1.failedReads++ > 100)
            {
                connectedLCDs = connectedLCDs & !CH1;
                return true;
            }
        }
        else
        {
            if (last_LCD_BIT_CH2.failedReads++ > 100)
            {
                connectedLCDs = connectedLCDs & !CH2;
                return true;
            }
        }
        //printf("Uart%d not readable within %duS\r\n",ch-1,FRAME_TIME);
        return true;
    }

    //there is some data in the queue, it should take about 4mS to read all of the data for one frame
    // but there is no guarentee that we are at the beginning of a frame
    currTime=startTime;

    //read full status data or timeout after one frame sec
    while (i<(STATUS_LEN-7) && ((currTime - startTime) < (4500))) //
    {
        // the first status command is always 0xFF00 (elapsed time)

        if (uart_is_readable_within_us(uart,500))
        {
            uart_read_blocking(uart,&c,1);
            if (c==0xff)
            { 
                if (!start)
                { 
                    uart_read_blocking(uart,&c,1);
                    if (c==ELAPSED_TIME_CMD)
                    {
                        lastPacket[i++] = 0xff;
                        lastPacket[i++] = c;
                        uart_read_blocking(uart,&lastPacket[i],4);
                        i+=4;
                        start=true;
                    }
                }
                else
                {
                    lastPacket[i++] = 0xff;
                    uart_read_blocking(uart,&lastPacket[i],5);
                    i+=5;
                }
            }
            
        }

        currTime = time_us_32();
        if (currTime < startTime) 
        {
            startTime=currTime; // the 32 bit timer wraps about once an hour this should handle that case
        }
    }

    if (i>101)
    {  // got a full message so parse it
        //printf("read_status(CH%d) returned %d Bytes\r\n",ch,i);
        parseStatus(ch,lastPacket,i);
        return true;
    }
    else
    {
        //printf("read_status(CH%d) returned %d Bytes\r\n",ch,i);
        return false;
    }

}

void print_status(LCD_BIT_t* BIT)
{
    if (BIT->failedReads > 50)
    {
        printf("LCD not responding (%d failed reads)\r\n",BIT->failedReads);
        return;
    }
    uint16_t h = BIT->elapsedTime.hours;
    uint8_t m = BIT->elapsedTime.mins;
    uint8_t s = BIT->elapsedTime.secs;
    printf("\tElapsed Time  \t= %d:%02d:%02d\r\n",h,m,s);
    printf("\tLCD Temp      \t= %2.1fdegC \r\n",BIT->temperature);
    printf("\tLCD Good      \t= %s \r\n",(BIT->lcdGood)?"OK":"Fail");
    printf("\tLVDS Timeout  \t= %s \r\n",(BIT->lvdsTimeout)?"OK":"Fail");
    printf("\tDRP Source    \t= %s \r\n",(BIT->drpSource)?"OK":"Fail");
    printf("\tDRP Gate      \t= %s \r\n",(BIT->drpGate)?"OK":"Fail");
    printf("\tvdd SA OK     \t= %s \r\n",(BIT->vddsa)?"OK":"Fail");
    printf("\tSource Enable \t= %s \r\n",(BIT->sourceEnable)?"OK":"Fail");
    printf("\tSource Disable\t= %s \r\n",(BIT->sourceDisable)?"OK":"Fail");
    printf("\tEnable        \t= %s \r\n",(BIT->enable)?"OK":"Fail");
    printf("\tGate Pch Enbl \t= %s \r\n",(BIT->gatePchEnable)?"OK":"Fail");
    printf("\tGate Pch Dsbl \t= %s \r\n",(BIT->gatePchDisable)?"OK":"Fail");
    printf("\tGate Nch Enbl \t= %s \r\n",(BIT->gatePchEnable)?"OK":"Fail");
    printf("\tGate Nch Dsbl \t= %s \r\n",(BIT->gatePchDisable)?"OK":"Fail");

    if (BIT->bitLVDS == 0x0F)
    {
        printf("\tBIT LVDS       \t= OK\r\n");
    } else
    {
        printf("\tBIT LVDS       \t= Fail(0x%x)\r\n",BIT->bitLVDS);
    }

    if (BIT->bitLCD1 == 0xFF)
    {
        printf("\tBIT LCD1       \t= OK\r\n");
    } else
    {
        printf("\tBIT LCD1       \t= Fail(0x%x)\r\n",BIT->bitLCD1);
    }

    if (BIT->bitLCD2 == 0x07)
    {
        printf("\tBIT LCD2      \t= OK\r\n");
    } else
    {
        printf("\tBIT LCD2      \t= Fail(0x%x)\r\n",BIT->bitLCD2);
    }
    
    printf("\tESD BUS SW HIGH\t= %s \r\n",(BIT->ESDHigh)?"OK":"Fail");
    printf("\tESD BUS SW LOW \t= %s \r\n",(BIT->ESDLow)?"OK":"Fail");
    printf("\tVdd SA current\t= %d \r\n",BIT->vddsaI);
    printf("\tcop R1        \t= (0x%x,0x%x,0x%x) \r\n",BIT->copR1.r,BIT->copR1.g,BIT->copR1.b);
    printf("\tcop R2        \t= (0x%x,0x%x,0x%x) \r\n",BIT->copR2.r,BIT->copR2.g,BIT->copR2.b);
    printf("\tcop L1        \t= (0x%x,0x%x,0x%x) \r\n",BIT->copL1.r,BIT->copL1.g,BIT->copL1.b);
    printf("\tcop L2        \t= (0x%x,0x%x,0x%x) \r\n",BIT->copL2.r,BIT->copL2.g,BIT->copL2.b);    
    printf("\tFPGA Ver      \t= 0x%x \r\n",BIT->fpga);

}

