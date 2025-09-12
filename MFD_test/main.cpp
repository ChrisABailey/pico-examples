#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/adc.h"
#include "pico/sync.h"
#include <tusb.h>
#include "dio.h"
#include "tone.h"
//#include "mI2C.h"
#include "DUTSerial.h"
#include "T38BoardIO.h"
#include "idc.h"
#include "SerialUI.h"
#include <cstring>
#include <ctype.h>
#include <stdio.h>
#include "adv7393.h"
#include <ctime>

#define AC_HERTZ 400
const char *RELEASE = "1.0.0";

// dut is determined by looking at Bezel discrete signals
enum dutTypes
{
    EED,
    MFD,
    UNKNOWN
};
dutTypes dutType = UNKNOWN;
DigitalIn configPin1(ETHIO_1, DigitalIn::PinMode::PullUp);
DigitalIn configPin2(ETHIO_2, DigitalIn::PinMode::PullUp);
DigitalOut configPin3(ETHIO_3); // this is a gpio, we drive it high as part of our IO test

DigitalIn bit(BIT_STATUS); // treat bit error as interupt so we detect it immediatly
DigitalIn bitInProgress(BIT_PROGRESS);
DigitalIn dhaOff(DHA_OFF, DigitalIn::PinMode::PullUp);
DigitalOut bezelLuminance(BEZEL_LUMINANCE);
DigitalOut calibrationMode(MODE_SEL);
DigitalOut lvdsRs170(LVDS_RS170);

PwmOut acPwm(AC_PWM);
DigitalOut led1(LED1);


// AnalogIn adc_temp(ADC_TEMP); // internal temperature of  test board MCU

// Class that encapsulates serial interface to DUT
DUTSerial dut(UART_TX, UART_RX, UART_RTS, UART_CTS);

// class that talks to ADV7393 (RS-170 Driver chip)
// Some boards may not have this installed
adv7393 rs170ic(ADV7393_SDA, ADV7393_SCL);
bool internalRS170 = INTERNAL_RS170;
bool ignoreResponses = false;

uint8_t testPatternOn = false;
const int HISTORY_LENGTH = 20;
int iBitErrorCount = 0;
uint8_t BITHistory[5][HISTORY_LENGTH];
int BITTime[HISTORY_LENGTH];

////////////////////////////////////////////////
// Heater Control Variables, Test board can automatically
// turn Heater on and off based on temperature
/////////////////////////////////////////////////
enum heaterControl
{
    AUTO,
    ON,
    OFF
};
heaterControl heaterMode = heaterControl::AUTO;

int heaterOnTemp = 0;
int heaterOffTemp = 10;
int heaterMaxOnTime = 60;
// Interupt driven IO
repeating_timer_t heaterControlTicker;

/////////////////////////////////////////////////
// Internal State Variables used in auto-test mode
/////////////////////////////////////////////////
absolute_time_t programStartTime;
int latestIBit = 0;
bool timeForiBIT = false;
bool autoIBIT = false;
bool cycleBacklight = false;

#ifdef ECHO
bool echo = true;
#else
bool echo = false;
#endif

int getElapsedTime(int &hour, int &min, int &sec)
{
    // int time = elapsedTime.read_ms();
    absolute_time_t curr_time = get_absolute_time();

    // Calculate elapsed time in microseconds
    int64_t elapsed_us = absolute_time_diff_us(programStartTime, curr_time);

    double totalsec = (double)elapsed_us / 1000000;

    hour = (int)totalsec / 3600;
    min = ((int)totalsec % 3600) / 60;
    sec = ((int)totalsec % 60);
    return (int)totalsec;
}

dutTypes getDutType()
{
    if (configPin1 == 0 && configPin2 == 1)
        return dutTypes::EED;
    if (configPin1 == 1 && configPin2 == 0)
        return dutTypes::MFD;
    return dutTypes::UNKNOWN;
}

const char *getTestMode()
{
    switch (getDutType())
    {
    case dutTypes::EED:
        return "EED DHA";
    case dutTypes::MFD:
        return "MFD DHA";
    default:
        return "\033[31mNo Bezel\033[0m";
    }
}

