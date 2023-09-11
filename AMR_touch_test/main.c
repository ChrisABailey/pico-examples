

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
                       "\tM - Enable/disable Driver Channels...\r\n";
                       

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
            decode_touch(touch_resp,&last_touch[ch]);
    }
    // next time read the other channel
    ch = (ch==CH1)?CH2:CH1;
    //critical_section_exit (&critsec);
    gpio_put(LED_PIN, (ch==CH1));

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



void printStatus()
{   
    printf("┌─────Board Status─┬──────┐\r\n");
    printf("│PWM percentage    │%5.2f%%│\r\n",get_pwm()*100);
    printf("│BIT_CH1 Discrete  │ %s │\r\n",gpio_get(BIT_CH1_PIN)?" OK ":"FAIL");
    printf("│BIT_CH2 Discret   │ %s │\r\n",gpio_get(BIT_CH2_PIN)?" OK ":"FAIL");
    printf("│TEMP_OK_CH1       │ %s │\r\n",gpio_get(TEMP_OK_CH1_PIN)?" OK ":"FAIL");
    printf("│TEMP_OK_CH2       │ %s │\r\n",gpio_get(TEMP_OK_CH2_PIN)?" OK ":"FAIL");
    printf("│LCD_ALARM_CH1     │ %s │\r\n",gpio_get(LCD_ALARM_CH1_PIN)?" OK ":"FAIL");
    printf("│LCD_ALARM_CH2     │ %s │\r\n",gpio_get(LCD_ALARM_CH2_PIN)?" OK ":"FAIL");
    printf("│CH1 Display Enable│ %s │\r\n",gpio_get(PD_CH1_PIN)?" ON ":"OFF ");
    printf("│CH2 Display Enable│ %s │\r\n",gpio_get(PD_CH2_PIN)?" ON ":"OFF ");
    printf("│                  │      │\r\n");
    printf("│CH1 BIT Register: │ ");
    print_bit(read_bit(CH1));
    printf(" │\r\n");
    printf("│CH2 BIT Register: │ ");
    print_bit(read_bit(CH2));
    printf(" │\r\n");
    printf("│CH1 LED IC Status:│ ");
    uint32_t resultCH1 = read_led_bit(CH1);
    print_led_bit(resultCH1);
    printf(" │\r\n");
    printf("│CH2 LED IC Status:│ ");
    uint32_t resultCH2 = read_led_bit(CH2);
    print_led_bit(resultCH2);
    printf(" │\r\n");
    printf("└──────────────────┴──────┘\r\n");
    isolate_led_bit(resultCH1,resultCH2);
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
                print_touch_status(&last_touch[CH1]);
            }
            
            startTime = time_us_32();
            if (touch_channels & CH2) 
            {
                while (!last_touch[CH2].new_data) // wait for ISR to get new data
                {
                    if (time_us_32() > startTime+(2*1000000)) // give up after 2 sec
                        break;
                }
                print_touch_status(&last_touch[CH2]);
            }
            break;
        }

        case 'i':
        {
            i2c_scan(CH1);  
            i2c_scan(CH2);
            initialize();
            break;
        }

        case 'm': {
            printf("Enable channels (01,02,03(both)) currently (%02d):", get_connected_channels());
            int newValue = getInt(echo);
            if (newValue >= 1 && newValue <= 3)
            {

                touch_channels = newValue;
                set_connected_channels(newValue);
                //printf("Set successful\r\n");
            }
            break;
        }
        case 'r':
        {
            //printf("Resetting Touch..");
            touch_reset();
            clear_touch_event(&last_touch[CH1]);
            clear_touch_event(&last_touch[CH2]);

            uint32_t rc;
            rc = read_touch_fw_version(CH1);
            printf("\r\nTouch CH1 Firmware Ver: %x.%x.%x\r\n",(rc >> 16),(rc&0xFF00)>>8,(rc & 0x0FF));
            rc = read_touch_fw_version(CH2);
            printf("Touch CH2 Firmware Ver: %x.%x.%x\r\n",(rc >> 16),(rc&0xFF00)>>8,(rc & 0x0FF));

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
    struct repeating_timer statusTimer;

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    //gpio_init(BIT_CH1_PIN      );
    //gpio_init(BIT_CH2_PIN      );
    //gpio_init(TEMP_OK_CH1_PIN  );
    //gpio_init(TEMP_OK_CH2_PIN  );
    //gpio_init(LCD_ALARM_CH1_PIN);
    //gpio_init(LCD_ALARM_CH2_PIN);
    //gpio_init(PD_CH1_PIN       );
    //gpio_init(PD_CH2_PIN       );
    gpio_init(TOUCH_CH1_PIN    );
    gpio_init(TOUCH_CH2_PIN    );
    gpio_init(TOUCH_CH1_RESET  );
    gpio_init(TOUCH_CH2_RESET  );   

    // gpio_set_dir(BIT_CH1_PIN      ,GPIO_IN);
    // gpio_set_dir(BIT_CH2_PIN      ,GPIO_IN);
    // gpio_set_dir(TEMP_OK_CH1_PIN  ,GPIO_IN);
    // gpio_set_dir(TEMP_OK_CH2_PIN  ,GPIO_IN);
    // gpio_set_dir(LCD_ALARM_CH1_PIN,GPIO_IN);
    // gpio_set_dir(LCD_ALARM_CH2_PIN,GPIO_IN);
    // gpio_set_dir(PD_CH1_PIN       ,GPIO_IN);
    // gpio_set_dir(PD_CH2_PIN       ,GPIO_IN);
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

    // Create a repeating timer that reads the LCD status uarts.
    //add_repeating_timer_ms(14, lcd_status_timer_callback, NULL, &statusTimer);
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

                printf("auto-test = %s\r\n",autoTest?"On":"Off");
                print_touch_status(&last_touch[CH1]);
                clear_touch_event(&last_touch[CH1]);
                clear_touch_event(&last_touch[CH2]);
            }
            stdio_flush();
        }
        if (autoTest)
        {
            if (last_touch[CH1].new_data)
            {
                erase_touch_status();
                print_touch_status(&last_touch[CH1]); 
            }
        }
        ledOn= !ledOn;

    }
    return 0;
}
