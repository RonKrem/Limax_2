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
#ifdef LED_SCREEN      
extern CLedScreen LedScreen;
#endif

extern CWellSimulator WellSimulator;
extern char OledText[];

extern xQueueHandle xStepQueue;
extern Flags Flag;
extern boolean LedState;
extern SemaphoreHandle_t xSDCardAccessMutex;

CTimers Timers;



//-------------------------------------------------------------------
//
CTimers::CTimers(void)
: mStepTimerState(NO_STEP_TIMER),
  mRecordIntervalTicks(0),
  mPlotIntervalTicks(0)
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

      // // The sensor samples are taken at whatever is the fastest of the two intervals,
      // // mPlotInterval or mRecordInterval. 
      // // Both times, however, are ticked at once.
      // //
      // Timers.StepPlotIntervalTicks();
      // Timers.StepRecordIntervalTicks();
      // if (Timers.GetRecordIntervalTicks() >= Timers.GetRecordInterval())
      // {
      //    // First reset the record ticks.
      //    //
      //    Timers.ClearRecordIntervalTicks();

      //    // Time to run a test and record it if a test is running.
      //    //
      //    if (Flag.SineTestRunning)
      //    {
      //       Flag.DoSample = true;
      //    }

      //    // and we schedule that the result of that sample will be
      //    // recorded on the sdcard.
      //    //
      //    Flag.ScheduleRecordEvent = true;
      // }

      // if (Timers.GetPlotIntervalTicks() >= Timers.GetPlotInterval())
      // {
      //    // First reset the record ticks.
      //    //
      //    Timers.ClearPlotIntervalTicks();

      //    // Always flag a sample for plotting.
      //    //
      //    Flag.DoSample = true;

      //    // and flag the sample result will be recorded.
      //    //
      //    Flag.SchedulePlotEvent = true;
      // }

      Timers.StepRecordIntervalTicks();
      if (Timers.GetRecordIntervalTicks() >= Timers.GetRecordInterval())
      {
         // First reset the record ticks.
         //
         Timers.ClearRecordIntervalTicks();

         Flag.DoSample = true;
      }

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
