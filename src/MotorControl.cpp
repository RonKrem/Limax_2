// Motor drive
//
#include "MotorControl.h"
#include "Data.h"
#include "Motor.h"
#include "Buttons.h"
#include "Sensors.h"
#include "Timers.h"
//#include "Controls.h"


#define  QUICKLOOK_STEP_SIZE     300

//#define  DEBUG_MTR_CNTRL
#ifdef DEBUG_MTR_CNTRL
#define  P_MTRCTRL(x)   x;
#else
#define  P_MTRCTRL(x)   // x
#endif

//extern void StartIntervalTimer(IntervalTimerType type, uint32_t delaymSecs);

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
extern uint32_t QuickLookStep;
extern uint32_t QuickLookTicks;
extern void NotifyClients(String state);



//-------------------------------------------------------------------
//
CMotorControl::CMotorControl()
{
   mBoundariesDefined = false;
   mTemperature = 22.3;
   mPhaseScale = (PI * 2.0 / 360.0);
   mSteps = 0;
   mSlugDepth = 0;
   mOneDrumTurnInCms = 0;
   mPrevious = 0;
   if ((mSlugMutex = xSemaphoreCreateMutex()) == NULL)
   {
      Serial.println("No room for mSlugMutex");
   }
}

//-------------------------------------------------------------------
//
void CMotorControl::Init(void)
{
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
      }
   }
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

      if (Flag.SweepTestRunning)
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
   Serial.printf("PutSlugDepth: %f\n", mSlugDepth);
   xSemaphoreGive(mSlugMutex);
}

//-------------------------------------------------------------------
//
float CMotorControl::GetSlugDepth(void) const
{
   float value;

   xSemaphoreTake(mSlugMutex, portMAX_DELAY);
   value = mSlugDepth;
//   Serial.printf(" SlugDepth = %f\n", value);
   xSemaphoreGive(mSlugMutex);

   return value;
}

//-------------------------------------------------------------------
//
void CMotorControl::DisplaySineProfile(void)
{
   float depth, difference, revsPerSec;
   boolean sine2, sine3;
   String jsonString;
   JSONVar values;
   char text[20];

   P_MTRCTRL(Serial.println("MotorControl.DisplaySineProfile"));
   sine2 = Buttons.GetBooleanState(B_SINE2_ENABLE);
   sine3 = Buttons.GetBooleanState(B_SINE3_ENABLE);

   // Compute the current slug depth.
   //
   mDepth = (NewSum(QuickLookStep, sine2, sine3) - mMax) * mScale;

   sprintf(text, "%.5f", mDepth);
   values["slug"] = String(text);
   values["time"] = SecondsToTime(QuickLookStep - GetSubStartValue());

   jsonString = JSON.stringify(values);
   events.send(jsonString.c_str(), "new_readings", millis());

   // Move the QuickLookStep value to the next value within the 
   // chosen period.
   //
   QuickLookStep += ((int)Data.GetDataEntryNumericValue(DB_SINE_RUNTIME_MINS) * 60) / QUICKLOOK_STEP_SIZE;

   if (QuickLookStep > GetSubEndValue())
   {
      Flag.DoSineQuicklook = false;

      jsonString = "0";
      Buttons.SetButtonState(B_SINE_QUICKLOOK, jsonString);
      NotifyClients(Buttons.GetButtonStates());
   }
}

