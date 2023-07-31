#ifndef _IDC_TOUCH_H_
#define _IDC_TOUCH_H_
#include <stdlib.h>

#ifdef __cplusplus
 extern "C" {
#endif

// 
#define TOUCH_RESP_LEN 24

int touch_init_all();
void touch_reset();
bool get_touch_pixel(uint8_t *result, uint8_t touchNum, uint16_t *x, uint16_t *y);
void print_touch(uint8_t *result);
uint8_t read_next_touch(uint8_t *result,int ch);
uint8_t read_next_gesture(uint8_t *result,int ch);
uint8_t read_touch(uint8_t *result,int ch);
uint32_t read_touch_fw_version(int ch);
uint8_t get_touch_addr(int ch);

#ifdef __cplusplus
 }
#endif

#endif 