void printButtons(bool start, bool MFD)
{
    static uint8_t upper = 0;
    static uint8_t lower = 0;
    static uint8_t left = 0;
    static uint8_t right = 0;
    static uint8_t switches = 0;

    if (start)
    {
        upper = lower = left = right = switches = 0;
    }

    upper |= dut.currentStatus.bezelButtonsUpper;
    lower |= dut.currentStatus.bezelButtonsLower;
    left |= dut.currentStatus.bezelButtonsLeft;
    right |= dut.currentStatus.bezelButtonsRight;
    switches |= dut.currentStatus.rockerSwitches;

    if (MFD)
    {
        printf("Upper Buttons:    \t %s %s %s %s %s %s  \r\n", upper & 0x01 ? "1" : "x",
               upper & 0x02 ? "2" : "x",
               upper & 0x04 ? "3" : "x",
               upper & 0x08 ? "4" : "x",
               upper & 0x10 ? "5" : "x",
               upper & 0x20 ? "6" : "x");
        printf("Left Buttons:     \t %s %s %s %s %s %s %s\r\n", left & 0x01 ? "1" : "x",
               left & 0x02 ? "2" : "x",
               left & 0x04 ? "3" : "x",
               left & 0x08 ? "4" : "x",
               left & 0x10 ? "5" : "x",
               left & 0x20 ? "6" : "x",
               left & 0x40 ? "7" : "x");
        printf("Right Buttons:    \t %s %s %s %s %s %s %s\r\n", right & 0x01 ? "1" : "x",
               right & 0x02 ? "2" : "x",
               right & 0x04 ? "3" : "x",
               right & 0x08 ? "4" : "x",
               right & 0x10 ? "5" : "x",
               right & 0x20 ? "6" : "x",
               right & 0x40 ? "7" : "x");
        printf("Lower Buttons:    \t %s %s %s %s %s %s  \r\n", lower & 0x01 ? "1" : "x",
               lower & 0x02 ? "2" : "x",
               lower & 0x04 ? "3" : "x",
               lower & 0x08 ? "4" : "x",
               lower & 0x10 ? "5" : "x",
               lower & 0x20 ? "6" : "x");
        printf("Rocker Switches:  \t %s %s %s %s %s %s  \r\n", switches & 0x01 ? "BRT^" : "x",
               switches & 0x02 ? "BRTv" : "x",
               switches & 0x04 ? "HDG^" : "x",
               switches & 0x08 ? "HDGv" : "x",
               switches & 0x10 ? "CRS^" : "x",
               switches & 0x20 ? "CRSv" : "x    ");
    }
    else
    {
        printf("Buttons:          \t %s %s %s %s %s     \r\n", upper & 0x01 ? "B^" : "x",
               upper & 0x02 ? "Bv" : "x",
               upper & 0x08 ? "T" : "x",
               upper & 0x10 ? "M" : "x",
               upper & 0x20 ? "C" : "x   ");
    }
}

///
// Note there 31-34 newline characters in this printStatus() function
// The erase Status function moves the cursor the same number of lines up so we can
// update the data without scrolling the screen.  If you change one function
// be sure to update the oter to match.
///
void printStatus()
{
    int hour, min, sec;
    bool error = false;
    getElapsedTime(hour, min, sec);
    const char *spaces = "                ";
    printf("\r\n=======================================\r\n");
    printf("    %d:%02d:%02d        \t %s              %s\r\n", hour, min, sec, getTestMode(), spaces);
    printf("==Last Status Msg==\t ==============%s\r\n", spaces);
    printf("SequenceNumber:  \t %d           %s\r\n", dut.currentStatus.echoSeqNumber, spaces);
    printf("Current Video:   \t %s           %s\r\n", (dut.currentStatus.inputDiscreteSettings & VIDEO_MODE) == 0 ? "RS-170" : "LVDS", spaces);
    printf("Calibration Mode:\t %s           %s\r\n", (dut.currentStatus.inputDiscreteSettings & CALIBRATION_MODE) == 0 ? "Normal" : "Calibration", spaces);
    printf("Programming Pin: \t %d           %s\r\n", dut.currentStatus.inputDiscreteSettings & PROGRAMMING, spaces);
    printf("Spare GPIO Pin:  \t %d           %s\r\n", dut.currentStatus.inputDiscreteSettings & SPARE, spaces);
    printf("Heater Status:   \t %s           %s\r\n",
           dut.currentStatus.currentHeaterEnabled == 0 ? "OFF" : "ON", spaces);
    printf("Day/Night Mode:  \t %s           %s\r\n",
           (dut.currentStatus.dayNight == 1 ? "DAY  " : "NIGHT"), spaces);
    printf("BIT In Progress: \t %s           %s\r\n", (dut.currentStatus.outputDiscreteSettings & BIT_IN_PROGRESS) == 0 ? "No" : "Yes", spaces);
    printf("BIT Summary:     \t %s           %s\r\n", (dut.currentStatus.outputDiscreteSettings & BIT_SUMMARY) == 0 ? "\033[31mError\033[0m" : "OK", spaces);
    printf("Backlight Step:  \t %d           %s\r\n", dut.currentStatus.backlightValue, spaces);
    printf("Bezel Voltage:   \t %dmV          %s\r\n", dut.getBezelVoltage(), spaces);

    if (abs(dut.getMCUTemp() - dut.getLCDTemp()) > 10)
    {
        error = true;
        printf("\033[31m");
    }

    printf("LCD Temp:        \t %2.2fdeg C     %s\r\n", dut.getLCDTemp() - DEGC_TO_K, spaces);
    printf("MCU Temperature:  \t %2.1fdeg C     %s\r\n", dut.getMCUTemp() - DEGC_TO_K, spaces);

    if (error)
    {
        error = false;
        printf("\033[0m");
    }

    for (int b = 0; b < 6; b++)
    {
        printf("BIT Byte %d:      \t ", b);
        dut.printBIT(dut.getCurrentBITResults(), b);
        printf("\r\n");
    }

    printf("Software Version: \t %X.%X          %s\r\n", dut.currentStatus.swVersionMsb, dut.currentStatus.swVersionLsb, spaces);
    if (dutTypes::EED != getDutType())
    {
        printf("Firmware Version: \t %X.%X          %s\r\n", dut.currentStatus.fwVersionMsb, dut.currentStatus.fwVersionLsb, spaces);
    }

    printf("=====Buttons===== \t =========== %s\r\n", spaces);
    printButtons(true, dutTypes::EED != getDutType());
    printf("================= \t ===========   %s\r\n", spaces);

    printf("Serial Com Status:   \t ");
    dut.printComStatus();

    printf("\r\n=Discrete Pins ==\t =========== %s\r\n", spaces);
    printf("BIT Discrete:        \t %s           %s\r\n",
           bit == 0 ? "\033[31mError\033[0m" : "OK    ", spaces);
    printf("BIT in Progress:     \t %s           %s\r\n",
           bitInProgress == 0 ? "No" : "Yes", spaces);
    if (getDutType() == dutTypes::MFD)
    {
        printf("LVDS/RS-170 Mode:\t %s        %s\r\n",
               lvdsRs170 == 1 ? "LVDS" : "RS-170", spaces);
    }
    if (getDutType() == dutTypes::EED)
    {
        printf("Calibration Mode:  \t %s       %s\r\n",
               calibrationMode == 0 ? "Normal" : "Calibration", spaces);
    }
    printf("Power knob:        \t %s           %s\r\n",
           dhaOff == 0 ? "ON " : "OFF", spaces);
    printf("=================\t =========== %s\r\n", spaces);
}

