#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "IDC_IO.h"
#include "touch.h"
#include "i2c.h"
#include <string.h>

int i2c_tchannels = 0;
uint8_t touch_addr_ch1, touch_addr_ch2;

const char *gesture_to_string(uint8_t gesture)
{
    switch(gesture)
    {
        case 1:
            return("No Touch");
        case 0:
            return ("Multi-Touch");
        case 0x60:
            return ("Single Tap");
        case 0x61:
            return ("Double Tap");
        case 0x62:
            return ("Hold");
        case 0x63:
            return ("Swipe Up");
        case 0x64:
            return ("Swipe Down");
        case 0x65:
            return ("Swipe Left");
        case 0x66:
            return ("Swipe Right");
        case 0x70:
            return ("Zoom In");
        case 0x71:
            return ("Zoom Out");
        case 0x72:
            return ("Rotate Left");
        case 0x73:
            return ("Rotate Right");
        case 0x74:
            return("Scroll Up");
        case 0x75:
            return("Scroll Down");
        case 0x76:
            return("Scroll Left");
        case 0x77:
            return("Scroll Right");
        default:
            return("Unknown Gesture");
    }

}
void touch_reset()
{
    if (i2c_tchannels & CH1)
    {
        gpio_put(TOUCH_CH1_RESET,0);
        busy_wait_ms(10);
        gpio_put(TOUCH_CH1_RESET,1);
    }
    if (i2c_tchannels & CH2)
    {
        gpio_put(TOUCH_CH2_RESET,0);
        busy_wait_ms(10);
        gpio_put(TOUCH_CH2_RESET,1);
    }

    busy_wait_ms(100);  
}

uint8_t clear_sticky_touch(uint8_t ch)
{
    uint8_t data[3];
    uint8_t resp[4];
    if (ch & i2c_tchannels)
    {  
        uint8_t addr = get_touch_addr(ch);
        data[0] = 0xA0;
        data[1] = 0xC2;
        data[2] = 0x43;
        data[3] = 0x53;

        int count = i2c_reg_read_n(ch,addr,data,4,resp,4);  // FW resp is 5 bytes
        //int count = i2c_reg_write(ch,addr,reg,data,3);  // no response
        if (count!= 4)
        {
            printf("Write accepted %d bytes (expected %d)\r\n",count,4);
            return 0;
        }

        printf("resp = 0x%x,%x,%x,%x\r\n",resp[0],resp[1],resp[2],resp[3]);
        return count;
    }
    return 0;
}

