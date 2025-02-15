/*****************************************************************************

 NAME:         MotorControl.cpp

 DESCRIPTION:  Provides the Limax control interface to the Powerstep01 
               command set.


 AUTHOR:       Ron Kreymborg
               Kremford Pty Ltd

 REVISIONS:

******************************************************************************/
//
#include "MotorControl.h"
#include "Data.h"
#include "Motor.h"
#include "Buttons.h"
#include "Sensors.h"
#include "Timers.h"
//#include "Controls.h"


#define  QUICKLOOK_STEP_SIZE     300

#define  DEBUG_MTR_CNTRL
#ifdef DEBUG_MTR_CNTRL
#define  P_MCTRL(x)   x;
#else
#define  P_MCTRL(x)   // x
#endif

extern CData Data;
extern CMotor Motor;
extern TaskHandle_t ProcessStepTask;
extern uint32_t TestElapsedSeconds;
extern Flags Flag;
extern CMotorControl MotorControl;
extern CButtons Buttons;
extern CTimers Timers;
//extern CControls Controls;

extern xQueueHandle xStepQueue;

extern AsyncEventSource events;
//extern void StartStepTimer(StepTimerType type, uint32_t delayMSecs);
//extern uint32_t QuickLookStep;
extern uint32_t QuickLookTicks;
extern void NotifyClients(String state);

JSONVar jSonPlots;



//-------------------------------------------------------------------
//
CMotorControl::CMotorControl()
{
   mBoundariesDefined = false;
   mTemperature = 22.3;
   mSteps = 0;
   mAnalysisSteps = 0;
   mSlugDepth = 0;
   mSpeedMultiplier = 0;
   mPrevious = 0;
   if ((mSlugMutex = xSemaphoreCreateMutex()) == NULL)
   {
      Serial.println("No room for mSlugMutex");
   }
}

//-------------------------------------------------------------------
//
boolean CMotorControl::Init(void)
{
   CMotorControl* instance = static_cast<CMotorControl*>(this);

   mSpeedMultiplier = PI * Data.GetDataEntryNumericValue(DB_DRUM_DIA_MM) / Data.GetDataEntryNumericValue(DB_DRIVE_RATIO);
   P_MCTRL(Serial.printf("mSpeedMultiplier %f\n", mSpeedMultiplier));

   // Must be able to create the queue for incoming EspNow messages.
   //
   if (xStepQueue = xQueueCreate(3, sizeof(RXQuePacket)))
   {
      // Create the task that will manage the incoming step messages.
      //
      if (xTaskCreatePinnedToCore(  CMotorControl::Task_ProcessStepTimer, 
                                    "Step", 
                                    3000, 
                                    NULL, 
                                    2, 
                                    &ProcessStepTask,
                                    0) != pdPASS)
      {
         Serial.println("Cannot create task Task_ProcessReceivedPacket");
         return false;
      }
   }
   
   // This timer is setup to be one-shot that will delay for the given time. 
   // The intent is to provide a timed gap between the profile display events.
   //
   mProfileTimerState = PROFILE_NOTHING;    // reset
   if (instance->xProfileIntervalTimer = xTimerCreate("ProTimer", pdMS_TO_TICKS(PROFILE_INTERVAL_TIME), pdTRUE, (void*)0, instance->TimerDone))
   {
      return true;
   }

   return false;
}

//-------------------------------------------------------------------
void CMotorControl::ComputeParameters(void)
{
   mSpeedMultiplier = PI * Data.GetDataEntryNumericValue(DB_DRUM_DIA_MM) / Data.GetDataEntryNumericValue(DB_DRIVE_RATIO);
   mParkingSpeedRPS = 1000.0 * PARKING_SPEED_MPM / 60.0 / mSpeedMultiplier;
   mParkingIncrement = mParkingSpeedRPS * 60 / 1000;
   Serial.printf("mSpeedMultiplier= %f, mParkingSpeedRPS= %f, mParkingIncrement= %f\n", mSpeedMultiplier, mParkingSpeedRPS, mParkingIncrement);
   mSlugDepth = 0;
   Flag.ReturnToZero = false;
   mTestSeconds = 0;
}