///////////////////////////////////////////////
// Move Cursor up to overwrite previouse status
///////////////////////////////////////////////
void eraseStatus()
{
    if (getDutType() == dutTypes::EED)
    {
        // 34 lines
        printf("\033[34A");
    }
    else
    {
        // 40 lines
        printf("\033[40A");
    }
}

int getMCUInternalTemp()
{
    return 0; //__LL_ADC_CALC_TEMPERATURE(((uint32_t)3300U),adc_temp.read_u16()>>4,LL_ADC_RESOLUTION_12B);
}

///////////////////////////////////////////////
// ISR based on the BIT descrete changing
///////////////////////////////////////////////
void bitRise(uint gpio, uint32_t events)
{
    if (events & GPIO_IRQ_EDGE_RISE)
    {
        led1 = 1;
    }
    else
    {
        led1 = 0;
    }
}

///////////////////////////////////////////////
// This is an ISR
// Every timer tick, update the heater based
// on reported temperatures.
///////////////////////////////////////////////
bool heaterControlCallback(__unused struct repeating_timer *t)
{

    float temp = dut.getLCDTemp();
    // temp returned is in DEG K.  If there was an error in the communications it will return 218.15 deg K = -55 deg C
    if ((temp < DEGCTOK(-45)) or (temp > DEGCTOK(40)))
    {   // dont risk it if the heater is below -50
        // the thermister may be broken
        // printf("Turning off heater due to suspect temperature reading (%2.2f Deg C)",DEGKTOC(temp));
        dut.setHeater(false);
        return true;
    }

    static absolute_time_t heaterTurnOnTime = get_absolute_time();
    // static float on_temp = -5.0f;


    // in auto mode we want to turn on the heater if the temp is below heaterOnTemp
    // and turn it off when it reaches heater on temp
    if (heaterMode == heaterControl::AUTO)
    { // NOT on now so see if we need to turn it on
        if (temp < DEGCTOK(heaterOnTemp))
        {
            dut.setHeater(true);
            heaterTurnOnTime = get_absolute_time(); // record start time to prevent us from
                                           // leaving the heater on too long
        }
        else if (temp > DEGCTOK(heaterOffTemp))
        {
            dut.setHeater(false);
            heaterTurnOnTime = 0;
        }
    }
    else if (heaterMode == heaterControl::ON)
    {
        if (heaterTurnOnTime == 0)
        {
            dut.setHeater(true);
            heaterTurnOnTime = get_absolute_time(); // record start time to prevent us from
        }
        else if (absolute_time_diff_us(heaterTurnOnTime,get_absolute_time())/1000000 > heaterMaxOnTime) // in manual mode only run the heater for 60Sec
        {
            heaterMode = heaterControl::OFF;
            heaterTurnOnTime = 0;
            dut.setHeater(false);
        }
    }
    else if (heaterMode == heaterControl::OFF)
    {
        dut.setHeater(false);
        heaterTurnOnTime = 0;
    }
    else
    {
        dut.setHeater(false);
    }
    return true;
}

