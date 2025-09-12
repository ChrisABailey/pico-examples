/*
 * 3ATIBoardIO.h
 *
 *  Created on: Mar 24, 2018
 *      Author: chrisbailey
 */

#ifndef INC_T38BOARDIO_H_
#define INC_T38BOARDIO_H_

#define DAY_MODE    0
#define NIGHT_MODE  1


//DUT Interface		Nucleo pin




//Top Left Pin on MBED Nucleo f303k8
//Bottom Right pin on Maker Nano RP2040
#define UART_TX		    0 //D1 
#define UART_RX		    1 //D18 

// NRST
// GND
#define UART_CTS	    2 //D2
//D3 NC
#define ADV7393_SDA		4 //D4
#define ADV7393_SCL		5 //D5
#define ADV7393_BLOCK   i2c0
#define ETHIO_1		    6 //D6	//GND = MFD Float=EED (pull_up)
#define MODE_SEL		7 //D7
#define BIT_PROGRESS	8 //D8
#define AC_PWM			9 //D9
#define UART_RTS	    17 //D10
#define DHA_OFF 		19 //D11	//GND = OFF Pull_up
#define RS232_RX        16 //D12
//Bottom Left Pin
//TOP Right of Maker Nano

//Bottom Right Pin
//TOP Left of Maker Nano
#define LED1           18
// 3.3V
// AREF                 NC
#define ETHIO_2		    26 //A0	//GND = EED Float=MFD (pull_up)
#define ETHIO_3		    27 //A1 //D0	//unused
#define BEZEL_LUMINANCE	28 //A2 
#define BIT_STATUS		29 //A3
#define MAIN_IO3		12 //A4	//unused
#define MAIN_IO2		13 //A5	//unused
#define LVDS_RS170		14 //A6
#define RS232_TX        15 //A7
//5V
//NRST
//GND
//VIN

#define SPEAKER          22
#define BUTTON          20





//extern I2C i2c;

//extern InterruptIn bit;  // treat bit error as interupt so we ditect it immediatly
//extern DigitalIn bitInProgress;
//extern DigitalIn configPin1;
//extern DigitalIn configPin2;
//extern DigitalIn dhaOff;

//extern DigitalOut bezelLuminance;
//extern DigitalOut eedConfigMode;
//extern DigitalOut mfdLvdsRs170;
//extern PwmOut acPwm;

#endif /* INC_3ATIBOARDIO_H_ */
