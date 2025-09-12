/*
 * serial.c
 *
 *  Created on: Apr 20, 2022
 *      Author: chris
 */
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "DUTSerial.h"
#include "idc.h"
#include <cassert>
#include <cstdint>


#define SWAP(crc) ((crc)>>8 | (crc)<<8)

static const char* const bitErrors[5][8] = {{"3.3V", "Backlight","Temp Sensor","Memory","Memory CRC","Stuck Key","1.8V","5V"},
                    {"RS_170 invalid", "LVDS Invalid","Debug1","Debug2","Debug3","Debug4","spare","spare"},
                    {"Video Decoder", "FPGA","Bezel","Heater3","Heater2","Heater1","spare","spare"},
                    {"PBIT 3.3", "PBIT Backlight","PBIT Temp Sensor","PBIT HEATER3","PBIT Heater2","PBIT Heater1","PBIT 1.8V","PBIT 5V"},
                    {"Watch Dog Timer","PBIT Memory","spare","spare","spare","spare","spare","spare"}};

#define CRC_POLYNOM 0x8408
#define CRC_PRESET 0xA5A5
#define DHA_BAUD 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

//
// Constructor: Takes transmit and receive pis as well as baud rate
// Ensures port is in blocking read mode ans then flushes and data from the buffer
//
DUTSerial::DUTSerial(uint tx, uint rx, uint rts, uint cts) {
    uart_init(uart0, DHA_BAUD);

    gpio_set_function(tx, UART_FUNCSEL_NUM(uart0, UART_TX_PIN));
    gpio_set_function(rx, UART_FUNCSEL_NUM(uart0, UART_RX_PIN));
    
    uart_set_hw_flow(uart0, false, false);
    uart_set_format(uart0, DATA_BITS, STOP_BITS, PARITY);
    dutUart = uart0;
    uart_set_fifo_enabled(uart0,true);

    flush();
}


//  Calculates the checksum on the data.
// Note we only calculate the XSUM on the data portion of the packet.  Not the 
// cmd id or the crc itself.
uint16_t DUTSerial::xsum(uint8_t *data, uint8_t cnt) {
  uint16_t crc = CRC_PRESET;

  for (int i = 0; i < cnt; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++) {
      if (crc & 0x0001)
        crc = (crc >> 1) ^ CRC_POLYNOM;
      else
        crc = (crc >> 1);
    }
  }
  return crc;
}

void DUTSerial::flush()
{
    uint8_t c;
    //dut->set_blocking(false);
    while (uart_is_readable(dutUart))
        uart_read_blocking(dutUart,&c,1);
};



//send an illegal command and see what happens
DUTSerial::enumErrorTypes DUTSerial::badCmdTest()
  {
  ibitCommandStruct cmd;

  cmd.ibitCmd = 1;
  cmd.cmdId = PERFORM_IBIT;
  uint16_t crc = xsum(&cmd.ibitCmd, 1);
  cmd.crcM = crc>>8;
  cmd.crcL = crc&0x0FF;
  uart_write_blocking(dutUart,(uint8_t*)&cmd, sizeof(ibitCommandStruct)-1);
    uint8_t response = readResponse();

  return checkResponse(response,PERFORM_IBIT,1);
  };

// pretty print the IBIT byte 0 meaning
void DUTSerial::printBIT(uint8_t *results, unsigned byte_)
{
    if (byte_>5)
    {
        debug_print("ERROR: Invalid index in printBIT()\r\n");
        return;
    }
    if (byte_==5)
    {
        printf("|0x%x|",results[5]);
        return;
    }

    int BITByte = results[byte_];
    if (BITByte== 0)
    {
        printf("| No Errors |                   ");
        return;
    }
    else
    {
        printf("|");
    }
    //debug_print("BIT0 : 0x%x\r\n",BIT0);
    for (int i=0;BITByte;BITByte=BITByte>>1,i++)
    {
        if (BITByte&0x01)
        {
            if (byte_ == 1)
            { // byte 1 is invalid video  (dont print in red)
                printf("%s|", bitErrors[byte_][i]);
            }
            else
            {
                printf(" \033[31m%s\033[0m |", bitErrors[byte_][i]); 
            }
        }
    }
}