///////////////////////////////////////////////
// this gets called every second
// Slowly adjust the backlight
// every 255 times through the run IBIT
//
// Errors reported by DHA Status get counted
//
// and are reported by the main loop
//////////////////////////////////////////////
int updateAutoTest()
{
    static int count = 100;

    int errors = 0;
    if (count++ < 255)
    {
        if (cycleBacklight)
            dut.setBacklight(count);
    }
    else
    {
        count = 0;
    }

    // Kick off an IBIT occationally but before we do,
    // print a status line of any bits that are about to be cleared
    if (count == 200 && autoIBIT)
    {

        uint8_t *BITTable = dut.getCurrentBITResults();
        if (BITTable[0] != 0 || BITTable[2] != 0 || BITTable[4] != 0)
        {
            int hour, min, sec;
            memcpy(BITHistory[iBitErrorCount % HISTORY_LENGTH], BITTable, 5);
            BITTime[iBitErrorCount % HISTORY_LENGTH] = getElapsedTime(hour, min, sec);
            iBitErrorCount++;
            if (BITTable[0] != 0)
            {
                printf("\r\nClearing BIT Byte 0 Error(s):    \t");
                dut.printBIT(BITTable, 0);
                printf("\r\n");
                errors++;
            }
            if (BITTable[2] != 0)
            {
                printf("\r\nClearing BIT Byte 2 Error(s):    \t");
                dut.printBIT(BITTable, 2);
                printf("\r\n");
                errors++;
            }
            if (BITTable[4] != 0)
            {
                printf("\r\nClearing BIT Byte 4 Error(s):    \t");
                dut.printBIT(BITTable, 4);
                printf("\r\n");
                errors++;
            }
        }

        dut.runIBIT(); // note this clears error other than PBIT errors
    }
    return errors;
}

// allows the retreval if individual status values
// used in automated test scripts
// Prefix these commands with "$"
int getSubStatus()
{
    uint8_t test;
    if (echo)
    {
        printf("Substatus: (?=help)\r\n");
    }

    test = getchar_timeout_us(0x7FFFFFFF);
    test = tolower(test);
    switch (test)
    {
    case ('?'):
    {
        printf(subMenu);
        break;
    }
    case ('@'):
    {
        printf("READY;");
        break;
    }
    case ('b'):
    {
        printf("%d;", (dut.currentStatus.backlightValue));
        break;
    }
    case ('c'):
    {
        printf("%c;", (dut.currentStatus.inputDiscreteSettings & CALIBRATION_MODE) == 0 ? 'n' : 'c');
        break;
    }
    case ('d'):
    {
        printf("%c", (dhaOff == 0) ? (dut.currentStatus.dayNight == 1 ? 'd' : 'n') : 'o');
        break;
    }
    case ('f'):
    {
        printf("%02X.%02X;", dut.currentStatus.fwVersionMsb, dut.currentStatus.fwVersionLsb);
        break;
    }
    case ('h'):
    {
        bezelLuminance = 1;
        printf("h;");
        break;
    }
    case ('k'):
    {
        printf("%02X%02X%02X%02X%02X;",
               dut.currentStatus.bezelButtonsUpper,
               dut.currentStatus.bezelButtonsLower,
               dut.currentStatus.bezelButtonsLeft,
               dut.currentStatus.bezelButtonsRight,
               dut.currentStatus.rockerSwitches);
        break;
    }
    case ('l'):
    {
        bezelLuminance = 0;
        printf("l;");
        break;
    }
    case ('m'):
    {
        printf("%c", (dut.currentStatus.inputDiscreteSettings & VIDEO_MODE) == 0 ? 'r' : 'l');
        break;
    }
    case ('s'):
    {
        printf("%02X.%02X;", dut.currentStatus.swVersionMsb, dut.currentStatus.swVersionLsb);
        break;
    }
    case ('t'):
    {
        printf("%2.1f;", dut.getMCUTemp() - DEGC_TO_K);
        break;
    }
    case ('v'):
    {
        printf("%d;", (dut.currentStatus.bezelBrightnessVoltageMsb * 256 + dut.currentStatus.bezelBrightnessVoltageLsb));
        break;
    }
    case ('0'):
    {
        printf("%d;", dut.currentStatus.bitResultsByte0);
        break;
    }
    case ('1'):
    {
        printf("%d;", dut.currentStatus.bitResultsByte1);
        break;
    }
    case ('2'):
    {
        printf("%d;", dut.currentStatus.bitResultsByte2);
        break;
    }
    case ('3'):
    {
        printf("%d;", dut.currentStatus.bitResultsByte3);
        break;
    }
    case ('4'):
    {
        printf("%d;", dut.currentStatus.bitResultsByte4);
        break;
    }
    }
    return 0;
}
////////////////////////////////////////////////
// These are settings the test board uses (not the DUT)
////////////////////////////////////////////////
int configureTestBoard()
{
    uint8_t test;
    int newIntValue;

    dut.setHeater(false, true); // turn off the heater because this could take a while

    printf("\r\n Test Board Configuration.\r\n");
    printf("Note: Any changes will be reset at board bootup\r\n");
    printf("h - Setup Heater Controls: \r\n");
    printf("w - Write ADV7393 Video Register: \r\n");
    printf("g - Get ADV7393 Vidoe Register: \r\n");
    printf("b - Toggle Cycle Backlight in Auto Test Mode (Currently %s) \r\n", cycleBacklight ? "ON" : "OFF");
    printf("i - Toggle Sending IBIT in Auto Test Mode (currently %s) \r\n", autoIBIT ? "ON" : "OFF");
    printf("p - poll for bad responses \r\n");
    printf("x - send bad cmd \r\n");
    printf("enter - return to main menu. \r\n");

    test = getchar_timeout_us(0x7FFFFFFF);
    test = tolower(test);
    if (test == 'h')
    {
        printf("\r\nTurn Heater On Below Temperature (-40 to 30 Deg C)(currently %d):", heaterOnTemp);
        newIntValue = getInt(true);
        if (newIntValue > -40 && newIntValue < 30)
        {
            heaterOnTemp = newIntValue;
            printf("\r\nNew Heater On Temp = %d Dec C\r\n", heaterOnTemp);
        }
        else
        {
            printf("\r\nValue ignored (must be between -40 and 30 Deg C)\r\n");
        }

        printf("\r\nTurn Heater Turn Off above Temperature (currently %d Deg C):", heaterOffTemp);
        newIntValue = getInt(true);
        if (newIntValue > heaterOnTemp && newIntValue < 20)
        {
            heaterOffTemp = newIntValue;
            printf("\r\nNew Heater Off Temp = %d Dec C\r\n", heaterOffTemp);
        }
        else
        {
            printf("\r\nValue ignored (must be between %d and 20 Deg C)\r\n", heaterOnTemp);
        }

        printf("\r\nIn manual mode, turn heater off after xxx Seconds (currently %d Sec):", heaterMaxOnTime);
        newIntValue = getInt(true);
        if (newIntValue > 0)
        {
            heaterMaxOnTime = newIntValue;
            printf("\r\nNew Heater Off Time = %d Sec\r\n", heaterMaxOnTime);
        }
        else
        {
            printf("\r\nValue ignored (must be > 0)\r\n");
        }
    }
    else if (test == 'w')
    {
        printf("Write Reg Addr: ");
        int reg = getInt(true);
        printf(" Value:");
        int value = getInt(true);
        bool rc = rs170ic.writeRegister(reg, value);
        printf(" %s\r\n", rc ? "Success" : "Failure");
    }
    else if (test == 'g')
    {
        printf("Get Reg Addr: ");
        int reg = getInt(true);
        int val = rs170ic.readRegister(reg);
        printf(">> 0X%02X = %d(0X%02X)\r\n", reg, val, val);
    }
    else if (test == 'b')
    {
        cycleBacklight = !cycleBacklight;
    }
    else if (test == 'i')
    {
        autoIBIT = !autoIBIT;
    }
    else if (test == 'c')
    {
        rs170ic.configureTestPatern();
    }
    else if (test == 'p')
    {
        ignoreResponses = !ignoreResponses;
    }
    else if (test == 'x')
    {
        dut.badCmdTest();
    }
    return 0;
}

