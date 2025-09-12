#ifndef _serialui_h_
#define _serialui_h_

#include "pico/stdlib.h"

const char BACKSPACE = 127;
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')
  

const char prompt[] = "\r\nMAIN COM (?= help menu): ";
const char menuEED[] = "\r\n\r\nIDC T-38 EED Tester:\r\n"
                       "\tI - Run iBIT\r\n"
                       "\tS - Get DHA Status\r\n"
                       "\tBxxx (xxx=0 to 255) - Backlight Control Message\r\n"                       
                       "\tH - Heater Enable (note Heater will be turned off after "
                       "1 min if LCD Temp > 10 DegC)\r\n"
                       "\tD -  Heater Disable \r\n"
                       "\tA -  Heater Auto Mode (Control heater based on LCD temperature)\r\n"
                       "Set Dicrete Signals:\r\n"
                       "\tC - Set Calibration Mode (Backlight not changed by Bezel)\r\n"
                       "\tN - Set Normal Mode (Bezel Up/Down changes Backlight)\r\n"                       
                       "Read Discrete Signals:\r\n"
                       "\tE - Get BIT_Summary (0=Error, 1=Normal)\r\n"
                       "\tP - Get BIT_In_Progress (1= In Progress)\r\n"
                       "Miscellaneous: \r\n"
                       "\tT = Get DUT Type (Bezel Personality Pins)\r\n"
                       "\tV - Raise / Lower Bezel Voltage (luminance)\r\n"
                       "\tY-Get LCD Temperature (deg C)\r\n"
                       "\t+/- Adjust Backlight\r\n"
                       "\t5 - DHA Control Board Configuration\r\n"
                       "\t6 - Test Board Configuration\r\n"
                       "\t7 - Auto-test mode (Continous Status Monitor)\r\n";

const char menuMFD[] = "\r\n\r\nIDC T-38 MFD Tester:\r\n"
                       "\tI - Run iBIT\r\n"
                       "\tS - Get DHA Status\r\n"
                       "\tBxxx (xxx=0 to 255) - Backlight Control Message\r\n"                       
                       "\tH - Heater Enable (note Heater will be turned off after 1 min if LCD Temp > 10 DegC)\r\n"
                       "\tD -  Heater Disable \r\n"
                       "\tA -  Heater Auto Mode (Control heater based on LCD temperature)\r\n"
                       "Set Dicrete Signals:\r\n"
                       "\tL - Set LVDS Mode\r\n"
                       "\tR - Set RS-170 Mode \r\n"                       
                       "Read Discrete Signals:\r\n"
                       "\tE - Get BIT_Summary (0=Error, 1=Normal)\r\n"
                       "\tP - Get BIT_In_Progress (1= In Progress)\r\n"
                       "Miscellaneous: \r\n"
                       "\tT = Get DUT Type (Bezel Personality Pins)\r\n"
                       "\tV - Raise / Lower Bezel Voltage (luminance)\r\n"
                       "\tY - Get LCD Temperature (deg C)\r\n"
                       "\t+/- Adjust Backlight\r\n"
                       "\t$ - Substatus command (Machine to Machine interface)\r\n"
                       "\t5 - DHA Control Board Configuration\r\n"
                       "\t6 - Test Board Configuration\r\n"
                       "\t7 - Auto-test mode (Continous Status Monitor)\r\n";

const char subMenu[] = "\r\nm - video mode (r/l)\r\n"
                        "d - day/night (d/n)\r\n"
                        "b -  backlight step (0-255)\r\n"
                        "v -  bezel voltage (0-5500mV)\r\n"
                        "s -  software version (xx.xx)\r\n"
                        "f -  Firmware version (xx.xx)\r\n" 
                        "t -  MCU Temperature (degC)\r\n" 
                        "0-4 - BIT byte 0-4 (0-255)\r\n"
                        "v -  bezel voltage (0-5500mV)\r\n"; 

const char delimiters[] = {'\n', '\r', ';', 0};

int getString(char *line, int bufLen, bool echo = false);
float getFloat(bool echo = false);
int getInt(bool echo = false);
#endif