//-------------------------------------------------------------------
// Set normal accel/decel. Will not work if motor is running.
//
void CMotorControl::SetNormalAccelRate(void)
{
   Motor.SetAccelRate(NORMAL_ACCEL_RATE);
   Motor.SetDecelRate(NORMAL_ACCEL_RATE);
}

//-------------------------------------------------------------------
// Set slow accel/decel. Will not work if motor is running.
//
void CMotorControl::SetSlowAccelRate(void)
{
   Motor.SetAccelRate(SLOW_ACCEL_RATE);
   Motor.SetDecelRate(SLOW_ACCEL_RATE);
}

//-------------------------------------------------------------------
// Return the actual elapsed test seconds.
//
uint32_t CMotorControl::GetElapsedSeconds(void)
{
   return mTestSeconds - mSubStart;
}

//-------------------------------------------------------------------
// This process managers the stepper motor speed control update that
// while running occurs every second.
//
void CMotorControl::Task_ProcessStepTimer(void* parameter)
{
   uint32_t message;

   while (true)
   {
      // Wait for an incoming message from the one second timer tick.
      //
      xQueueReceive(xStepQueue, (uint32_t*)&message, portMAX_DELAY);
//      Serial.println("xStepQueue message received");

      // We arrive here every second. Check if we should be running.
      //
      if (Flag.SineTestRunning)
      {
         MotorControl.ManageSineTest();
      }
      else if (Flag.SweepTestRunning)
      {
         MotorControl.ManageSweepTest();
      }
   }

   vTaskDelete(NULL);
}

//-------------------------------------------------------------------
//
void CMotorControl::PutSlugDepth(float position)
{
   xSemaphoreTake(mSlugMutex, portMAX_DELAY);
   mSlugDepth = position;
   xSemaphoreGive(mSlugMutex);
//   P_MCTRL(Serial.printf("PutSlugDepth: %f\n", mSlugDepth));
}

//-------------------------------------------------------------------
//
float CMotorControl::GetSlugDepth(void) const
{
   float value;

   xSemaphoreTake(mSlugMutex, portMAX_DELAY);
   value = mSlugDepth;
   xSemaphoreGive(mSlugMutex);
   P_MCTRL(Serial.printf(" SlugDepth = %f\n", value));

   return value;
}

//-----------------------------------------------------------------------------
// Call this function to initiate a profile plot.
// 
void CMotorControl::BeginProfileTimer(void)
{
   xTimerChangePeriod(xProfileIntervalTimer, PROFILE_INTERVAL_TIME, 0);
   mProfileTimerState = PROFILE_EVENT;
   P_MCTRL(Serial.println("MotorControl: BeginProfileTimer"));
}

//-----------------------------------------------------------------------------
// We arrive here after the simulated sensor response timeout.
//
void CMotorControl::TimerDone(xTimerHandle pxTimer)
{
   switch (MotorControl.GetProfileTimerState())
   {
         break;

      case PROFILE_EVENT:
         // Ready to send the next profile position.
         //
Serial.println("PROFILE_EVENT");
         MotorControl.DisplaySineProfile();
         break;

      case PROFILE_NOTHING:
      default:
         break;
   }
}

//-------------------------------------------------------------------
//
void CMotorControl::StartProfileTimer(void)
{
   P_MCTRL(Serial.println("MotorControl.StartProfileTimer"));

   DefineTestBoundaries(  DB_SINE_RUNTIME_MINS, 
                          Buttons.GetBooleanState(B_SINE2_ENABLE), 
                          Buttons.GetBooleanState(B_SINE3_ENABLE) );
   
   // Profile events occur every PROFILE_INTERVAL_TIME milliseconds. This interval
   // is less than the resolution of an uint32_t, so the mProfileElapsedTime
   // is in milleseconds.
   //
   mProfileElapsedTime = mSubStart;
   mMaxProfileTime = (uint32_t)Data.GetDataEntryNumericValue(DB_SINE_RUNTIME_MINS) * 60 + mProfileElapsedTime;
   P_MCTRL(Serial.printf("mProfileElapsedTime %d  mMaxProfileTime %d\n", mProfileElapsedTime, mMaxProfileTime));
   mProfileTimerState = PROFILE_NOTHING;    // reset

   BeginProfileTimer();
}