void setRS170()
{
    testPatternOn = 0;
    dut.internalTestPattern(testPatternOn);
    if (internalRS170)
    {
        rs170ic.configureTestPatern();
    }
    lvdsRs170 = 0;
}

void setLVDS()
{
    lvdsRs170 = 1;
    testPatternOn = 0;
    dut.internalTestPattern(testPatternOn);
}

////////////////////////////////////////////////
// These are settings on the DUT set vial the Config Command
////////////////////////////////////////////////
int configureDHABoard()
{
    uint8_t test;
    int reg;
    int data;
    uint8_t cmd[CONFIG_DATA_SIZE];

    dut.setHeater(false, true); // turn off the heater because this could take a while

    printf("\r\n DHA Control Board Configuration.\r\n");
    printf("Note: Any changes will be reset at next power cycle\r\n");
    printf("t - Toggle FPGA Test Pattern: \r\n");
    printf("s - Set UART timeout length (seconds)");
    printf("r - Read Video Decoder Register: \r\n");
    printf("w - Write Video Decoder Register: \r\n");
    printf("d - Dump Video Decoder Register Block: \r\n");
    printf("a - Adjust Video Decoder A Gain (green)\r\n");
    printf("b - Adjust Video Decoder A Gain (red)\r\n");
    printf("c - Adjust Video Decoder A Gain (blue)\r\n");

    printf("enter - return to main menu. \r\n");
    test = getchar_timeout_us(0x7FFFFFFF); // blocking read
    test = tolower(test);
    switch (test)
    {
    case ('r'):
    {
        printf("Register Addr:");
        reg = getInt(true);
        if ((reg >= 0) && (reg < 256))
        {
            cmd[0] = reg;
            cmd[1] = 0;
            dut.sendConfig(VIDEO_READ, cmd);
            printf("\r\n reg[%d(0X%02X)] = 0X%02X\r\n", cmd[0], cmd[0], cmd[1]);
        }
        break;
    }
    case ('d'):
    {
        printf("First Register to Read (0=255):");
        int reg1 = getInt(true);
        printf("\r\nLast Register to Read (0=255):");
        int reg2 = getInt(true);
        printf("\r\nRegister \tValue\r\n");
        for (reg = reg1; reg <= reg2; reg++)
        {
            cmd[0] = reg;
            cmd[1] = 0;
            dut.sendConfig(VIDEO_READ, cmd);
            printf("%d(0X%02X) \t0X%02X\r\n", cmd[0], cmd[0], cmd[1]);
        }
        break;
    }
    case ('w'):
    {
        printf("Register Addr:");
        reg = getInt(true);
        if ((reg >= 0) && (reg < 256))
        {
            printf("\r\nValue:");
            data = getInt(true);
            cmd[0] = reg;
            cmd[1] = data;
            dut.sendConfig(VIDEO_WRITE, cmd);
            printf("\r\n reg[%d(0X%02X)] = 0X%02X\r\n", cmd[0], cmd[0], cmd[1]);
        }
        break;
    }

    case ('t'):
    {
        if (testPatternOn)
        {
            testPatternOn = 0;
            printf("\r\nFPGA internal test Pattern OFF\r\n");
        }
        else
        {
            testPatternOn = 1;
            printf("\r\nFPGA internal test Pattern ON\r\n");
        }
        dut.internalTestPattern(testPatternOn);
        break;
    }

    case ('s'):
    {
        printf("UART Timeout (integer seconds):");
        uint16_t timeout = getInt(true);
        dut.setTimeout(timeout);
        break;
    }

    case 'a':
    {
        uint8_t cmd[CONFIG_DATA_SIZE];
        printf("\r\nGain A:");
        int v = getInt(true);
        int wv = 0xc0 + (v >> 8);
        cmd[0] = 0x6c;
        cmd[1] = wv;
        dut.sendConfig(VIDEO_WRITE, cmd);
        cmd[0] = 0x6d;
        cmd[1] = v & 0xff;
        dut.sendConfig(VIDEO_WRITE, cmd);
        break;
    }
    case 'b':
    {
        uint8_t cmd[CONFIG_DATA_SIZE];
        int lowNibble;

        cmd[0] = 0x6f;
        cmd[1] = 0;
        dut.sendConfig(VIDEO_READ, cmd);
        lowNibble = cmd[1] & 0x0F;

        printf("\r\nGain B:");
        int v = getInt(true);
        int wv = 0xc0 + (v >> 4);
        cmd[0] = 0x6e;
        cmd[1] = wv;
        dut.sendConfig(VIDEO_WRITE, cmd);
        cmd[0] = 0x6f;
        cmd[1] = ((v & 0x0f) << 4) | lowNibble;
        dut.sendConfig(VIDEO_WRITE, cmd);
        break;
    }
    case 'c':
    {
        uint8_t cmd[CONFIG_DATA_SIZE];
        int highNibble;

        cmd[0] = 0x6f;
        cmd[1] = 0;
        dut.sendConfig(VIDEO_READ, cmd);
        highNibble = cmd[1] & 0xF0;

        printf("\r\nGain C:");
        int v = getInt(true);
        int wv = highNibble | (v >> 8);
        cmd[0] = 0x6f;
        cmd[1] = wv;
        dut.sendConfig(VIDEO_WRITE, cmd);
        cmd[0] = 0x70;
        cmd[1] = v & 0xff;
        dut.sendConfig(VIDEO_WRITE, cmd);
        break;
    }
    default:
        printf("Received %c (%d)\r\n", test, test);
        break;
    }

    return 0;
}