void DUTSerial::printComStatus()
{
    switch(errorStatus)
    {
        case noStatus:
            printf("\033[31mNo Response Requested\033[0m    ");
            break;
        case noError:
            printf("No Errors                   ");
            break;
        case badEcho:
            printf("\033[31mIncorrect Sequence Number\033[0m");
            break;
        case badCRC:
            printf("\033[31mIncorrect CRC\033[0m            ");
            break;
        case nackReceived:
            printf("\033[31mNACK Received reason=0x%x\033[0m",nackReason);
            break;
        case unexpectedResponse:
            printf("\033[31mUnexpected Response \033[0m     ");
            break;
        case unknownResp:
            printf("\033[31mUnknown Response Type\033[0m    ");
            break;
        case invalidSize:
            printf("\033[31mIncorrect Response Size\033[0m  ");
            break;
        case respTimeout:
            printf("\033[31mTimeout-Check Board Connection\033[0m");
            break;
        default:
            printf("Unknown Error            ");
        
    }
}


// general checks on the response packet.
// sets the errorStatus Flag
DUTSerial::enumErrorTypes DUTSerial::checkResponse(uint8_t response,uint8_t cmdId, int data)
{
    // respons ID should be the same as command ID without the high bit.
    uint8_t expectedResponse;
    if (cmdId == 0x85)
    {
        expectedResponse = 0x04;  // the iBIT command returns a status response
    }
    else
    {
        expectedResponse = (cmdId & 0x7F); // all other responses are just the low nibble of the cmd
    }
    if (response != expectedResponse) 
    {
        if (response == NACK)
        {
            errorStatus = nackReceived;
            debug_print("NACK Received. Reason Code = 0x%x\r\n",nackReason);
        }
        else if (response == 0xFF)
        {
            errorStatus = respTimeout;
        }
        else
        {
            errorStatus = unexpectedResponse;
            //debug_print("Unexpected Response Received (0x%x) expected (0x%x)\r\n",response,expectedResponse);
        }
    }
    else
    { 
        if (cmdId == STATUS)
        {
            if (data == currentStatus.echoSeqNumber)
            {
                return errorStatus;
            }
            else
            {
                errorStatus = badEcho;
                debug_print("Unexpected Sequence Number(%d)\r\n",currentStatus.echoSeqNumber);
            }
        }
    }
    return errorStatus;

}

//
// Send the Status Command to the DHA,
// Wait for a response and update the "currentStatus" structure
//
DUTSerial::enumErrorTypes DUTSerial::updateStatus() 
{
    flush();
    statusCommandStruct cmd;
    assert(sizeof(cmd)==STATUS_SIZE);
    cmd.seqNumber = ++seqNum;
    cmd.cmdId = STATUS;
    uint16_t crc = xsum(&cmd.seqNumber, 1);
    //debug_print("send data=%d, checksum = 0x%X\r\n",seqNum,crc);
    cmd.crcM = crc>>8;
    cmd.crcL = crc&0x0FF;
    if (!ignoreResponses) // this flag reads responses but does not request status
    {    
      uart_write_blocking(uart0,(uint8_t*)&cmd, sizeof(statusCommandStruct));  
      //uart_write_blocking(dutUart,(uint8_t*)&cmd, sizeof(statusCommandStruct));
    }



    int response = readResponse();
    if (response == 0xFF)
    {
        errorStatus = respTimeout;
    }
    else {
        if (noError == checkResponse(response,cmd.cmdId,seqNum))
        {
            backlight = currentStatus.backlightValue;
            currentHeaterStatus = currentStatus.currentHeaterEnabled;
        }
    }
    return errorStatus;

}

//
// set the backlight to a step between 0 (dark) to 255 (bright)
// Wait for the response.
//
DUTSerial::enumErrorTypes DUTSerial::setBacklight(uint8_t step) {
  //debug_print("Set Backlight to %d\r\n",step);
  backlightControlStruct cmd;
  assert(sizeof(cmd)==BACKLIGHT_CONTROL_SIZE);
  cmd.backlightStep = step;
  cmd.cmdId = BACKLIGHT_CONTROL;
  uint16_t crc = xsum(&cmd.backlightStep, 1);
  cmd.crcM = crc>>8;
  cmd.crcL = crc&0x0FF;
  //debug_print("data=%d, checksum = 0x%x%x\r\n",step,cmd.crcM,cmd.crcL);
  uart_write_blocking(dutUart,(uint8_t*)&cmd, sizeof(backlightControlStruct));
  //debug_print("Read Response\r\n");
  int response = readResponse();

  return checkResponse(response,cmd.cmdId,step);
}