//-------------------------------------------------------------------
//
void CMotorControl::DisplaySineProfile(void)
{
   boolean sine2, sine3;
   uint32_t seconds;
   float depth;
   String jsonString;
   char text[50];

   P_MCTRL(Serial.println("MotorControl.DisplaySineProfile"));

   // Compute the current slug depth.
   //
   sine2 = Buttons.GetBooleanState(B_SINE2_ENABLE);
   sine3 = Buttons.GetBooleanState(B_SINE3_ENABLE);
   seconds = mProfileElapsedTime;
   mDepth = (NewSum(seconds, sine2, sine3) - mMax) * mScale;
//   Serial.printf("seconds %d  depth %f\n", seconds, mDepth);   

   sprintf(text, "%s", SecondsToTime(seconds).c_str());
   jSonPlots["time"] = String(text);
   sprintf(text, "%.5f", mDepth);
//   Serial.printf("<%s>\n", text);
   jSonPlots["slug"] = String(text);
   jsonString = JSON.stringify(jSonPlots);
//   Serial.println(jsonString);
   events.send(jsonString.c_str(), "new_readings", millis());

   mProfileElapsedTime++; // += PROFILE_INTERVAL_TIME;
//   Serial.println(mProfileElapsedTime);

   if (mProfileElapsedTime > mMaxProfileTime)
   {
      mProfileTimerState = PROFILE_NOTHING;    // reset
      xTimerStop(xProfileIntervalTimer, 0);
      Buttons.SetButtonState(B_SINE_QUICKLOOK, "0");     // turn button off
      Buttons.ButtonOff(B_SINE_QUICKLOOK);
   }

   // // Move the QuickLookStep value to the next value within the 
   // // chosen period.
   // //
   // QuickLookStep += ((int)Data.GetDataEntryNumericValue(DB_SINE_RUNTIME_MINS) * 60) / QUICKLOOK_STEP_SIZE;

   // if (QuickLookStep > GetSubEndValue())
   // {
   //    Flag.DoSineQuicklook = false;

   //    Buttons.SetButtonState(B_SINE_QUICKLOOK, "0");     // turn button off
   //    Buttons.ButtonOff(B_SINE_QUICKLOOK);
   // }
}

// //-------------------------------------------------------------------
// //
// void CMotorControl::DisplaySweepProfile(void)
// {
//    float depth, difference, revsPerSec;
//    String jsonString;
//    JSONVar values;
//    char text[20];

//    P_MCTRL(Serial.println("MotorControl.DisplaySweepProfile"));

//    // Compute the current slug depth.
//    //
//    mDepth = cos(2.0 * PI * QuickLookStep * (mR * QuickLookStep / 2.0 + mf0));
//    mDepth = mDepth / 2.0 - 0.5;    // translate to negative
//    mDepth *= Data.GetDataEntryNumericValue(DB_SWEEP_DEPTH);

//    sprintf(text, "%.5f", mDepth);
//    values["slug"] = String(text);
//    values["time"] = SecondsToTime(QuickLookStep - GetSubStartValue());

//    jsonString = JSON.stringify(values);
//    events.send(jsonString.c_str(), "new_readings", millis());

//    // Move the QuickLookStep value to the next value within the 
//    // chosen period.
//    //
//    QuickLookStep += ((int)Data.GetDataEntryNumericValue(DB_SWEEP_RUNTIME_MINS) * 60) / QUICKLOOK_STEP_SIZE;

//    if (QuickLookStep > GetSubEndValue())
//    {
//       Flag.DoSweepQuicklook = false;

//       jsonString = "0";
//       Buttons.SetButtonState(B_SWEEP_QUICKLOOK, jsonString);
//       NotifyClients(Buttons.GetButtonStates());
//    }
// }

//-------------------------------------------------------------------
//
String CMotorControl::SecondsToTime(uint32_t seconds)
{
   String time;
   uint32_t hours, minutes, secs;
   char text[20];

   hours = seconds / 3600;
   minutes = (seconds - 3600 * hours) / 60;
   secs = seconds % 60;
   sprintf(text, "%d:%02d:%02d", hours, minutes, seconds % 60);
//   P_MCTRLSerial.printf("%d  %d %d %d  %s\n", seconds,  hours, minutes, secs, text));
   time = String(text);
   return time;
}