void handleBezelFunctions()
{
    if (dut.currentStatus.bezelButtonsLeft == 0x01)
    {
        if (dut.currentStatus.bezelButtonsUpper == 0x01)
        {
            setRS170();
        }
        else if (dut.currentStatus.bezelButtonsUpper == 0x02)
        {
            setLVDS();
        }
        else if (dut.currentStatus.bezelButtonsUpper == 0x04)
        {
            dut.internalTestPattern(testPatternOn = 1);
        }
    }
}

////////////////////////////////////////////////
// Process user keystrokes
////////////////////////////////////////////////
int handleMenuKey(char key)
{
    switch (tolower(key)) /* Decode KEY ENTRY */
    {
    case 13: // Enter ("CR"), do nothing
    {
        printf("\r\n");
        printf(prompt);
        return 0;
    }
    case '?': // Display User interface help menu ("h")
    {
        if (getDutType() == dutTypes::EED)
        {
            printf(menuEED);
        }
        else
        {
            printf(menuMFD);
        }
        printf(prompt);
        return 0;
    }
    case '!':
    {
        // the Westar is easier to program if we don't have echo turned on
        echo = false;
        break;
    }
    case '*':
    {
        echo = true;
        break;
    }

    case '+':
    {
        int step = dut.getBacklightStep();
        step = (step < 255) ? step + 1 : 255;
        {
            dut.setBacklight(step);
        }
        if (echo)
            printf("Backlight Step: %d\r\n", step);
        break;
    }
    case '-': // Increase PWM by 25%
    {
        int step = dut.getBacklightStep();
        step = (step > 1) ? step - 1 : 1;
        {
            dut.setBacklight(step);
        }
        if (echo)
            printf("Backlight Step: %d\r\n", step);
        break;
    }
    case '5': // config command
    {
        configureDHABoard();
        printf(prompt);
        return 0;
    }
    case '6': // adjust heater settings and sense resistors
    {
        configureTestBoard();
        printf(prompt);
        return 0;
    }
    case 'a':
    {
        if (echo)
            printf("Heater Mode Auto\r\n");
        heaterMode = heaterControl::AUTO;
        break;
    }
    case 'b':
    {
        if (echo)
            printf("Enter Backlight Step (0:255):");
        dut.setBacklight(getInt(echo));
        break;
    }
    case 'c':
    {
        if (echo)
            printf("Calibration Mode\r\n");
        calibrationMode = 1;
        break;
    }
    case 'd': // disable Heater
    {
        if (echo)
            printf("Heater Disabled\r\n");
        heaterMode = heaterControl::OFF;
        dut.setHeater(false);
        break;
    }
    case 'e':
    {
        if (echo)
            printf("Current BIT :");
        printf("%d", bit.read());
        break;
    }
    case 'f':
    {
        dut.flush();
        break;
    }

    case 'h': // Enable Heater
    {
        heaterMode = heaterControl::ON;
        dut.setHeater(true);
        break;
    }
    case 'i':
    {
        dut.runIBIT();
        if (echo)
        {
            printStatus();
            printf(prompt);
        }
        return 0;
    }
    case 'l':
    {
        setLVDS();
        if (echo)
            printf("LVDS Mode\r\n");
        break;
    }
    case 'n':
    {
        calibrationMode = 0;
        if (echo)
            printf("Normal Mode\r\n");
        break;
    }
    case 'p':
    {
        if (echo)
            printf("BIT in progress:");
        printf("%d", bitInProgress.read());
        break;
    }
    case 'r':
    {
        setRS170();

        if (echo)
            printf("RS-170 Mode\r\n");
        break;
    }
    case 's':
    {
        dut.updateStatus();
        printStatus();
        printf(prompt);
        return 0;
    }
    case '$':
    {
        getSubStatus();
        break;
    }
    case 't':
    {
        if (echo)
            printf("DUT Type:");
        printf("%s;", getTestMode());
        return 0;
    }
    case 'v':
    {
        if (bezelLuminance != 0)
        {
            bezelLuminance = 0;
            if (echo)
                printf("Bezel Low V\r\n");
        }
        else
        {
            bezelLuminance = 1;
            if (echo)
                printf("Bezel High V\r\n");
        }
        break;
    }
    case 'y': // LCD Temperature (Deg C)
    {
        if (echo)
            printf("LCD Temperature Deg C: ");
        printf("%2.2f;", dut.getLCDTemp() - DEGC_TO_K);
        break;
    }
    default:
    {
        return key;
    }
    } // End switch case
    fflush(stdout);
    if (echo)
        printf(prompt);
    return 0;
}