//
// Send SetConfiguration Packet
// Currently enables Bezel
// Timeout and OutDiscretes are ignored
//
DUTSerial::enumErrorTypes DUTSerial::sendConfig(uint8_t ccmd, uint8_t data[CONFIG_DATA_SIZE]) {
    //debug_print("Set Backlight to %d\r\n",step);
  configControlStruct cmd;
  cmd.cmdId = CONFIG_CMD;
  assert(sizeof(cmd)==CONFIG_CMD_SIZE);
  cmd.cmd = ccmd;
  cmd.data[0] = data[0];
  cmd.data[1] = data[1];
  uint16_t crc = xsum(&cmd.cmd, 3);
  cmd.crcM = (crc >> 8);
  cmd.crcL = crc;
  uart_write_blocking(dutUart,(uint8_t*)&cmd, sizeof(configControlStruct));
  //debug_print("Read Response\r\n");
  int response = readResponse();
  if (response == 0xFF)
  {
      return respTimeout;
  }
  enumErrorTypes stat = checkResponse(response,cmd.cmdId,ccmd);
  if (stat==noError)
  {
      data[0]=regAddress;
      data[1]=regValue;
  }

  return stat;
}

// turn the heater on or off
// Wait for a response
DUTSerial::enumErrorTypes DUTSerial::processHeater() {
    if (currentHeaterStatus != queuedHeater)
    {
        heaterControlStruct cmd;
        cmd.heaterEnabled = queuedHeater;
        cmd.cmdId = HEATER_CONTROL;
        uint16_t crc = xsum(&cmd.heaterEnabled, 1);
        cmd.crcM = crc>>8;
        cmd.crcL = crc&0x0FF;
        uart_write_blocking(dutUart,(uint8_t*)&cmd, sizeof(heaterControlStruct));

        int response = readResponse(); // this updates currentHeaterStatus

        return checkResponse(response,cmd.cmdId,queuedHeater);
    }
    return noError;
}

// Run IBIT
//Wait for response and update the CurrentStatus structure
//
DUTSerial::enumErrorTypes DUTSerial::runIBIT() {
  ibitCommandStruct cmd;
  cmd.ibitCmd = 1;
  cmd.cmdId = PERFORM_IBIT;
  uint16_t crc = xsum(&cmd.ibitCmd, 1);
  cmd.crcM = crc>>8;
  cmd.crcL = crc&0x0FF;
  uart_write_blocking(uart0,(uint8_t*)&cmd, sizeof(ibitCommandStruct));
  uint8_t response = readResponse();

  return checkResponse(response,cmd.cmdId,1);
}

//
// We should not really need this as we delay long enough that the full message should
// be in the read buffer by the time we get around to reading it
// but if the message is shorter than expected, this function will wait until the full message is received 
//
int DUTSerial::readUntilDone(uint8_t buffer[], int size) {
    int bytesRead=0;
    int tries=0;

    while(bytesRead< size)
    {
        if (uart_is_readable_within_us(dutUart,1000))
        {
            buffer[bytesRead++] = uart_getc(dutUart);
        }
        else
        {
            tries++;
        }
        if (tries>3)
        {
            
            debug_print("received %d of %d expected bytes\r\n",bytesRead,size);
            break;
        }
    }
    return bytesRead;
}