//-----------------------------------------------------------------------------
// Manage a running plain sine test.
//
void CMotorControl::ManageSineTest(void)
{
   // The SineTestRunning flag is set. Check if the test elapsed time
   // has finished.
   //
   P_MCTRL(Serial.println("ManageSineTest"));

   if (Flag.ReturnToZero)
   {
      DoParking();
   }
   else if (mTestSeconds > (int)Data.GetDataEntryNumericValue(DB_SINE_RUNTIME_MINS) * 60)
   {
      P_MCTRL(Serial.println("StopSineTest"));

      Flag.Recording = false;    // stop recording

      DoParking();
   }
   else
   {
      // We arrive here every second while the SineTestRunning flag is set 
      // and the test time has not concluded. Because the test parameters 
      // may have changed the test boundaries must be redefined for every case. 
      ////
      if (MotorControl.AreBoundariesDefined() == false)
      {
         // Test boundaries must be re-defined.
         //
         P_MCTRL(Serial.println("Defining boundaries"));
         MotorControl.DefineTestBoundaries(  DB_SINE_RUNTIME_MINS, 
                                             Buttons.GetBooleanState(B_SINE2_ENABLE), 
                                             Buttons.GetBooleanState(B_SINE3_ENABLE) );
                  
         vTaskDelay(1);

         // The mTestSeconds varuable contains the current elapsed test seconds offset
         // by the computed correct start position computed by DefineTestBoaundaries.
         // 
         mTestSeconds = mSubStart;
      }

      // Compute a new slug depth every second.
      //
      MotorControl.ComputeSineDepth();
   }
}

//-----------------------------------------------------------------------------
// While ever the Flag.SweepTestRunning is set, the 1 second timer will call 
// this method. Here we first check the elapsed time against the user setting,
// and if exceeded, stop the test. 
//
void CMotorControl::ManageSweepTest(void)
{
   float revsPerSec;
   static float working;

   // The SweepTestRunning flag is still set. Check if the sweep test elapsed
   // time has finished.
   //
   Serial.println("ManageSweepTest");
   if (Flag.ReturnToZero)
   {
      DoParking();
   }
   else if (mTestSeconds > (int)Data.GetDataEntryNumericValue(DB_SWEEP_RUNTIME_MINS) * 60)
   {
      P_MCTRL(Serial.println("StopSweepTest"));

      Flag.Recording = false;    // stop recording

      DoParking();
   }
   else
   {
      // We arrive here every second while ever the SweepTestRunning button is
      // set and the test time has not concluded. The test boundaries must be 
      // redefined if any parameters are redefined. 
      //
      if (MotorControl.AreBoundariesDefined() == false)
      {
         // Test boundaries must be re-defined.
         //
         P_MCTRL(Serial.println("Defining boundaries"));
         MotorControl.DefineTestBoundaries(DB_SWEEP_RUNTIME_MINS, false, false);
                  
         vTaskDelay(1);

         // The mTestSeconds varuable contains the current elapsed test seconds offset
         // by the computed correct start position computed by DefineTestBoaundaries.
         // 
         mTestSeconds = mSubStart;
      }

      // Compute a new slug depth every second.
      //
      MotorControl.ComputeSweepDepth();
   }
}

//-----------------------------------------------------------------------------
//
void CMotorControl::StopTest(void)
{
   Serial.println("MotorControl.StopTest");
   Flag.SineTestRunning = Flag.SweepTestRunning = false;     // flag as no tests running

   // Stop the motor.
   //
   if (Flag.MotorRunning)
   {
      Motor.SoftStop();
      Motor.BrakePowerOff();
      Flag.MotorRunning = false;
   }
   Flag.SineTestRunning = Flag.SweepTestRunning = false;

   mPrevious = 0;       // ensure we start from zero

   // Turn both start buttons off.
   //
   Buttons.SetButtonState(B_SWEEP_CONTROL, "0");     // turn button off
   Buttons.ButtonOff(B_SWEEP_CONTROL);

   Buttons.SetButtonState(B_TEST_CONTROL, "0");     // turn button off
   Buttons.ButtonOff(B_TEST_CONTROL);

   // Now undefine the test parameters.
   //
   MotorControl.ClearBoundariesDefined();
}

