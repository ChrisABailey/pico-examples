

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include <tusb.h>
#include "IDC_IO.h"
#include "i2c.h"
#include "led.h"
#include "bit.h"
#include "lcd.h"
#include "touch.h"

struct repeating_timer statusTimer;
int touch_channels=0;
touch_event_t last_touch[3];
bool gesture_filter = false;

const char promptString[] = "\r\nMAIN COM (?= help menu): ";
const char menu[] = "\r\n20x8 Touch Tester:\r\n"
                       "\tI - Initialize the DHA\r\n"
                       "\tR - Reset Touch (shows FW Ver)\r\n"
                        "\tF - Get Next Touch Response...\r\n"
                       "\tG -  Filter for Gestures (toggle)...\r\n"
                       "\tUx - Get Stuck Touch Location (x1-5)\r\n"
                       "\tC - Clear Sticky Touchs...\r\n";
                       

const char BACKSPACE = 127;


unsigned char mygetchar() {
	int c;
	while ( (c = getchar_timeout_us(0)) < 0); 
	return (unsigned char)c;
};

int getString(char *line, int bufLen, bool echo ) {
  int cnt = 0;
  int currentChar;

  while (cnt < bufLen - 1) {
    currentChar = mygetchar();
    //printf("Debug char number: 0x%x",currentChar);
    if (currentChar == '\n' || currentChar == '\r' ||currentChar == ';' )
      break;
    if (currentChar == BACKSPACE) {
      if (echo) {
        putchar('\b');
        putchar(' ');
        putchar('\b');
      }
      cnt--;
      line[cnt] = ' ';
    } else { 
       // printf("Debug char number: %d",currentChar);
      if (echo)
        putchar(currentChar);
      line[cnt++] = currentChar;
    }
  }
  line[cnt++] = '\0';
  //printf("getString returned(%s)",line);
  return cnt;
}

// read characters from the serial port and convert them to a float after 8
// characters or when the user hits enter. Echos the characters as the user
// types. Handles backspace
float getFloat(bool echo ) {
  char line[9];
  getString(line, 9, echo);
  return atof(line);
}

int getInt(bool echo ) {
  char line[9];
  getString(line, 9, echo);
  return strtol(line,NULL,0);
}

void prompt(bool echo)
{
    if (echo)
        printf(promptString);
}

// read info from the LCD uarts and update LCD_BIT structures
// bool lcd_status_timer_callback(struct repeating_timer *t) {
//     static int ch = CH1;

//     critical_section_enter_blocking (&critsec);
//     // this should take less than 1 frame and typically < 3.5mS
//     //read_LCD_status(ch);
//     // next time read the other channel
//     ch = (ch==CH1)?CH2:CH1;
//     critical_section_exit (&critsec);
//     gpio_put(LED_PIN, (ch==CH1));

//     return true;
// }

// read info from the touch i2c and update TOUCH_EVENT structures
bool touch_status_timer_callback(struct repeating_timer *t) {
    static int ch = CH1;
    static uint8_t touch_resp[TOUCH_RESP_LEN];

    //critical_section_enter_blocking (&critsec);
    // this should take less than 1 frame and typically < 3.5mS
    if(0==read_next_touch(touch_resp,ch,false))
    {
        if (!gesture_filter || is_gesture(touch_resp) )
        {
            decode_touch(touch_resp,&last_touch[ch]);
            gpio_put(LED_PIN, (ch==CH1));
        }
    }
    // next time read the other channel
    ch = (ch==CH1)?CH2:CH1;
    //critical_section_exit (&critsec);


    return true;
}

void initialize()
{
    i2c_init_all();
    printf("I2C Working \r\n");
    
    clear_touch_event(&last_touch[CH1]);
    clear_touch_event(&last_touch[CH2]);

    touch_channels = touch_init_all();
    if (touch_channels & CH1)
    {
        uint32_t rc = read_touch_fw_version(CH1);
        printf("Touch CH1 Firmware Ver: %x.%x.%x\r\n",(rc >> 16),(rc&0xFF00)>>8,(rc & 0x0FF));
    }
    if (touch_channels & CH2)
    {
        uint32_t rc = read_touch_fw_version(CH2);
        printf("Touch CH2 Firmware Ver: %x.%x.%x\r\n",(rc >> 16),(rc&0xFF00)>>8,(rc & 0x0FF));
    }
    if(touch_channels== 0)
    {
        printf("No Touch Controller Found\r\n");
    }

}





