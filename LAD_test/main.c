

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "pico/sync.h"
#include <tusb.h>
#include "IDC_IO.h"
#include "i2c.h"
#include "led.h"
#include "bit.h"
#include "lcd.h"
#include "touch.h"

//a random comment
critical_section_t critsec;

const char promptString[] = "\r\nMAIN COM (?= help menu): ";
const char menu[] = "\r\n20x8 DHA Tester:\r\n"
                       "\tI - Initialize the DHA\r\n"
                       "\tR - Reset Touch (shows FW Ver)\r\n"
                       "\tB - Read DHA Controller Board BIT\r\n"
                       "\tL - Read LCD Status\r\n"
                       "\tS - Read DHA Controller Board Status \r\n"
                       "\tF - Get Next Touch Response...\r\n"
                       "\tW - Get Next Touch Swipe Response...\r\n"
                       "\tpxxxx (xxxx=0 to 3000) - Backlight PWM\r\n"                       
                       "\tT - Read Backlight Temperature\r\n"
                       "\tC - Read LED Default Current\r\n"
                       "\tD - Day Mode \r\n"
                       "\tN - Night Mode\r\n"             
                       "\tM - Enable/disable Driver Channels\r\n"          
                       "\t+/- Raise/Lower Backlight by 10%%\r\n";
                       

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
bool lcd_status_timer_callback(struct repeating_timer *t) {
    static int ch = CH1;

    critical_section_enter_blocking (&critsec);
    // this should take less than 1 frame and typically < 3.5mS
    read_LCD_status(ch);
    // next time read the other channel
    ch = (ch==CH1)?CH2:CH1;
    critical_section_exit (&critsec);
    gpio_put(LED_PIN, (ch==CH1));

    return true;
}