//-----------------------------------------------------------------------------
//
void CMotorControl::DoParking(void)
{
   float revsPerSec;

   if (Flag.ReturnToZero == false)
   {
      Flag.ReturnToZero = true;
      revsPerSec = mParkingSpeedRPS;
      Flag.MotorRunning = true;
      Motor.RunMotorAtRevsPerSec(revsPerSec);
      Motor.EnergiseBrake();
   }
   else if (mSlugDepth < -PARK_SHUTDOWN_DISTANCE)
   {
      P_MCTRL(Serial.printf("mSlugDepth %f\n", mSlugDepth)); 
      mSlugDepth += mParkingIncrement;
   }
   else
   {
      mSlugDepth = 0;
      mPrevious = 0;
      StopTest();
      Flag.ReturnToZero = false;
   }
}

//-----------------------------------------------------------------------------
// This computes the new depth then translates that into the required
// motor speed.
//
void CMotorControl::ComputeSineDepth(void)
{
   float working, difference, revsPerSec;
   boolean sine2, sine3;

   P_MCTRL(Serial.printf("MotorControl.ComputeSineDepth: %d  %d  %d\n", Flag.DoSineQuicklook, Flag.SineTestRunning, Flag.SweepTestRunning));

   if (Flag.SineTestRunning)
   {
      sine2 = Buttons.GetBooleanState(B_SINE2_ENABLE);
      sine3 = Buttons.GetBooleanState(B_SINE3_ENABLE);

      // Compute the current slug depth.
      //
      working = (NewSum(mTestSeconds, sine2, sine3) - mMax) * mScale;

      // Compute the difference in millimetres.
      //
      difference = (working - mPrevious) * 1000.0;
      mPrevious = working;

      // Convert to revs/sec.
      //
      revsPerSec = difference / mSpeedMultiplier;

      P_MCTRL(Serial.printf("Secs %d  RPS %f\n", mTestSeconds, revsPerSec));

      // Motor only runs for the active tests.
      //
      if (Flag.SineTestRunning)
      {
         // Ensure the motor is flagged as running.
         //
         Flag.MotorRunning = true;

         // Run the motor at the current speed.
         //
         Motor.RunMotorAtRevsPerSec(revsPerSec);

         Motor.EnergiseBrake();
      }
   }

   // Publish the current depth.
   //
   PutSlugDepth(working);

   // Step elapsed seconds.
   //
   mTestSeconds++;
}

//-----------------------------------------------------------------------------
// The B_SWEEP_CONTROL has just been pressed. Initialise the required 
// sweep parameters.
//
void CMotorControl::InitSweep(void)
{
   Serial.println("MotorControl.InitSweep");
   mf0 = 1.0 / (60.0 * Data.GetDataEntryNumericValue(DB_SWEEP_START));
   mf1 = 1.0 / (60.0 * Data.GetDataEntryNumericValue(DB_SWEEP_STOP));
   mTime = Data.GetDataEntryNumericValue(DB_SWEEP_RUNTIME_MINS) * 60.0;
   mR = (mf1 - mf0) / mTime;
   mPrevious = 0;
   P_MCTRL(Serial.printf("f0=%f, f1=%f, Time=%f, R=%f\n", mf0, mf1, mTime, mR));
}

//-----------------------------------------------------------------------------
//
void CMotorControl::ComputeSweepDepth(void)
{
   float difference;
   float revsPerSec;
   float working;

   working = cos(2.0 * PI * mTestSeconds * (mR * mTestSeconds / 2.0 + mf0));
   working = working / 2.0 - 0.5;    // translate to negative
   working *= Data.GetDataEntryNumericValue(DB_SWEEP_DEPTH);

   // The difference in millimetres.
   //
   difference = (mPrevious - working) * 1000;
   mPrevious = working;

   revsPerSec = difference / mSpeedMultiplier;

   P_MCTRL(Serial.printf("Secs %d  RPS %f\n", mTestSeconds, revsPerSec));

   if (Flag.SweepTestRunning)
   {
      // Ensure the motor is flagged as running.
      //
      Flag.MotorRunning = true;

      Motor.RunMotorAtRevsPerSec(revsPerSec);

      Motor.EnergiseBrake();
   }

   // Publish the current depth.
   //
   PutSlugDepth(working);

   // Step elapsed seconds.
   //
   mTestSeconds++;
}