////////////////////////////////////////////////
//
// Initialize the Board and then run the main loop
//
////////////////////////////////////////////////
int main()
{
    int ledOn = 0;
    stdio_init_all();


    // wait for tiny usb device to be connected (wait for host to oupe the usb port)
    //while (!tud_cdc_connected())
    //{
    //    sleep_ms(150);
    //    led1 = ledOn;
    //    ledOn = !ledOn;
    //}
    

     Tone buzzer(SPEAKER);
     buzzer.play(550,9);
     sleep_ms(1000);
     buzzer.stop(); 

    printf("USB Uart Connected()\n");

    absolute_time_t lastAutoStatusTime = get_absolute_time();
    programStartTime = get_absolute_time();

    uint8_t lastButtonsLeft = 0, lastButtonsRight = 0, lastButtonsUpper = 0, lastButtonsLower = 0, lastRockerSwitches = 0;

    // configure 5V AC input for bezel luminance
    float period = (1.0 / AC_HERTZ) * 1000.0;
    acPwm.period_ms(period);
    acPwm.write(0.5);


    // Configure Bezel configuration Pin 3 to drive high so
    // the bezel test box lights up.
    configPin3.write(1);

    // interrupt handler for bit discrete
    // treat bit error as interupt so we detect it immediatly
    // gpio_init(BIT_STATUS);
    gpio_set_irq_enabled_with_callback(BIT_STATUS, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &bitRise);

    sleep_ms(100);
    printf("BIT Interrupt Configured\r\n");

    // Autotest variables
    int bitErrorSeen = 0;
    iBitErrorCount = 0;
    
    bool autoTest = false;

    // Heater control variables
    heaterMode = heaterControl::OFF;

    add_repeating_timer_ms(100, heaterControlCallback, NULL, &heaterControlTicker);

    sleep_ms(1000);
    printf("\r\nIDC T-38 Test (Version %s)\r\n", RELEASE);

    //rs170ic.i2c_scan();
    // If we have the RS170 video test pattern running default to RS170,
    // otherwise default to LVDS
    if (internalRS170)
    {
        rs170ic.configureTestPatern();
        // if (rs170ic.checkForDevice())
        // {
        //     printf("\r\nRS-170 Test Pattern IC found.\r\n");
        //     rs170ic.configureTestPatern();
        // }
        // else
        // {
        //     printf("\r\nRS-170 Test Pattern IC NOT found.\r\n");
        // }
    }
    else
    {
        printf("Software Configured for external RS-170 Generator");
    }
    lvdsRs170 = 1;

#ifdef startWithTestPattern
    printf("Configuring MFD Internal Test Pattern (1Sec)\r\n");
    wait_us(1000000);
    testPatternOn = 1;
    dut.internalTestPattern(testPatternOn);

#endif

    printf(menuMFD);
    printf(prompt);

    int key = 0;

    while (1)
    {

        // loop runs once every 16.6mS

        key = 0;
        dut.processHeater(); // execute any heater command queued in isr

        key = getchar_timeout_us(16);
        if (key != PICO_ERROR_TIMEOUT) // handel keystroke from USB
        {

            if (handleMenuKey((char)key) == '7') // The 7 key toggles autotest
            {
                if (!autoTest)
                {
                    if (!echo)
                    {
                        printButtons(true, dutTypes::EED != getDutType());
                    }
                    else
                    {
                        printf("\033c"); // clear screen
                        printStatus();
                    }

                    bitErrorSeen = 0;
                    iBitErrorCount = 0;
                    autoTest = true;
                }
                else
                {
                    autoTest = false;
                    if (bitErrorSeen != 0)
                    {
                        printf("\r\n \033[31mAutotest Complete %d errors reset by IBIT\033[0m\r\n", iBitErrorCount);
                        for (int i = 0; (i < HISTORY_LENGTH && i < iBitErrorCount); i++)
                        {
                            for (int j = 0; j < 5; j++)
                            {
                                dut.printBIT(BITHistory[i], j);
                            }
                        }
                    }
                    else
                    {
                        printf("\r\n Autotest completed no bit errors reported\r\n");
                    }
                    printf(prompt);
                }
                // pc.sync();
            }
            else if (autoTest)
            {
                // we are in autotest mode but the user typed something 
                // clear the screen to get rid of any response text
                if (echo)
                {
                    printf("\033c"); // clear screen
                }
            }
        }
        led1=ledOn;  // flash the led whenever we request status
        ledOn = !ledOn;
        dut.updateStatus(); // get new status from DUT (does not print anything)
        
        handleBezelFunctions();
        sleep_us(16667); // Loop gets processed 60 times / sec
        // led1=0;

        if (autoTest)
        {

            if (!echo)
            { // when echo is off, we only print button status
                if (dut.currentStatus.bezelButtonsLeft != 0 ||
                    dut.currentStatus.bezelButtonsRight != 0 ||
                    dut.currentStatus.bezelButtonsUpper != 0 ||
                    dut.currentStatus.bezelButtonsLower != 0 ||
                    dut.currentStatus.rockerSwitches != 0)
                {
                    printButtons(false, dutTypes::EED != getDutType());
                }
            }
            else
            {
                // we get status every time through loop but only print it every sec or if a bezel button changes
                if ((float)absolute_time_diff_us(lastAutoStatusTime,get_absolute_time()) / 1000000.0f > 1.0f ||
                    dut.currentStatus.bezelButtonsLeft != lastButtonsLeft ||
                    dut.currentStatus.bezelButtonsRight != lastButtonsRight ||
                    dut.currentStatus.bezelButtonsUpper != lastButtonsUpper ||
                    dut.currentStatus.bezelButtonsLower != lastButtonsLower ||
                    dut.currentStatus.rockerSwitches != lastRockerSwitches)
                {
                    lastButtonsLeft = dut.currentStatus.bezelButtonsLeft;
                    lastButtonsRight = dut.currentStatus.bezelButtonsRight;
                    lastButtonsUpper = dut.currentStatus.bezelButtonsUpper;
                    lastButtonsLower = dut.currentStatus.bezelButtonsLower;
                    lastRockerSwitches = dut.currentStatus.rockerSwitches;
                    
                    uint8_t b = dut.currentStatus.bezelButtonsLeft |
                        dut.currentStatus.bezelButtonsRight |
                        dut.currentStatus.bezelButtonsUpper |
                        dut.currentStatus.bezelButtonsLower |
                        dut.currentStatus.rockerSwitches;
                    if (b)
                    {
                        buzzer.playNote(b*10, 250, 4); // beep to show we got a button press
                    }

                    // this stuff runs once a second
                    eraseStatus();
                    bitErrorSeen |= updateAutoTest();
                    printStatus();
                    printf("-- Press 7 to exit AutoTest Mode --");
                    lastAutoStatusTime = get_absolute_time();
                }
            }
        }
    }
}