void initialize()
{
    i2c_init_all();
    printf("I2C Working \r\n");
    int chs = led_init_all();
    if (chs == CH1)
    {
        printf("LED CH1 Responding\r\n");
    }
    else if (chs == CH2)
    {
        printf("LED CH2 Responding\r\n");
    }
    else if (chs == (CH1|CH2))
    {
        printf("LED CH1 & CH2 Responding\r\n");
    }
    else
    {
        printf("Error, No LED controllers responded\r\n");
    }

    lcd_init_all();

    chs = bit_init_all();

    printf("BIT Channel 1 %s, BIT Channel 2 %s \r\n",(chs & CH1)? "Responding":"Not Found",(chs & CH2)? "Responding":"Not Found");

    chs = touch_init_all();
    if (chs & CH1)
    {
        uint32_t rc = read_touch_fw_version(CH1);
        printf("Touch CH1 Firmware Ver: %x.%x.%x\r\n",(rc >> 16),(rc&0xFF00)>>8,(rc & 0x0FF));
    }
    if (chs & CH2)
    {
        uint32_t rc = read_touch_fw_version(CH2);
        printf("Touch CH2 Firmware Ver: %x.%x.%x\r\n",(rc >> 16),(rc&0xFF00)>>8,(rc & 0x0FF));
    }
    if(chs== 0)
    {
        printf("No Touch Controller Found\r\n");
    }
    day(0.33,CH1);
    day(0.33,CH2);

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
    printf("│CH1 LED IC Status:│ ");
    print_led_bit(read_led_bit(CH1));
    printf(" │\r\n");
    printf("│CH2 BIT Register: │ ");
    print_bit(read_bit(CH2));
    printf(" │\r\n");
    printf("│CH2 LED IC Status:│ ");
    print_led_bit(read_led_bit(CH2));
    printf(" │\r\n");
    printf("└──────────────────┴──────┘\r\n");
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
        case 'b':
        {
            uint8_t result1 = read_bit(CH1);
            uint8_t result2 = read_bit(CH2);
            if (echo)
            {
                printf("\r\nChannel 1 Controler Board BIT :");
                print_bit(read_bit(CH1));
                printf("\r\nChannel 2 Controler Board BIT :");
                print_bit(read_bit(CH2));
            }
            else
            {
                printf("0x%x;",result1);
                printf("0x%x;",result2);
            }
            break;
        }
        case 'c':
        {
            printf("\r\nLED Driver Current (0-4095) = %d \n",read_current(CH2));
            break;
        }        
        case 'd':
        {
            day(0.3,CH1);            
            day(0.3,CH2);
            break;
        }
        case 'f':
        {
            uint8_t touchResult[TOUCH_RESP_LEN];
            uint8_t rc;

            printf("\r\nWaiting 5Sec for Touch...\r\n");

            printf(" CH1:\r\n");
            if ( (rc = read_next_touch(touchResult,CH1))==0)
            {
                print_touch(touchResult);
            }
            else if (rc==1)
            {
                printf("No Touch Signal Received (Timed Out)\r\n");
            }
            else
            {
                printf("Unrecognised Touch Response\r\n");
            }

            printf(" CH2:\r\n");
            if ( (rc = read_next_touch(touchResult,CH2))==0)
            {
                print_touch(touchResult);
            }
            else if (rc==1)
            {
                printf("No Touch Signal Received (Timed Out)\r\n");
            }
            else
            {
                printf("Unrecognised Touch Response\r\n");
            }
            break;
        }
        case 'n':
        {
            night(0.3,CH1);
            night(0.3,CH2);
            break;
        }
        case 'i':
        {

            i2c_scan(CH1);  
            i2c_scan(CH2);
            initialize();

            break;
        }
        case 'l':
        {
            printf("\r\n");
            printf("Channel 1 LCD Status:\r\n");
            print_status(&last_LCD_BIT_CH1);
            printf("Channel 2 LCD Status:\r\n");
            print_status(&last_LCD_BIT_CH2);
            break;
        }
        case 'p': {
            float newValue = getFloat(echo);
            if (newValue >= 0.0f && newValue <= 3000.0f)
            {
                printf("\r\nPWM = %2.2f \r\n",set_pwm(newValue/3000.0f)*100);
                //printf("Set successful\r\n");
            }
            break;
        }
        case 'm': {
            printf("Enable channels (01,02,03(both)) currently (%02d):", get_connected_channels());
            int newValue = getInt(echo);
            if (newValue >= 1 && newValue <= 3)
            {

                set_connected_channels(newValue);
                //printf("Set successful\r\n");
            }
            break;
        }
        case 'r':
        {
            //printf("Resetting Touch..");
            touch_reset();

            uint32_t rc;
            rc = read_touch_fw_version(CH1);
            printf("\r\nTouch CH1 Firmware Ver: %x.%x.%x\r\n",(rc >> 16),(rc&0xFF00)>>8,(rc & 0x0FF));
            rc = read_touch_fw_version(CH2);
            printf("Touch CH2 Firmware Ver: %x.%x.%x\r\n",(rc >> 16),(rc&0xFF00)>>8,(rc & 0x0FF));

            break;
        }
        case 's':
        {
            printf("\r\n");
            printStatus();
            break;
        }
        case 't':
        {
            if (echo)
            {
                printf("\r\nCH1 LED Rail%d Temperature = %3.1f DegC\n",RAIL1,read_temperature(RAIL1,CH1));
                printf("CH1 LED Rail%d Temperature = %3.1f DegC\n",RAIL2,read_temperature(RAIL2,CH1));
                printf("CH2 LED Rail%d Temperature = %3.1f DegC\n",RAIL1,read_temperature(RAIL1,CH2));
                printf("CH2 LED Rail%d Temperature = %3.1f DegC\n",RAIL2,read_temperature(RAIL2,CH2));
            }
            else
            {
                printf("%3.1f,%3.1f,%3.1f,%3.1f;",read_temperature(RAIL1,CH1),
                                                read_temperature(RAIL2,CH1),
                                                read_temperature(RAIL1,CH2),
                                                read_temperature(RAIL2,CH2));
            }
            break;
        }
        case 'w':
        {
            uint8_t touchResult[TOUCH_RESP_LEN];
            //const char *dir[] = {"No Swipe","Up","Down","Left","Right"};

            printf("\r\nWaiting 5Sec for Swipe...\r\n");

            
            uint8_t rc = read_next_gesture(touchResult,CH1);
            if (rc==0) 
                printf("CH1: No Gesture seen\r\n");
            else
            {
                printf(" CH1 Swipe:\r\n");
                print_touch(touchResult);
            }
            rc = read_next_gesture(touchResult,CH2);
            if (rc==0) 
                printf("CH2: No Gesture seen\r\n");
            else
            {
                printf(" CH2 Swipe:\r\n");
                print_touch(touchResult);
            }
            break;
        }        
        case '+': // Increase PWM by 10%
        {
            float curr = get_pwm();
            curr = curr*1.1;
            printf("PWM = %2.2f \r\n",set_pwm(curr)*100);
            break;
        }
        case '-':               
        {
            float curr = get_pwm();
            curr = curr * 0.9;
            printf("PWM = %2.2f \r\n",set_pwm(curr)*100);
            
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

    gpio_init(BIT_CH1_PIN      );
    gpio_init(BIT_CH2_PIN      );
    gpio_init(TEMP_OK_CH1_PIN  );
    gpio_init(TEMP_OK_CH2_PIN  );
    gpio_init(LCD_ALARM_CH1_PIN);
    gpio_init(LCD_ALARM_CH2_PIN);
    gpio_init(PD_CH1_PIN       );
    gpio_init(PD_CH2_PIN       );
    gpio_init(TOUCH_CH1_PIN    );
    gpio_init(TOUCH_CH2_PIN    );
    gpio_init(TOUCH_CH1_RESET  );
    gpio_init(TOUCH_CH2_RESET  );   

    gpio_set_dir(BIT_CH1_PIN      ,GPIO_IN);
    gpio_set_dir(BIT_CH2_PIN      ,GPIO_IN);
    gpio_set_dir(TEMP_OK_CH1_PIN  ,GPIO_IN);
    gpio_set_dir(TEMP_OK_CH2_PIN  ,GPIO_IN);
    gpio_set_dir(LCD_ALARM_CH1_PIN,GPIO_IN);
    gpio_set_dir(LCD_ALARM_CH2_PIN,GPIO_IN);
    gpio_set_dir(PD_CH1_PIN       ,GPIO_IN);
    gpio_set_dir(PD_CH2_PIN       ,GPIO_IN);
    gpio_set_dir(TOUCH_CH1_PIN       ,GPIO_IN);
    gpio_set_dir(TOUCH_CH2_PIN       ,GPIO_IN);
    gpio_set_dir(TOUCH_CH1_RESET, GPIO_OUT);
    gpio_set_dir(TOUCH_CH2_RESET, GPIO_OUT);

    critical_section_init(&critsec);
    stdio_init_all();
    while (!tud_cdc_connected()) { 
        sleep_ms(150);  
        gpio_put(LED_PIN, ledOn);
        ledOn = !ledOn;
    }

    printf("USB Uart Connected()\n");


    initialize();

    // Create a repeating timer that reads the LCD status uarts.
    add_repeating_timer_ms(14, lcd_status_timer_callback, NULL, &statusTimer);
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

                printf("\n");
                //printStatus();
            }
            stdio_flush();
        }
        //gpio_put(LED_PIN, ledOn);
        ledOn= !ledOn;

    }
    return 0;
}