// Process user keystrokes
int handleMenuKey(char key,bool echo)
{
    if (echo) 
        putchar(key);

    switch (tolower(key)) /* Decode KEY ENTRY */
    {
        case 13:                // Enter ("CR"), do nothing
        {
            printf("\r\n");           
            break;
        }
        case '?':               // Display User interface help menu ("h")
        {
            printf(menu);
            break;
        }

        case 'f':
        {
            //start_touch_status_timer();
            printf("\r\nWaiting 5Sec for Touch...\r\n");

            uint32_t startTime;

            clear_touch_event(&last_touch[CH1]);
            last_touch[CH1].new_data = false;
            clear_touch_event(&last_touch[CH2]);
            last_touch[CH2].new_data = false;

            startTime = time_us_32();
            if (touch_channels & CH1) 
            {
                while (!last_touch[CH1].new_data) // wait for ISR to get new data
                {
                    if (time_us_32() > startTime+(5*1000000)) // give up after 5 sec
                        break;
                }
                print_touch_status(&last_touch[CH1],CH1);
            }
            
            startTime = time_us_32();
            if (touch_channels & CH2) 
            {
                while (!last_touch[CH2].new_data) // wait for ISR to get new data
                {
                    if (time_us_32() > startTime+(2*1000000)) // give up after 2 sec
                        break;
                }
                print_touch_status(&last_touch[CH2],CH2);
            }

            //start_lcd_status_timer();
            break;

        }

        case 'i':
        {
            i2c_scan(CH1);  
            i2c_scan(CH2);
            initialize();
            break;
        }

        //case 'm': {
        //    printf("Enable channels (01,02,03(both)) currently (%02d):", get_connected_channels());
        //    int newValue = getInt(echo);
        //    if (newValue >= 1 && newValue <= 3)
        //    {

        //        touch_channels = newValue;
        //        set_connected_channels(newValue);
                //printf("Set successful\r\n");
        //    }
        //    break;
        //}
        case 'r':
        {

            cancel_repeating_timer(&statusTimer);
            printf("Resetting Touch..\r\n");
            touch_reset();

            busy_wait_ms(10);
            clear_touch_event(&last_touch[CH1]);
            clear_touch_event(&last_touch[CH2]);

            printf("Read FW Versions\r\n");
            uint32_t rc;
            rc = read_touch_fw_version(CH1);
            printf("\r\nTouch CH1 Firmware Ver: %x.%x.%x\r\n",(rc >> 16),(rc&0xFF00)>>8,(rc & 0x0FF));
            rc = read_touch_fw_version(CH2);
            printf("Touch CH2 Firmware Ver: %x.%x.%x\r\n",(rc >> 16),(rc&0xFF00)>>8,(rc & 0x0FF));

            add_repeating_timer_ms(20, touch_status_timer_callback, NULL, &statusTimer);
            break;
        }

        case 'c':
        {

            cancel_repeating_timer(&statusTimer);

            printf("Clearing sticky Touches\r\n");
            clear_sticky_touch(CH1);
            clear_sticky_touch(CH2);
            add_repeating_timer_ms(20, touch_status_timer_callback, NULL, &statusTimer);
            break;
        }

        case 'u':
        {
            int Touch = getInt(echo);
            if (Touch >= 1 && Touch <= 5)
            {
                uint16_t ULX;
                uint16_t ULY;
                uint16_t BRX;
                uint16_t BRY;
                read_sticky_touch_location(CH1,Touch,&ULX,&ULY,&BRX,&BRY);
                printf("\r\nCH1: Top-Left (%d,%d) Bottom-Right (%d,%d)\r\n",ULX,ULY,BRX,BRY);
                read_sticky_touch_location(CH2,Touch,&ULX,&ULY,&BRX,&BRY);
                printf("\r\nCH2: Top-Left (%d,%d) Bottom-Right (%d,%d)\r\n",ULX,ULY,BRX,BRY);
            }
            break;
        }
        case 'g':
        {
            gesture_filter = !gesture_filter;
            printf("Gesture Filter %s \r\n",gesture_filter?"On":"Off");
            break;
        }        
        
        default:
        {
            prompt(echo);            
            return key;
        }
    }     // End switch case
    prompt(echo);
    fflush(stdout);
    return 0;
}


int main() {
    bool autoTest=false;
    bool echo = true;
    int ledOn = 0;

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(TOUCH_CH1_PIN    );
    gpio_init(TOUCH_CH2_PIN    );
    gpio_init(TOUCH_CH1_RESET  );
    gpio_init(TOUCH_CH2_RESET  );   

    gpio_set_dir(TOUCH_CH1_PIN       ,GPIO_IN);
    gpio_set_dir(TOUCH_CH2_PIN       ,GPIO_IN);
    gpio_set_dir(TOUCH_CH1_RESET, GPIO_OUT);
    gpio_set_dir(TOUCH_CH2_RESET, GPIO_OUT);

    //critical_section_init(&critsec);
    stdio_init_all();
    while (!tud_cdc_connected()) { 
        sleep_ms(150);  
        gpio_put(LED_PIN, ledOn);
        ledOn = !ledOn;
    }

    printf("USB Uart Connected()\r\n");


    initialize();

    // Create a repeating timer that reads the Touch Controller
    add_repeating_timer_ms(20, touch_status_timer_callback, NULL, &statusTimer);
    //gpio_set_irq_enabled_with_callback(2, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    prompt(echo);
    
    while(true)
    {
        int key = getchar_timeout_us(16);
        if ( key != PICO_ERROR_TIMEOUT)  // handle keystroke from USB
        {
            if (handleMenuKey(key,echo) == '7')  // The 7 key toggles autotest
            {
                autoTest = !autoTest;
                if (autoTest)
                {
                    printf("\r\nTouch auto-test mode, (press 7 to exit)\r\n");
                    if (gesture_filter)
                    {
                        printf("Note: Gesture Filter is active, (press G to toggle filter off)\r\n");
                    }
                    compare_touch_status(&last_touch[CH1],&last_touch[CH2]);
                }
                else
                {
                    prompt(echo);
                }
            }
            stdio_flush();
        }
        if (autoTest)
        {
            if (last_touch[CH1].new_data || last_touch[CH2].new_data)
            {
                erase_touch_status();
                compare_touch_status(&last_touch[CH1],&last_touch[CH2]);

            }
        }
        ledOn= !ledOn;

    }
    return 0;
}
