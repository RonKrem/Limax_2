// Main.h
//
#ifndef  _MAIN_H
#define  _MAIN_H

#include <Arduino.h>
#include <math.h>
#include <ElegantOTA.h>
#include <SPI.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <AsyncEventSource.h>
#include <Arduino_JSON.h>
#include <LittleFS.h>
#include <esp_now.h>
#include <SparkFun_Qwiic_OLED.h>
#include <res/qw_fnt_8x16.h>
#include <SD_MMC.h>
#include "Ledscreen.h"


//#define  LIMAX_CONTROLLER
//#define  LED_SCREEN
#define  SIMULATING

// New line

#define  BRAKE_PORT              27
#define  LED_PORT                25

#define  DATA_FOLDER             "/LimaxData"         // user CSV file
#define  DISPLAY_DATA_FOLDER     "/DisplayData"       // temporary data folder
#define  TEMP_DATA_FILENAME      "/DisplayData/temp"  // the internal dynamic data file
#define  DISPLAY_SAMPLES         40

#define  MAX_PATHNAME_LENGTH     40    // used in SetupDatabase

#define  SDCARD_ACCESS_MUTEX     1500   // mSec

// Configuration for Sparkfun ESP32 Datalogger IOT
// The theSPI connections are:
//   SDcard SDO (Serial Data Out) to ESP32 CIPO (Controller In Peripheral Out)
//   SDcard SDI (Serial Data In)  to ESP32 COPI (Controller Out Peripheral In)
//
#define  STEPPER_SCK             18    // Stepper spi clock
#define  STEPPER_COPI            23    // Stepper MOSI (Master Out Slave In) to ESP32 PICO (Peripheral In Controller Out)
#define  STEPPER_CIPO            19    // Stepper MISO from ESP32 POCI
#define  STEPPER_CS              5     // Stepper select (low)
#define  STEPPER_RESET           33    // Stepper reset

#define  BACKGROUND_SAMPLE_TIME  10

#define  MOTOR_STEPS_PER_REV     200
#define  PARKING_SPEED_MPM       3     // parking speed in metres/minute
#define  NORMAL_ACCEL_RATE       100
#define  SLOW_ACCEL_RATE         5
#define  PARK_SHUTDOWN_DISTANCE  0.01     // below park position

#define  PROFILE_INTERVAL_TIME   1000       // millisecs between profile plot events

#define  SAMPLE_QUEUE_SIZE       7

#define  LO_PRIORITY             2
#define  MD_PRIORITY             3
#define  HI_PRIORITY             4

//#define  SPIFFS                  LittleFS


//-----------------------------------------------------------------------------
//
typedef struct
{
   uint32_t DoSine1              : 1;
   uint32_t DoSine3              : 1;
   uint32_t OnSetup              : 1;
   uint32_t DoSample             : 1;
   uint32_t StartLedFlasher      : 1;
   uint32_t SineTestRunning      : 1;
   uint32_t SweepTestRunning     : 1;
   uint32_t MotorRunning         : 1;
   uint32_t DoSineQuicklook      : 1;
   uint32_t DoSweepQuicklook     : 1;
   uint32_t DoQuickLookStep      : 1;
   uint32_t Tick200msecs         : 1;
   uint32_t OledExists           : 1;
   uint32_t BlinkLed             : 1;
   uint32_t RePlot               : 1;  
   uint32_t UseLedScreen         : 1;
   uint32_t ReturnToZero         : 1;
   uint32_t Recording            : 1;
   // uint32_t DoRecordEvent        : 1;
   // uint32_t DoPlotEvent          : 1;
   // uint32_t SchedulePlotEvent    : 1;
   // uint32_t ScheduleRecordEvent  : 1;
   // uint32_t SampleRunning        : 1;

} Flags;


typedef enum
{
   NO_INTERVAL_TIMER,
   DO_SAMPLE,
} IntervalTimerType;

typedef enum
{
   NO_STEP_TIMER,
   DO_STEP,
   DO_QUICKLOOK,
} StepTimerType;

enum
{
   SENSOR_1,
   SENSOR_2,
   SENSOR_3,
};


#endif   // _MAIN_H