//-------------------------------------------------------------------
//
void CMotorControl::DisplaySweepProfile(void)
{
   float depth, difference, revsPerSec;
   String jsonString;
   JSONVar values;
   char text[20];

   P_MTRCTRL(Serial.println("MotorControl.DisplaySweepProfile"));

   // Compute the current slug depth.
   //
   mDepth = cos(2.0 * PI * QuickLookStep * (mR * QuickLookStep / 2.0 + mf0));
   mDepth = mDepth / 2.0 - 0.5;    // translate to negative
   mDepth *= Data.GetDataEntryNumericValue(DB_SWEEP_DEPTH);

   sprintf(text, "%.5f", mDepth);
   values["slug"] = String(text);
   values["time"] = SecondsToTime(QuickLookStep - GetSubStartValue());

   jsonString = JSON.stringify(values);
   events.send(jsonString.c_str(), "new_readings", millis());

   // Move the QuickLookStep value to the next value within the 
   // chosen period.
   //
   QuickLookStep += ((int)Data.GetDataEntryNumericValue(DB_SWEEP_RUNTIME_MINS) * 60) / QUICKLOOK_STEP_SIZE;

   if (QuickLookStep > GetSubEndValue())
   {
      Flag.DoSweepQuicklook = false;

      jsonString = "0";
      Buttons.SetButtonState(B_SWEEP_QUICKLOOK, jsonString);
      NotifyClients(Buttons.GetButtonStates());
   }
}

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
//   Serial.printf("%d  %d %d %d  %s\n", seconds,  hours, minutes, secs, text);
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
   Serial.println("ManageSineTest");
         //
         //+++++++++ PROBABLY NEED TO CHANGE HOW THE TEST TIME SYSTEM WORKS
         //
   if (Timers.GetTestElapsedSeconds() > (int)Data.GetDataEntryNumericValue(DB_SINE_RUNTIME_MINS) * 60)
   {
      Serial.println("StopSineTest");

      StopTest();

      // Turn the run button off.
      //
      Serial.println("B_TEST_CONTROL cleared");
      Buttons.SetButtonState(B_TEST_CONTROL, "0");     // turn button off
      NotifyClients(Buttons.GetButtonStates());
   }
   else
   {
      // We arrive here every second while the SineTestRunning flag is set 
      // and the test time has not concluded. Because the test parameters 
      // may have changed the test boundaries must be redefined for every case. 
      //
      if (MotorControl.AreBoundariesDefined() == false)
      {
         JSONVar values;
         String jsonString;
         char text[20];

         // Test boundaries must be re-defined.
         //
         Serial.println("Defining boundaries");
         MotorControl.DefineTestBoundaries(  DB_SINE_RUNTIME_MINS, 
                                             Buttons.GetBooleanState(B_SINE2_ENABLE), 
                                             Buttons.GetBooleanState(B_SINE3_ENABLE) );
                  
         vTaskDelay(1);

         // Time starts from now.
         //
         Timers.ClearSecondsTimer();
      }

      // Compute a new slug depth every second.
      //
         //
         //+++++++++ PROBABLY NEED TO CHANGE HOW THE TEST TIME SYSTEM WORKS
         //
      MotorControl.ComputeDepth(Timers.GetTestElapsedSeconds());
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

   PutSlugDepth(0);     // reset slug depth

   Timers.StartIntervalTimer(DO_SAMPLE, BACKGROUND_SAMPLE_TIME);

   Serial.println("StopTest");
   Timers.ClearSecondsTimer();

   // Now undefine the test parameters.
   //
   MotorControl.ClearBoundariesDefined();
}

