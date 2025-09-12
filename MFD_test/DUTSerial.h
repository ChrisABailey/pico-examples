/*
 * serial.h
 *
 *  Created on: May 6, 2022
 *      Author: CAB
 */

#ifndef DUTSERIAL_H_
#define DUTSERIAL_H_
#include "T38BoardIO.h"
#include "data_interface.h"
#include <cstdint>
#include <stdbool.h>

#define DEGCTOK(_DEGC_) ((_DEGC_)+273.15)
#define DEGKTOC(_DEGK_) ((_DEGK_)-273.15)

class DUTSerial {
public:
  enum enumErrorTypes {
    noStatus,
    noError,
    badEcho,
    badCRC,
    nackReceived,
    unexpectedResponse,
    unknownResp,
    invalidSize,
    respTimeout
  };

  
   // Last status command 
  statusResponseStruct currentStatus;

   // set if command fails
  enumErrorTypes errorStatus = noStatus;


  DUTSerial(uint tx, uint rx, uint rts, uint cts);


  void flush();

// return LCD thermistor temperature in degK
  int getLCDTemp() 
  {
      if (errorStatus == noError)
      {
        return currentStatus.lcdTemperatureLsb +
            currentStatus.lcdTemperatureMsb * 256;
      }
      else
      {
          return -55+273;
      }
  };

// return MCU temperatur in degK
  int getMCUTemp() 
  {
      if (errorStatus == noError)
      {
        return currentStatus.mcuTemperatureLsb +
            currentStatus.mcuTemperatureMsb * 256;
      }
      else
      {
          return -55+273;
      }
  };

  // returns Bezel voltage in mV
  int getBezelVoltage() 
  {
    if (errorStatus == noError)
    {
        return currentStatus.bezelBrightnessVoltageLsb +
           currentStatus.bezelBrightnessVoltageMsb * 256;
    }
    return 0;
  };

  int getBacklightStep() 
  {

      return backlight;

  }

  void internalTestPattern(uint8_t testPatternOn) {
    uint8_t cmd[CONFIG_DATA_SIZE];
    cmd[0] = testPatternOn;
    cmd[1] = 0;
    sendConfig(FPGA_TEST_CMD, cmd);
  }

  void setTimeout(uint16_t timeout) {
    uint8_t cmd[CONFIG_DATA_SIZE];
    cmd[0] = timeout&0x00FF;
    cmd[1] = timeout>>8;
    sendConfig(SERIAL_TIMEOUT_CMD, cmd);
  }

  uint8_t *getCurrentBITResults() {
      return &currentStatus.bitResultsByte0;
  }
  
  //pretty print BIT failures in the given byte of status BIT table into a single line
  // byte_ specifiex the desired byte of the status table (0 to 4) 
  void printBIT(uint8_t* BITResults, unsigned byte_);
 
  // print Error Status
  void printComStatus();


  enumErrorTypes updateStatus();
  enumErrorTypes setBacklight(uint8_t step);

  // Send in a config commang with 2 bytes of data.  
  // Response updates the 2 bytes of data
  enumErrorTypes sendConfig(uint8_t cmd, uint8_t data[CONFIG_DATA_SIZE]);

  // SEND AN ILLEGAL CMD
  enumErrorTypes badCmdTest();

  //Semd an IBIT cmd
  enumErrorTypes runIBIT();

  // note: the following "setHeater()" command does not actually send a setHeater cmd 
  // unless immediate is set to true
  // it simply queues up a heater command and returns, sthis allows the heater controls to
  // be set in an ISR
  // The actual sending of the message and reading of the response is done
  // in the processHeater() which needs to be called in the main thread
  // also note any command such as configureBoard that can block should turn off the heater before running 
  void setHeater(bool on, bool immediate=false) 
  { 
      queuedHeater = on?1:0;
      if (immediate) {
          processHeater();
      }
  };
  DUTSerial::enumErrorTypes processHeater();

private:
    //const int bufferSize = 64;
    uint8_t nackReason = 0;
    uint8_t currentHeaterStatus = 0;
    uint8_t queuedHeater = 0;
    uint8_t backlight = 255;
    uint8_t seqNum = 0;
    uart_inst_t *dutUart; // Serial Port to DUT

    // read response and return the type of response
    // sets Error Structure if respons does not match expected values
    uint8_t readResponse();
    // check the response to see if it echos back the data passed in
    enumErrorTypes checkResponse(uint8_t response, uint8_t cmdId, int data);

    // calculate the checksum on the data 
    uint16_t xsum(uint8_t *data, uint8_t cnt);

    // Read an entire response or wait until the data comes
    int readUntilDone(uint8_t buffer[],int size);

    uint8_t cmd = 0xAA;
    uint8_t cmdbuf[CONFIG_CMD_SIZE];
    uint8_t respbuf[STATUS_RESPONSE_SIZE];
    uint8_t regAddress; // these are options for the config command
    uint8_t regValue; // these are options for the config command

};

#endif /* SERIAL_H_ */