int touch_init_all()
{

    i2c_tchannels = 0;
    gpio_put(TOUCH_CH1_RESET,1);
    gpio_put(TOUCH_CH2_RESET,1);
    busy_wait_ms(100);    

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

/// @brief return truo if the touch result is a gesture 
/// @param result 24 Byte touch result data
/// @return 1 if gesture 0 for multi-touch or tap message
int is_gesture(uint8_t *result)
{
    if (result[0] != 0x90)
    {
        printf("invalid response (header = 0x%x)\r\n",result[0]);
        return 0;
    }   

    if ((result[17]>=0x63) && (result[17]<=0x77))
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
    uint16_t i,j;
  
    if (result[1] & (1<<(touchNum-1)))
    {
        i = ((result[offset]&0xF0)<<4) + result[offset+1];
        j = ((result[offset]&0x0F)<<8) + result[offset+2];
        *x=(2560* i)/4096;
        *y=(1024* j)/4096;
        return true;
    }
    *x=0;
    *y=0;
    return false;
}

/// @brief print content od 24byte touch response (Depricated)
/// @param result buffer of data to print
void print_touch(uint8_t *result)
{
    if (result[0] != 0x90)
    {
        printf("Invalid touch response (header = 0x%x)\r\n",result[0]);
        return;
    }
    
    printf(gesture_to_string(result[17]));

    printf("Touches: \t%0b \r\n",result[1]);
    for (int i=1; i<=5;i++)
    {
        uint16_t x,y;
        if (get_touch_pixel(result,i,&x,&y))
        {
            printf("touch %d (x,y):\t (%d, %d) \r\n",i,x,y);
        }
    }
    
    if (result[20] != 0)
    {
        printf("%x Stuck Touch(s) reported\r\n",result[20]);
        //printf("Bytes [20:23]:0x%x %x %x %x\r\n",result[20],result[21],result[22],result[23]);
    }
}

void print_touch_status(volatile touch_event_t *touch,uint8_t ch)
{
    printf("┌────Touch┬Status─CH%d──┐\r\n",ch);
    printf("│ Touches │    %05b   │\r\n",touch->touches);
    printf("├─────────┼────────────┤\r\n"); 
    for (int i=0;i<5;i++)
    {
        printf("│ x= %4d │  y= %4d   │\r\n",touch->x[i],touch->y[i]);
    }
    printf("├─────────┼────────────┤\r\n");
    printf("│ Gesture │%12s│\r\n",gesture_to_string((int)(touch->gesture)));
    printf("├─────────┼────────────┤\r\n");
    printf("│ Sticky  │    %3d     │\r\n",touch->sticky_touch_count);
    printf("└─────────┴────────────┘\r\n");  
    touch->new_data = false;
}

/// @brief given 2 touch events, compare their locations and print status in RED if more than 4mm apart
/// @param touch1 left touch status
/// @param touch2 right touch status
void compare_touch_status(volatile touch_event_t *touch1,volatile touch_event_t *touch2)
{
    char* RED = "\033[31m";
    char* GREEN = "\033[32m";
    char* END = "\033[0m";
    char* color;
    printf("┌────Touch┬Status─CH%d──┐┌────Touch┬Status─CH%d──┐\r\n",1,2);
    if (touch1->touches!=touch2->touches)
    {
        color = RED;
    }
    else
    {
        color = GREEN;
    }
    printf("│ %sTouches │    %05b   ││ Touches │    %05b  %s │\r\n",color,touch1->touches,touch2->touches,END);
    printf("├─────────┼────────────┤├─────────┼────────────┤\r\n"); 
    for (int i=0;i<5;i++)
    {
        if (((touch1->x[i]-touch2->x[i])*(touch1->x[i]-touch2->x[i]) + (touch1->y[i]-touch2->y[i])*(touch1->y[i]-touch2->y[i])) > (20*20))
        {
        color = RED;
        }
        else
        {
            color = GREEN;
        }
        printf("│%s x= %4d │  y= %4d   ││ x= %4d │  y= %4d  %s │\r\n",color,touch1->x[i],touch1->y[i],touch2->x[i],touch2->y[i],END);
    }
    printf("├─────────┼────────────┤├─────────┼────────────┤\r\n");
    printf("│ Gesture │%12s││ Gesture │%12s│\r\n",gesture_to_string((int)(touch1->gesture)),gesture_to_string((int)(touch2->gesture)));
    printf("├─────────┼────────────┤├─────────┼────────────┤\r\n");
    printf("│ Sticky  │    %3d     ││ Sticky  │    %3d     │\r\n",touch1->sticky_touch_count,touch2->sticky_touch_count);
    printf("└─────────┴────────────┘└─────────┴────────────┘\r\n");  
    touch1->new_data = false; 
    touch2->new_data = false; 
}

void erase_touch_status()
{
    // back up 13 lines
    printf("\033[13A");
}

void clear_touch_event(volatile touch_event_t *event)
{
    event->gesture = no_touch;
    for (int i=0;i<5;i++)
    {
        event->x[i] = 0;
        event->y[i] = 0;
    }
    event->sticky_touch_count=0;
    event->touches = 0;
    event->crc = 0;
    event->gesture_d1 = 0;
    event->gesture_d2 = 0;
    event->new_data = 1;
}


/// @brief Parse 24bytes of touch response int a touch event struct
/// @param result Buffer w/ 24Bytes of data
/// @param event touch structure output
/// @return true for success, false for parsing error
bool decode_touch(volatile touch_event_t *event)
{
    uint16_t x,y;
    uint8_t result[TOUCH_RESP_LEN];
    strcpy(result, event->raw);

    if (result[0] != 0x90)
    {
        //printf("Invalid touch response (header = 0x%x)\r\n",result[0]);
        return false;
    }
    
    event->gesture = (gesture_enum)(result[17]);

    event->touches = result[1];
    for (uint8_t i=0; i<5;i++)
    {
        get_touch_pixel(result,i+1,&x,&y);
        event->x[i] = x;
        event->y[i] = y;
    }
    
    event->sticky_touch_count = result[20];
    event->gesture_d1 = result[18];
    event->gesture_d1 = result[19];
    event->crc = result[23];
    event->new_data = true;
    return true;

}

/// @brief Check Touch interupt, read data if availiable
/// @param result 0 for success, 1= timeout/data interupt not low
/// @param ch channel to read
/// @param wait true = wait up to 5 sec for data
/// @return 0 success, 1 no data
uint8_t read_next_touch(uint8_t *result,int ch, bool wait)
{
    uint int_pin = (ch==CH1)?TOUCH_CH1_PIN:TOUCH_CH2_PIN;
    
    uint32_t startTime;
    startTime = time_us_32();
    if (wait)
    {
        while (gpio_get(int_pin)) // interupt goes low when data available
        {
            if (time_us_32() > startTime+(5*1000000)) // give up after 5 sec
            {
                return 1;
            }
        }
    }

    if (0==gpio_get(int_pin))
    {
        return read_touch(result,ch);
    }
    else
        return 1;
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
        if (0==read_touch(result,ch))  // return 0 on success
        {
            uint8_t rc = is_gesture(result);
            if (rc)
            {
                return rc;
            }
        }
        if (time_us_32() > startTime+(5*1000000)) // give up after 5 sec
        {
            return 0;
        }

    }
}