//-----------------------------------------------------------------------------
// This computes the new depth then translates that into the required
// motor speed.
//
void CMotorControl::ComputeDepth(uint32_t seconds)
{
   float depth, difference, revsPerSec;
   boolean sine2, sine3;

   P_MTRCTRL(Serial.printf("MotorControl.ComputeDepth: %d  %d  %d\n", Flag.DoSineQuicklook, Flag.SineTestRunning, Flag.SweepTestRunning));

   if (Flag.DoSineQuicklook || Flag.SineTestRunning || Flag.SweepTestRunning)
   {
      sine2 = Buttons.GetBooleanState(B_SINE2_ENABLE);
      sine3 = Buttons.GetBooleanState(B_SINE3_ENABLE);

      // Compute the current slug depth.
      //
      mDepth = (NewSum(seconds, sine2, sine3) - mMax) * mScale;

      difference = mPrevious - mDepth;

      mPrevious = mDepth;

      revsPerSec = difference / mOneDrumTurnInCms;

      P_MTRCTRL(Serial.printf("Depth %f   RPS %f\n", mDepth, revsPerSec));

      if (Flag.SineTestRunning || Flag.SweepTestRunning)
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
   else
   {
      mDepth = 0;
   }

   // Publish the current depth.
   //
   PutSlugDepth(mDepth);
}

//-----------------------------------------------------------------------------
// While ever the Flag.SweepTestRunning is set, the 1 second timer will call 
// this method. Here we first check the elapsed time against the user setting,
// and if exceeded, stop the test. 
// The user can change  
//
void CMotorControl::ManageSweepTest(void)
{
   // The SweepTestRunning flag is still set. Check if the sweep test elapsed
   // time has finished.
   //
   Serial.println("ManageSweepTest");
         //
         //+++++++++ PROBABLY NEED TO CHANGE HOW THE TEST TIME SYSTEM WORKS
         //
   if (Timers.GetTestElapsedSeconds() > (int)Data.GetDataEntryNumericValue(DB_SWEEP_RUNTIME_MINS) * 60)
   {
      Serial.println("StopSweepTest");

      StopTest();

      // Turn the start button off.
      //
      Serial.println("B_SWEEP_CONTROL cleared");
      Buttons.SetButtonState(B_SWEEP_CONTROL, "0");     // turn button off
      NotifyClients(Buttons.GetButtonStates());
   }
   else
   {
      // We arrive here every second while ever the SweepTestRunning button is
      // set and the test time has not concluded. Because the test parameters 
      // may have changed the test boundaries must be redefined for every test. 
      //
      if (MotorControl.AreBoundariesDefined() == false)
      {
         JSONVar values;
         String jsonString;
         char text[20];

         // Test boundaries must be re-defined.
         //
         Serial.println("Defining boundaries");
         MotorControl.DefineTestBoundaries(DB_SWEEP_RUNTIME_MINS, false, false);
                  
         vTaskDelay(1);

         // Time starts from now.
         //
         //+++++++++ PROBABLY NEED TO CHANGE HOW THE TEST TIME SYSTEM WORKS
         //
         Timers.GetTestElapsedSeconds();
//         StartIntervalTimer(DO_SAMPLE, Control.GetSampleInterval_mSecs());
      }

      // Compute a new slug depth every second.
      //
         //
         //+++++++++ PROBABLY NEED TO CHANGE HOW THE TEST TIME SYSTEM WORKS
         //
      MotorControl.SweepState(Timers.GetTestElapsedSeconds());
   }
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
   Serial.printf("f0=%f, f1=%f, Time=%f, R=%f\n", mf0, mf1, mTime, mR);
}

//-----------------------------------------------------------------------------
//
void CMotorControl::SweepState(uint32_t seconds)
{
   float difference;
   float revsPerSec;

   mDepth = cos(2.0 * PI * seconds * (mR * seconds / 2.0 + mf0));
   mDepth = mDepth / 2.0 - 0.5;    // translate to negative
   mDepth *= Data.GetDataEntryNumericValue(DB_SWEEP_DEPTH);

   difference = mPrevious - mDepth;

   mPrevious = mDepth;

   revsPerSec = difference / mOneDrumTurnInCms;

   P_MTRCTRL(Serial.printf("Depth %f   RPS %f\n", mDepth, revsPerSec));

   // Ensure the motor is flagged as running.
   //
   Flag.MotorRunning = true;

   Motor.RunMotorAtRevsPerSec(revsPerSec);
   Motor.EnergiseBrake();

   PutSlugDepth(mDepth);
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

   P_MTRCTRL(Serial.println("MotorControl:DefineTestBoundaries"));

   mSteps = (int)Data.GetDataEntryNumericValue(runtimeIndex) * 60;   // test elapsed time in seconds
   P_MTRCTRL(Serial.printf("Steps %d\n", mSteps));

   // Establish which sines are involved.
   //
//   sine2 = Control.GetButtonState(B_SINE2_ENABLE);
//   sine3 = Control.GetButtonState(B_SINE3_ENABLE);

   // Copy the three phase offsets.
   //
   mPhase1 = (double)Data.GetDataEntryNumericValue(DB_PERIOD1_DEGS);
   if (sine2) mPhase2 = (double)Data.GetDataEntryNumericValue(DB_PERIOD2_DEGS); else mPhase2 = 0;
   if (sine3) mPhase3 = (double)Data.GetDataEntryNumericValue(DB_PERIOD3_DEGS); else mPhase3 = 0;

   mAmplitude1 = (double)Data.GetDataEntryNumericValue(DB_PERIOD1_CMS);
   if (sine2) mAmplitude2 = (double)Data.GetDataEntryNumericValue(DB_PERIOD2_CMS); else mAmplitude2 = 0;
   if (sine3) mAmplitude3 = (double)Data.GetDataEntryNumericValue(DB_PERIOD3_CMS); else mAmplitude3 = 0;

   mAmplitude = (double)Data.GetDataEntryNumericValue(DB_DEPTH_MS);
   P_MTRCTRL(Serial.printf("Amplitudes %f %f %f %f\n", mAmplitude, mAmplitude1, mAmplitude2, mAmplitude3));

   // Ensure no phase angle is allowed for sine1 if used alone.
   //
   if ( (Buttons.GetBooleanState(B_SINE2_ENABLE) == false) && (Buttons.GetBooleanState(B_SINE3_ENABLE) == false) ) 
   {
      mPhase1 = 0.0;
      Data.PutDataEntryValue(DB_PERIOD1_DEGS, "0");
   }

   // Convert the three sine periods from minutes to seconds.
   //
   mPeriod1 = (int)Data.GetDataEntryNumericValue(DB_PERIOD1_MINS) * 60.0;
   if (sine2) mPeriod2 = (int)Data.GetDataEntryNumericValue(DB_PERIOD2_MINS) * 60.0; else mPeriod2 = 0;
   if (sine3) mPeriod3 = (int)Data.GetDataEntryNumericValue(DB_PERIOD3_MINS) * 60.0; else mPeriod3 = 0;
   P_MTRCTRL(Serial.printf("Periods %f %f %f\n", mPeriod1, mPeriod2, mPeriod3));

   // Convert the periods to angular frequency.
   //
   mAngularFrequency1 = 2.0 * PI / mPeriod1;    // to angular frequency
   mAngularFrequency2 = 2.0 * PI / mPeriod2;
   mAngularFrequency3 = 2.0 * PI / mPeriod3;
   P_MTRCTRL(Serial.printf("Angular freq %f %f %f\n", mAngularFrequency1, mAngularFrequency2, mAngularFrequency3));
   P_MTRCTRL(Serial.printf("Phases %f %f %f\n", mPhase1, mPhase2, mPhase3));

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
      P_MTRCTRL(Serial.printf("i = %d,  n = %d\n", i, n));
      while (i < n)
      {
         current = NewSum(i, sine2, sine3);
         if (i < 10) Serial.printf("%f\n", current);
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

      P_MTRCTRL(Serial.printf("index = %d,  mMin = %e, mMax = %e,  oldmMax = %e,  fabs(%e)\n", index, mMin, mMax, oldmMax, fabs(mMax-oldmMax)));

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

   mSubStart = index;
   mSubEnd = index + mSteps;
   Serial.printf("mAmplitude = %f, SubStart = %d,  SubEnd = %d\n", mAmplitude, mSubStart, mSubEnd);

   mScale = mAmplitude / (mMax - mMin);
   Serial.printf("mScale = %f\n", mScale);

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

      case DB_DRUM_DIA:    // drive roller dia
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
//
float CMotorControl::NewSum(uint32_t index, boolean sine2, boolean sine3)
{
   float temp;
   float cosine1, cosine2, cosine3;

   // Compute a new sum of sines term.
   //
      mAngularFrequency1 = 2.0 * PI / 20;    // to angular frequency
      mAmplitude1 = 20;

   Serial.printf("%f %f %f %f\n", mAngularFrequency1, mPhaseScale, mPhase1, mAmplitude1);
   cosine1 = cos((mAngularFrequency1 * index) + (mPhaseScale * mPhase1));
   cosine1 = (cosine1 / 2.0 - 0.5) * mAmplitude1;

   if (sine2)
   {
      cosine2 = cos((mAngularFrequency2 * index) + (mPhaseScale * mPhase2));
      cosine2 = (cosine2 / 2.0 - 0.5) * mAmplitude2;
   }
   else
   {
      cosine2 = 0.0;
   }

   if (sine3)
   {
      cosine3 = cos((mAngularFrequency3 * index) + (mPhaseScale * mPhase3));
      cosine3 = (cosine3 / 2.0 - 0.5) * mAmplitude3;
   }
   else
   {
      cosine3 = 0.0;
   }

   temp = cosine1 + cosine2 + cosine3;

   return temp;
}