//-----------------------------------------------------------------------------
// Takes the slug profile variables from the basicsine.html page and creates
// the bounding parameters that define the slug travel profile.
//
void CMotorControl::DefineTestBoundaries(uint32_t runtimeIndex, boolean sine2, boolean sine3)
{
   uint32_t index;
   float oldmMax;
   boolean proceed;
   uint32_t i, j, n;
   float current, temp, r;
//   boolean sine2, sine3;

   P_MCTRL(Serial.println("MotorControl:DefineTestBoundaries"));

   mSteps = (uint32_t)Data.GetDataEntryNumericValue(runtimeIndex) * 60;   // test elapsed time in seconds
   mAnalysisSteps = mSteps * 4 + 1;

   P_MCTRL(Serial.printf("Steps %d  mAnalysisSteps %d\n", mSteps, mAnalysisSteps));
   mDepth = Data.GetDataEntryNumericValue(DB_DEPTH_MS);
   P_MCTRL(Serial.printf("Depth %d\n", mDepth));

   // Copy the three phase offsets.
   //
   mPhase1 = 0; //(double)Data.GetDataEntryNumericValue(DB_PERIOD1_DEGS);
   if (sine2) mPhase2 = (double)Data.GetDataEntryNumericValue(DB_PERIOD2_DEGS); else mPhase2 = 0;
   if (sine3) mPhase3 = (double)Data.GetDataEntryNumericValue(DB_PERIOD3_DEGS); else mPhase3 = 0;

   mAmplitude1 = (double)Data.GetDataEntryNumericValue(DB_PERIOD1_CMS);
   if (sine2) mAmplitude2 = (double)Data.GetDataEntryNumericValue(DB_PERIOD2_CMS); else mAmplitude2 = 0;
   if (sine3) mAmplitude3 = (double)Data.GetDataEntryNumericValue(DB_PERIOD3_CMS); else mAmplitude3 = 0;

   mDepth = (double)Data.GetDataEntryNumericValue(DB_DEPTH_MS);
   P_MCTRL(Serial.printf("Amplitudes %f %f %f %f\n", mDepth, mAmplitude1, mAmplitude2, mAmplitude3));

   // // Ensure no phase angle is allowed for sine1 if used alone.
   // //
   // if ( (Buttons.GetBooleanState(B_SINE2_ENABLE) == false) && (Buttons.GetBooleanState(B_SINE3_ENABLE) == false) ) 
   // {
   //    mPhase1 = 0.0;
   //    Data.PutDataEntryValue(DB_PERIOD1_DEGS, "0");
   // }

   // Convert the three sine periods from minutes to seconds.
   //
   mPeriod1 = (int)Data.GetDataEntryNumericValue(DB_PERIOD1_MINS) * 60.0;
   if (sine2) mPeriod2 = (int)Data.GetDataEntryNumericValue(DB_PERIOD2_MINS) * 60.0; else mPeriod2 = 0;
   if (sine3) mPeriod3 = (int)Data.GetDataEntryNumericValue(DB_PERIOD3_MINS) * 60.0; else mPeriod3 = 0;
   P_MCTRL(Serial.printf("Periods %f %f %f\n", mPeriod1, mPeriod2, mPeriod3));

   // Convert the periods to angular frequency.
   //
   P_MCTRL(Serial.printf("Phases %f %f %f\n", mPhase1, mPhase2, mPhase3));

   mMin = 0.0;
   mMax = -1000.0;
   oldmMax = mMax;
   mSubStart = 0;
   mSubEnd = 0;
   
   j = 0;
   index = 0;
   proceed = true;
   while (proceed)
   {
      // Search for a new mMax larger than the last in the sample from time j.
      //
      Serial.println("Cycle ====");
      i = j;
      n = i + mSteps;
      P_MCTRL(Serial.printf("i = %d,  n = %d\n", i, n));
      if (n > mAnalysisSteps)
      {
         break;   // enough searching
      }
      while (i < n)
      {
         current = NewSum(i, sine2, sine3);
         if (current > mMax)
         {
            index = i;
            mMax = current;
         }

         if (current < mMin)
         {
            mMin = current;
         }

         i++;
      }

      P_MCTRL(Serial.printf("index = %d,  mMin = %f, mMax = %f,  current = %f\n", index, mMin, mMax, current));

      if (fabs(mMax - oldmMax) < 0.1)
      {
         proceed = false;
      }
      else
      {
         oldmMax = mMax;
         mMax = -1000;
         j = index;
      }
   }

   P_MCTRL(Serial.printf("mMin %f  mMax %f  index %d\n", mMin, mMax, index));
   mSubStart = index;
   mSubEnd = index + mSteps;
   P_MCTRL(Serial.printf("mDepth = %f, SubStart = %d,  SubEnd = %d,  current = %f\n", mDepth, mSubStart, mSubEnd, current));

   mScale = mDepth / (mMax - mMin);
   P_MCTRL(Serial.printf("mScale = %f\n", mScale));

   mBoundariesDefined = true;
}