uint8_t get_touch_addr(int ch)
{
    return (ch==CH1)?touch_addr_ch1:touch_addr_ch2;
}

/// @brief Read 24 Bytes of touch data over I2C 
/// @param result buffer to put data in
/// @param ch channel to read
/// @return 0 for success, 2 if data not available, 3 if channel is down
uint8_t read_touch(uint8_t *result,int ch)
{
    uint8_t reg[2] = {0};
    if (ch & i2c_tchannels)
    {
        
        int count = i2c_reg_read_n(ch,get_touch_addr(ch),reg,2,result,TOUCH_RESP_LEN);
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
    uint8_t reg[2];
    uint32_t result=0;
    uint8_t ver[5];
    if (ch & i2c_tchannels)
    {  
        uint8_t addr = get_touch_addr(ch);
        reg[0] = 0xA0;
        reg[1] = 0x20;
        int count = i2c_reg_read_n(ch,addr,reg,2,ver,5);  // FW resp is 5 bytes
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

// Get the bounding box for a reported sticky touch
// @param touch sticky touch index (1-5)
bool read_sticky_touch_location(int ch,uint8_t touch, uint16_t*TLX, uint16_t*TLY, uint16_t*BRX, uint16_t*BRY)
{
    uint8_t index[3];
    uint8_t resp[11] = {0};
    if (ch & i2c_tchannels)
    {  
        index[0] = 0xA0;
        index[1] = 0xC1;
        index[2] = touch;
        uint8_t addr = get_touch_addr(ch);
        int count = i2c_reg_read_n(ch,addr,index,3,resp,11);  // Sticky Touch Response is 11 bytes
        if (count!= 11)
        {
            printf("Read returned %d bytes (expected %d)\r\n",count,11);
            return false;
        }
        if ((resp[0] != 0xA0) || (resp[1] != 0xC1))
        {
            printf("Unexpected Version Response, Header = 0x%x%x, expected 0xA0C1%x\r\n",resp[0],resp[1],index[2]);
            return false;
        }

        *TLX = (2560* ((resp[3]<<8) | resp[4]))/4096;
        *TLY = (1024 * ((resp[5]<<8) | resp[6]))/4096;
        *BRX = (2560* ((resp[7]<<8) | resp[8]))/4096;
        *BRY = (1024* ((resp[9]<<8) | resp[10]))/4096;
        return true;
    }
    return false;
}