//
// Read the response from the DHA for any command, update the status structures and error flags 
// Return the Response ID read (first Byte)
//
uint8_t DUTSerial::readResponse() {
  uint8_t responseId;

  if (!uart_is_readable_within_us(uart0,500000)) // max timeout = 500mS (0.5Sec)
  {
      errorStatus=respTimeout;
      //debug_print("Timeout waiting for response\r\n");
      return 0XFF;
  }


  // at this point there is definitly a char in the buffer  
  uart_read_blocking(dutUart,&responseId, 1);
  int bytesRead = 1;
  //debug_print("Response ID = 0x%x\r\n",responseId);

  errorStatus = noError;
  sleep_us(86*STATUS_RESPONSE_SIZE);  //It takes a little less 86uS to send 1 char at 115200bps so wait until we get all the characters
  switch (responseId) {
    case HEATER_CONTROL_RESPONSE: 
    {
        heaterResponseStruct tmp;
        tmp.respId = responseId;
        //debug_print("bytesread=%d, hearerRespStructSize=%d\r\n",bytesRead,sizeof(heaterResponseStruct));
        bytesRead +=
            readUntilDone(&tmp.currentHeaterEnabled, (sizeof(heaterResponseStruct) - 1));
        if (bytesRead != (HEATER_CONTROL_RESPONSE_SIZE))
        {
            debug_print("Incorrect heater Response Length (RespID=%d),Length=%d\r\n",responseId,bytesRead);
            responseId=0;
            errorStatus = invalidSize;
            //dut->sync();
        }
        else 
        {
            uint16_t x = xsum(&tmp.currentHeaterEnabled, HEATER_CONTROL_RESPONSE_SIZE-3);
            if (x!=(tmp.crcM<<8|tmp.crcL))
            {
                errorStatus = badCRC;
                debug_print("Bad CRC (0x%x%x) in Status Response. expected (0x%x)\r\n",tmp.crcM,tmp.crcL,x);
            }
            else
            {
                currentHeaterStatus = tmp.currentHeaterEnabled;
            }
        }
        
        break;
    }
    case BACKLIGHT_CONTROL_RESPONSE: 
    {
        backlightResponseStruct tmp;
        tmp.respId = responseId;

        bytesRead += readUntilDone(&tmp.currentBacklightVal,
                            sizeof(backlightResponseStruct) - 1);
        if (bytesRead != BACKLIGHT_CONTROL_RESPONSE_SIZE)
        {
            debug_print("Incorrect Response Length (RespID=%d),Length=%d\r\n",responseId,bytesRead);
            responseId=0;
            errorStatus = invalidSize;
        }
        else 
        {
            uint16_t x = xsum(&tmp.currentBacklightVal, BACKLIGHT_CONTROL_RESPONSE_SIZE-3);
            if (x!=(tmp.crcM<<8|tmp.crcL))
            {
                errorStatus = badCRC;
                debug_print("Bad CRC (0x%x%x) in Backlight response; expected (0x%x)\r\n",tmp.crcM,tmp.crcL,x);
            }

            backlight = tmp.currentBacklightVal; 
            //debug_print("id=0x%x, value= 0x%x,crc=0x%x%x\r\n",responseId,tmp.currentBacklightVal,tmp.crcM,tmp.crcL);
        }
        break;
    }
    case STATUS_RESPONSE: 
    {
        // statusResponseStruct tmp;
        currentStatus.respId = responseId;
        int size = sizeof(statusResponseStruct) - 1;
        //debug_print("** status response - reading %d bytes \r\n",size);
        bytesRead += readUntilDone(&currentStatus.echoSeqNumber,  size);
        //debug_print("Read %d bytes, Seq number = %d\r\n",bytesRead,currentStatus.echoSeqNumber);
        if (bytesRead != STATUS_RESPONSE_SIZE)
        {
            debug_print("Incorrect Response Length (RespID=%d),Length=%d\r\n",responseId,bytesRead);
            responseId=0;
            errorStatus = invalidSize;
            //dut->sync();
        }
        else 
        {
            uint16_t x = xsum(&currentStatus.echoSeqNumber, STATUS_RESPONSE_SIZE-3);
            if (x!=(currentStatus.crcM<<8|currentStatus.crcL))
            {
                errorStatus = badCRC;
                debug_print("Bad CRC (0x%x%x) in Status Response; expected (0x%x)\r\n",currentStatus.crcM,currentStatus.crcL,x);
            }

        }
        break;
    } 
    case CONFIG_RESPONSE: {
        configResponseStruct tmp;
        bytesRead += readUntilDone(&tmp.currentCmd, sizeof(configResponseStruct) - 1);
        uint8_t ccmd = tmp.currentCmd;
        if (bytesRead != CONFIG_RESPONSE_SIZE)
        {
            debug_print("Incorrect Response Length (RespID=%d),Length=%d\r\n",responseId,bytesRead);
            responseId=0;
            errorStatus = invalidSize;
            //dut->sync();
        }
        else 
        {
            uint16_t x = xsum(&tmp.currentCmd, CONFIG_RESPONSE_SIZE-3);
            if (x!=(tmp.crcM<<8|tmp.crcL))
            {
                errorStatus = badCRC;
                debug_print("Bad CRC (0x%x%x) in Config Command Response. expected (0x%x)\r\n",tmp.crcM,tmp.crcL,x);
            }
            else
            {
                if ((ccmd == VIDEO_WRITE ) || (ccmd == VIDEO_READ))
                {
                    regAddress = tmp.data[0];
                    regValue = tmp.data[1];
                }
            }

        }
    } break;
    case NACK:
        nackResponseStruct tmp;
        bytesRead += readUntilDone(&tmp.reasonCode, sizeof(nackResponseStruct) -1);
        nackReason = tmp.reasonCode;
        errorStatus = nackReceived;
        debug_print("id=0x%x, value= 0x%x,crc=0x%x%x\r\n",responseId,tmp.reasonCode,tmp.crcM,tmp.crcL);
    default:
        //dut->sync(); // flush the buffer
        bytesRead = 0;
        errorStatus = unknownResp;
  }
  //dut->sync();
  flush();
  return responseId;
}