//-------------------------------------------------------------------
//
boolean CMotorControl::MotorActionParameters(int index)
{
   boolean reply = true;

   switch (index)
   {
      case DB_ACCEL_POWER:    // accel pwr
         Motor.SetAccelPwr((int)Data.GetDataEntryNumericValue(index));
         break;

      case DB_DECEL_POWER:    // decel pwr
         Motor.SetDecelPwr((int)Data.GetDataEntryNumericValue(index));
         break;

      case DB_RUN_POWER:    // run pwr
         Motor.SetRunPwr((int)Data.GetDataEntryNumericValue(index));
         break;

      case DB_STOP_POWER:    // stop pwr
         Motor.SetStoppedPwr((int)Data.GetDataEntryNumericValue(index));
         break;

      case DB_DRUM_DIA_MM:    // drive roller dia
         break;

      case DB_DRIVE_RATIO:    // drive ratio
         break;

      case DB_CONTROLLER_TEMP:
         break;

      case DB_MOTOR_LIFT_RPM:
         // Convert RPM to SPS the set speed.
         //     
         Motor.SetWinchSpeedRPM((int)Data.GetDataEntryNumericValue(DB_MOTOR_LIFT_RPM));
         break;

      default:
         reply = false;    // not processed here
         break;
   }

   return reply;
}

//-------------------------------------------------------------------
//
float CMotorControl::DriveTemperature(void)
{
   return mTemperature;
}

//-------------------------------------------------------------------
// Must call DefineTestBoundaries before using.
//
float CMotorControl::ComputeSlugPosition(uint32_t index)
{	
   float amplitude;
   boolean sine2, sine3;

   // Compute a new sum of sines term.
   sine2 = Buttons.GetBooleanState(B_SINE2_ENABLE);
   sine3 = Buttons.GetBooleanState(B_SINE3_ENABLE);
   amplitude = (NewSum(index, sine2, sine3) - mMax) * mScale;

   return amplitude;
}

//-----------------------------------------------------------------------------
// This sum uses the algorithm described in the Kremford paper:
//     The Limax Control Development. 
//
float CMotorControl::NewSum(uint32_t seconds, boolean sine2, boolean sine3)
{
   float temp;
   float cosine1, cosine2, cosine3;

   cosine1 = CoSine(seconds, mPeriod1, mPhase1, mAmplitude1);

   if (sine2)
   {
      cosine2 = CoSine(seconds, mPeriod2, mPhase2, mAmplitude2);
   }
   else
   {
      cosine2 = 0.0;
   }

   if (sine3)
   {
      cosine3 = CoSine(seconds, mPeriod3, mPhase3, mAmplitude3);
   }
   else
   {
      cosine3 = 0.0;
   }

   temp = (cosine1 + cosine2 + cosine3);

   return temp;
}

//-----------------------------------------------------------------------------
// Compute the required sine. 
//
float CMotorControl::CoSine(float seconds, float period, float phase, float amplitude) const
{
   return cos((2.0 * seconds / period + phase / 180.0) * PI) / 2.0 * amplitude;
}