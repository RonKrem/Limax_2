// Timers.cpp
//
#include "Timers.h"
#include "Motor.h"
#include "Data.h"
#include "ManageSamples.h"
#include "LimaxTime.h"
#include "TempTMP116.h"
#include "Ledscreen.h"

#include "WellSimulator.h"



//#define  PRINT_TIMERS
#ifdef PRINT_TIMERS
#define  P_TIME(x)   x;
#else
#define  P_TIME(x)   // x
#endif

extern Flags Flag;
extern CMotor Motor;
extern CData Data;
extern CManageSamples ManageSamples;
extern CLimaxTime LimaxTime;
extern CTempTMP116 tmp116;
extern CLedScreen LedScreen;

extern CWellSimulator WellSimulator;
extern char OledText[];

extern xQueueHandle xStepQueue;

extern boolean LedState;
extern SemaphoreHandle_t xSDCardAccessMutex;

CTimers Timers;



//-------------------------------------------------------------------
//
CTimers::CTimers(void)
: mIntervalTimerState(NO_INTERVAL_TIMER),
  mStepTimerState(NO_STEP_TIMER),
  mTestElapsedSeconds(0)
{
}

//-----------------------------------------------------------------------------
//
void CTimers::Init(void)
{
   CTimers* instance = static_cast<CTimers*>(this);

   if (instance->xStepTimer = xTimerCreate("Step", (1000 / portTICK_PERIOD_MS), pdTRUE, (void*)0, instance->StepTimerDone))
   {
      // The motor step timer always runs at one second intervals.
      //
      StartStepTimer(DO_STEP, 1000);

      // The inteval or sample time timer sets the user defined sample interval.
      //
      if (instance->xIntervalTimer = xTimerCreate("Interval", (BACKGROUND_SAMPLE_TIME / portTICK_PERIOD_MS), pdTRUE, (void*)0, instance->IntervalTimerDone))
      {
#ifdef SIMULATING       
         StartIntervalTimer(DO_SAMPLE, 5000);   // arbitrary
#else         
         StartIntervalTimer(DO_SAMPLE, BACKGROUND_SAMPLE_TIME);
#endif         
      }
   }
}

//-----------------------------------------------------------------------------
// This will change the inter-sample delay.
//
void CTimers::StartIntervalTimer(IntervalTimerType type, uint32_t delaymSecs)
{
   P_TIME(Serial.printf("StartIntervalTimer %d  %d\n", type, delaymSecs));
   mIntervalTimerState = type;
   Flag.DoSample = false;
   xTimerChangePeriod(xIntervalTimer, delaymSecs / portTICK_PERIOD_MS, 10);
}

//-----------------------------------------------------------------------------
// The sample interval time has expired. Here we need to recompute the delay
// time as the user may have made a change.
//
void CTimers::IntervalTimerDone(xTimerHandle pxTimer)
{
   uint32_t interval;
   String temperatureC;

   // A timer period has expired. What happens here depends
   // on what the <IntervalTimerState> has been set to.
   //
   P_TIME(Serial.printf("IntervalTimer done %d\n", Timers.GetIntervalTimerState()));
   switch (Timers.GetIntervalTimerState())
   {
   case NO_INTERVAL_TIMER:
      break;
   
   case DO_SAMPLE:
      // This interval timer runs continuously at the period set by a
      // call to StartIntervalTimer.
      //
      Flag.DoSample = true;                     // start this sample
      
#ifdef LIMAX_CONTROLLER      
      temperatureC = tmp116.ReadTemperature();
#endif      
//      sprintf(OledText, "Temp %.2f", temperatureC.toFloat());
//      LedScreen.WriteLine1(OledText);
      Data.PutDataEntryValue(DB_CONTROLLER_TEMP, temperatureC);

      break;

   default:
      break;
   }
}

//-----------------------------------------------------------------------------
//
void CTimers::StartStepTimer(StepTimerType type, uint32_t delayMSecs)
{
   P_TIME(Serial.printf("StartStepTimer %d  %d\n", type, delayMSecs));
   mStepTimerState = type;
   xTimerChangePeriod(xStepTimer, (delayMSecs / portTICK_PERIOD_MS), 10);
}

//-----------------------------------------------------------------------------
// A step timer interval has expired. The timer has automatically restarted
// its 200 Msec interval. Here we count up to one second and signal the 
// Hyperdrive to step and provide 200 mSec step signal as required.
//
void CTimers::StepTimerDone(xTimerHandle pxTimer)
{
   static uint8_t littleTick = 0;
   uint32_t message;

   // A timer period has expired. What happens here depends
   // on what the CurrentTimer has been set to.
   //
   switch (Timers.GetStepTimerState())
   {
   case NO_STEP_TIMER:
      break;
   
   case DO_STEP:

      // This timer ticks forever at a 1 second rate. This is the motor
      // step rate. For the time being, it also ticks a one second
      // clock (LimaxTime) that provides a standard clock for the system.
      //
      LimaxTime.StepLimaxTime();  // For the time being, this is the actual time clock

      Flag.BlinkLed = true;

      // Flash the LED at 1 sec intervals, it is on for 1 sec and off for 1 sec.
      //
      if (LedState)
      {
         digitalWrite(LED_PORT, LOW);
         LedState = false;
      }
      else
      {
         digitalWrite(LED_PORT, HIGH);
         LedState = true;
      }

      // Message the motor software Task_ProcessStepTimer with the one 
      // second tick. If the B_TEST_CONTROL button is pressed the
      // motor will run at the currently computed speed. The message
      // itself has no function.
      //
      if (xStepQueue)
      {
//         Serial.println("Sending to xStepQueue");
         if (xQueueSendToBack(xStepQueue, &message, portMAX_DELAY) != pdTRUE)
         {
            Serial.println("xStepQueue is full");
         }
      }
      break;

   default:
      break;
   }
}
