#ifndef _IDC_H_
#define _IDC_H_
#include <cstdio>

extern bool ignoreResponses;

#define DEBUG 1
#define INTERNAL_RS170 true // set this to false to default to driveing RS-170 from the BNC connectors connected to an external video generator
//#define RS232 1 // this sets the uart to us the RS232 (A7/D7 pins) instead of USB for the UI
//#define PORT_SPEED 19200 // for use with westar automation
#define PORT_SPEED 115200 // for standard Teraterm style usage
#define ECHO 1
//#define startWithTestPattern 
#define debug_print(fmt, ...) do { if (DEBUG) printf(fmt, ##__VA_ARGS__); } while (0)



#endif