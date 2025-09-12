#include <stdint.h>

#ifndef DATA_INTERFACE_H_
#define DATA_INTERFACE_H_


#define DEGC_TO_K           273.15

#define INVALID_CRC 		1
#define BAD_CMD_ID			2
#define INVALID_MSG_SIZE    3
#define DATA_OUT_OF_RANGE	4

// Location in Output BIT Discrete Table
#define	BIT_SUMMARY						0x02
#define	BIT_IN_PROGRESS					0x01

// Location in Input BIT Discrete Table
#define	CALIBRATION_MODE				0x01
#define	VIDEO_MODE						0x02
#define	PROGRAMMING						0x04
#define SPARE							0x08

#define CONFIG_DATA_SIZE                2
#define VIDEO_WRITE   	                0xA5
#define VIDEO_READ		                0xB5
#define SERIAL_TIMEOUT_CMD              0xC5
#define FPGA_TEST_CMD                   0x55
#define MIN_CMD_SIZE 					4
#define	BACKLIGHT_CONTROL				0x81
#define BACKLIGHT_CONTROL_SIZE			4
#define	HEATER_CONTROL					0x82
#define	HEATER_CONTROL_SIZE				4
#define	CONFIG_CMD						0x83
#define	CONFIG_CMD_SIZE					6
#define	STATUS							0x84
#define	STATUS_SIZE						4
#define	PERFORM_IBIT					0x85
#define	PERFORM_IBIT_SIZE				4

#define	NACK							0x0
#define	NACK_SIZE						4
#define	BACKLIGHT_CONTROL_RESPONSE		0x1
#define BACKLIGHT_CONTROL_RESPONSE_SIZE	4
#define	HEATER_CONTROL_RESPONSE			0x2
#define	HEATER_CONTROL_RESPONSE_SIZE	4
#define	CONFIG_RESPONSE					0x3
#define	CONFIG_RESPONSE_SIZE			6
#define	STATUS_RESPONSE					0x4
#define	STATUS_RESPONSE_SIZE			32

#define BIT_5V                          0x20
#define BIT_18V                         0x20
#define BIT_STUCK_KEY                   0x20
#define BIT_MEMORY_CRC                  0x10
#define BIT_MEMORY                      0x08
#define BIT_TEMP_SENSOR                 0x04
#define BIT_BACKLIGHT                   0x02
#define BIT_33V                         0x01
#define BIT_LVDS_VALID                  0x02
#define RS170_VALID                     0x01
#define PBIT_MEMORY                     0x02
#define PBIT_WATCH_DOG                  0x01
#define PBIT_5V                         0x80
#define PBIT_18V                        0x40
#define PBIT_HEATER1                    0x20
#define PBIT_HEATER2                    0x10
#define PBIT_HEATER3                    0x08
#define PBIT_TEMP_SENSOR                0x04
#define PBIT_BACKLIGHT                  0x02
#define PBIT_33V                        0x01
#define BIT_HEATER1                     0x20
#define BIT_HEATER2                     0x10
#define BIT_HEATER3                     0x08
#define BIT_BEZEL                       0x04
#define BIT_FPGA                        0x02
#define BIT_DECODER                     0x01


#pragma pack(push, 1)

typedef struct {
	uint8_t cmdId;
	uint8_t backlightStep;
	uint8_t crcM;
    uint8_t crcL;
} backlightControlStruct;

typedef struct {
	uint8_t respId;
	uint8_t currentBacklightVal;
	uint8_t crcM;
    uint8_t crcL;
} backlightResponseStruct;

typedef struct {
	uint8_t cmdId;
	uint8_t heaterEnabled;
	uint8_t crcM;
    uint8_t crcL;
} heaterControlStruct;

typedef struct {
	uint8_t respId;
	uint8_t currentHeaterEnabled;
	uint8_t crcM;
    uint8_t crcL;
} heaterResponseStruct;

typedef struct {
	uint8_t cmdId;
	uint8_t cmd;
	uint8_t data[CONFIG_DATA_SIZE];
	uint8_t crcM;
	uint8_t crcL;
} configControlStruct;

typedef struct {
	uint8_t respId;
	uint8_t currentCmd;
	uint8_t data[CONFIG_DATA_SIZE];
	uint8_t crcM;
	uint8_t crcL;
} configResponseStruct;

typedef struct {
	uint8_t cmdId;
	uint8_t seqNumber;
	uint8_t crcM;
    uint8_t crcL;
} statusCommandStruct;


typedef struct {
	uint8_t respId;
	uint8_t echoSeqNumber;
	uint8_t inputDiscreteSettings;
	uint8_t currentHeaterEnabled;
	uint8_t dayNight;
	uint8_t reserved;
	uint8_t outputDiscreteSettings;
	uint8_t bezelButtonsUpper;
	uint8_t bezelButtonsLower;
	uint8_t bezelButtonsLeft;
	uint8_t bezelButtonsRight;
	uint8_t rockerSwitches;
	uint8_t backlightValue;
	uint8_t backlightHighByte;
	uint8_t bezelBrightnessVoltageLsb;
	uint8_t bezelBrightnessVoltageMsb;
	uint8_t lcdTemperatureLsb;
	uint8_t lcdTemperatureMsb;
	uint8_t bitResultsByte0;
	uint8_t bitResultsByte1;
	uint8_t bitResultsByte2;
	uint8_t bitResultsByte3;
	uint8_t bitResultsByte4;
	uint8_t bitResultsByte5;
	uint8_t mcuTemperatureLsb;
	uint8_t mcuTemperatureMsb;
	uint8_t swVersionLsb;
	uint8_t swVersionMsb;
	uint8_t fwVersionLsb;
	uint8_t fwVersionMsb;
	uint8_t crcM;
    uint8_t crcL;
} statusResponseStruct;

typedef struct {
	uint8_t cmdId;
	uint8_t ibitCmd;
	uint8_t crcM;
    uint8_t crcL;
} ibitCommandStruct;

typedef struct {
	uint8_t respId;
	uint8_t reasonCode;
	uint8_t crcM;
    uint8_t crcL;
} nackResponseStruct;

#pragma pack(pop)
#endif /* DATA_INTERFACE_H_ */
