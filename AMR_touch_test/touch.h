#ifndef _IDC_TOUCH_H_
#define _IDC_TOUCH_H_
#include <stdlib.h>

#ifdef __cplusplus
 extern "C" {
#endif

// 
#define TOUCH_RESP_LEN 24

typedef enum { 
                no_touch = 1,
                multi_touch=0,
                single_tap=0x60,
                double_tap=0x61,
                hold = 0x62,
                swipe_up = 0x63,
                swipe_down = 0x64,
                swipe_left = 0x65,
                swipe_right = 0x66,
                zoom_in = 0x70,
                zoom_out = 0x71,
                rotate_left = 0x72,
                rotate_right = 0x73,
                scroll_up = 0x74,
                scroll_down = 0x75,
                scroll_left = 0x76,
                scroll_right = 0x77
                } gesture_enum ;
typedef struct
{   
    bool    new_data;
    uint8_t touches;
    uint16_t x[5];
    uint16_t y[5];
    gesture_enum gesture;
    uint8_t sticky_touch_count;
    uint16_t gesture_d1;
    uint16_t gesture_d2;
    uint8_t crc;
} touch_event_t;

int touch_init_all();
void touch_reset();
int is_gesture(uint8_t *result);
bool get_touch_pixel(uint8_t *result, uint8_t touchNum, uint16_t *x, uint16_t *y);
const char *gesture_to_string(uint8_t gesture);
void print_touch(uint8_t *result);
void print_touch_status(touch_event_t *touch);
void erase_touch_status();
void clear_touch_event(touch_event_t *event);
bool decode_touch(uint8_t *result, touch_event_t *event);
uint8_t read_next_touch(uint8_t *result,int ch,bool wait);
uint8_t read_next_gesture(uint8_t *result,int ch);
uint8_t read_touch(uint8_t *result,int ch);
uint32_t read_touch_fw_version(int ch);
bool read_sticky_touch_location(int ch,uint8_t touch, uint16_t*TLX, uint16_t*TLY, uint16_t*BRX, uint16_t*BRY);
uint8_t get_touch_addr(int ch);

#ifdef __cplusplus
 }
#endif

#endif 