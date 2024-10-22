// Buttons.cpp
//
#include "Buttons.h"
#include "DataInputs.h"
#include "SDCard.h"
//#include "Motor.h"
//#include "MotorControl.h"


extern CSDCard SDCard; 
//extern CMotor Motor;
//extern CMotorControl MotorControl;

// Entries are:
//   id        html id
//   checked   checked or not (a String)
//   onText    text shown to turn it ON
//   offText   text shown to turn it OFF
//   *path     associated SPIFFS path
//   saveState state is saved if "T"
//   isGPIO    "T" if ID is a GPIO port
//
ButtonDetails Button[] = 
{
   {  "0", "0", "INVALID",    "INVALID",      NULL, 0},    // zero entry unused
   {  "1", "0", "UP",         "RUNNING",      NULL, 0},    //  1 motor up
   {  "2", "0", "STOP",       "STOP",         NULL, 0},    //  2 motor down 
   {  "3", "0", "DOWN",       "RUNNING",      NULL, STATE_IS_SAVED},    //  3 motor off
};
int NumButtons = sizeof(Button) / sizeof(ButtonDetails);


//-------------------------------------------------------------------
//
CButtons::CButtons(void)
{
}

//-----------------------------------------------------------------------------
//
void CButtons::SetButtonPaths(void)
{
   // Create filenames for those buttons that actually save their state.
   //
   for (int i=0; i < NumButtons; i++)
   {
      String name;
      char* pathName;

      Serial.printf("%d  ", i);

      if (Button[i].states & STATE_IS_SAVED)
      {
         // Using new here is safe as this function is only called once
         // at program startup.
         //
         pathName = new char [MAX_PATHNAME_LENGTH];
         name = Button[i].id;
         strcpy(pathName, "/Button");
         strcat(pathName, name.c_str());
         strcat(pathName, ".txt");
         Button[i].path = pathName;
         Serial.printf("%s\n", Button[i].path);
      }
      else
      {
         Serial.println();
      }

   }
}

//-----------------------------------------------------------------------------
// Copy current Button states to a String for the client.
//
String CButtons::GetButtonStates(void)
{
   JSONVar myArray;

  Serial.printf(" Control:GetButtonStates (%d)\n", NumButtons);

   for (int i=0; i<NumButtons - 1; i++)
   {
      myArray["button"][i]["id"] = Button[i+1].id;
      myArray["button"][i]["checked"] = Button[i+1].state;
      myArray["button"][i]["onText"] = Button[i+1].onText;
      myArray["button"][i]["offText"] = Button[i+1].offText;
      Serial.printf("Button %d checked %s\n", i+1, Button[i+1].state.c_str());
   }

   String jsonString = JSON.stringify(myArray);
   return jsonString;
}

//-----------------------------------------------------------------------------
//
void CButtons::ProcessButtons(String id, String checked)
{
   int i;

   Serial.printf(" Control:ProcessButtons id=%d\n", id.toInt());

   // First locate the matching Button[] entry so we can check
   // if its state should be saved in SPIFFS.
   //
   for (i=1; i<NumButtons; i++)
   {
      // Find the matching button.
      //
      if (id == Button[i].id)
      {
         String state;

         Serial.printf(" Match at Button database index: %d, checked %s\n", i, checked.c_str());

         // Convert the "true"/"false" from javascript to the 
         // "T"/"F" required in SPIFFS.
         //
         state = "0";
         if (checked == "true") 
         {
            state = "1";
         }

         if (Button[i].states & IS_GPIO_PORT)
         {
            // This is a real gpio port, so set its output according
            // to the javascript "checked" state.
            //
            // HOWEVER, THERE ARE NO DIRECT IO POINTS IN THE LIMAX THE USER CAN CHANGE.
         }

         // Some buttons can have their pressed state saved
         // over a power down.
         //
         if (Button[i].states & STATE_IS_SAVED)
         {
            Serial.printf(" Recording button: %s state\n", id);
            SDCard.WriteFile(Button[i].path, state.c_str());
            Button[i].state = state;
         }

         ActionButton(i, checked, state);
      }
   }

//   NotifyClients(GetButtonStates());
}

//----------------------------------------------------------------------------------------------
// Process a button change.
// <checked> is either "true" or "false".
// <state> is either "1" or "0"
//
void CButtons::ActionButton(uint16_t index, String checked, String state)
{
   Serial.printf(" Control:ActionButton index %d  state %s  checked %s\n", index, state.c_str(), checked.c_str());
   if (index < NumButtons)
   {
      // Note the parameter index jSonValues are 1 based, whereas the
      // buttons are 0 based. Here we use the button index as we will be
      // processing with the button database Button array.
      //
      switch (index)
      {
         case BUTTON_GO_UP:    // button 1 - UP/OFF
            Serial.println("UP/OFF Button");
            ManageMotorUp(index, state);
            break;

         case BUTTON_GO_DOWN:    // button 2 - DOWN/OFF
            Serial.println("DOWN/OFF Button");
            ManageMotorDown(index, state);
            break;

         case BUTTON_DIRECTION:     // button 3 - CW/CCW
            Serial.println("DIRECTION Button");
            break;

         case BUTTON_BRAKE_OVERIDE:     // button 4 - brake function
            SetButtonState(state, index);
            if (GetButtonState(index))
            {
//               Motor.EnergiseBrake();
            }
            else
            {
//               Motor.BrakePowerOff();
            }
            break;

         case BUTTON_DYNAMIC_DEPTH:     // button 5 - dynamic depth
            break;

         case BUTTON_DATA_VIEW:     // button 6 - view data
            Serial.println("Data view");
            break;

         case BUTTON_DATA_DOWNLOAD:     // button 7 - download data
            Serial.println("Data download");
            break;

         case BUTTON_DATA_DELETE:     // button 8 - delete data
            Serial.println("Data delete");
            break;

         case BUTTON_SINE2_ENABLE:   // button 9
            SetButtonState(state, index);
            Serial.println("Sensor 2 enable");
            break;

         case BUTTON_SINE3_ENABLE:   // button 10
            SetButtonState(state, index);
            Serial.println("Sensor 3 enable");
            break;

         //------------------------------------------------------------------------------
         // This is the event that starts a test running: the user pressing this button.
         // To start we set the SineTestRunning flag and clear the SampleNumber and
         // MonitorEntries counters. The MotorControl.Task_ProcessStepTimer task runs
         // every second via a queue set from the StepTimerDone timer in Main. If it
         // finds this flag set it calls the MotorControl.ManageSineTest function. Its
         // job is to check the current TestElapsedSeconds count against the user set
         // DB[DB_SINE_RUNTIME_MINS] numeric and if exceed, will stops the motor which clears
         // the SineTestRunning flag.
         //
         case BUTTON_TEST_CONTROL:     // button 11
            SetButtonState(state, index);
            Serial.println("Sine start/stop");
            if (GetButtonState(index))
            {
               // Button has just been pushed on.
               //
               Serial.println("Starting test");       
//               StartIntervalTimer(DO_SAMPLE, GetSampleInterval_mSecs());
//               SampleNumber = 0;
//               MonitorEntries = 0;  // reset the monitor system
//               TestElapsedSeconds = 0;
//               Flag.SineTestRunning = true;
            }
            else
            {
               // Button has just been pushed off. Stop the test. This clears the 
               // flags and resets the boundaries.
               //
//               MotorControl.StopTest();
            }

           Serial.println("Sine start/stop");
            break;

         //----------------------------------------------------------
         case BUTTON_SINE_QUICKLOOK:        // button 12
            {
               String jsonString;
               JSONVar values;
               char text[20];

               SetButtonState(state, index);
               Serial.printf("Sine quicklook %d\n", state.toInt());
               if (GetButtonState(index))
               {
                  // This starts a quicklook display of the proposed slug
                  // position.
                  // 1. Define the test boundaries with the current settings.
                  // 2. Set QuickLookSteps to equal the MasterSecondsTicks
                  //    value somewhere between 1 and 2 seconds from now.
                  // 3. Set QuickLookStep to the GetSubStartValue() value.
                  //    This defines the correct start value where the slug
                  //    is at its highest during the chosen period.
                  //
//                  MotorControl.DefineTestBoundaries(  DB_SINE_RUNTIME_MINS, 
//                                                      GetButtonState(BUTTON_SINE2_ENABLE), 
//                                                      GetButtonState(BUTTON_SINE3_ENABLE) );
                  
//                  QuickLookStep = MotorControl.GetSubStartValue();

                  vTaskDelay(1);

                  // Now set the min scale for the chart y axis.
                  //
                  // sprintf(text, "%d", (int32_t)MotorControl.GetAmplitude());
                  // values["minValue"] = String(text);
                  // jsonString = JSON.stringify(values);
                  // events.send(jsonString.c_str(), "set_min_value", millis());
                  
//                  RemoveOldData();   // remove any old data

//                  QuickLookTicks = 0;
//                  Flag.DoSineQuicklook = true;
               }
               else
               {
//                  Flag.DoSineQuicklook = false;     // to be sure
               }
            }
            break;

         //------------------------------------------------------------------------------
         // This is the event that starts a sweep test running: the user pressing this
         // button. To start we set the SweepTestRunning flag and clear the SampleNumber and
         // MonitorEntries counters. The MotorControl.Task_ProcessStepTimer task runs
         // every second via a queue set from the StepTimerDone timer in Main. If it
         // finds this flag set it calls the MotorControl.ManageSweepTest function. Its
         // job is to check the current TestElapsedSeconds count against the user set
         // DB[DB_SINE_RUNTIME_MINS] numeric and if exceeded, will stop the motor which clears
         // the SweepTestRunning flag.
         //
         case BUTTON_SWEEP_CONTROL:        // button 13
            SetButtonState(state, index);
            Serial.println("Sweep start/stop");

            if (GetButtonState(index))
            {
               // Button has just been pushed on.
               //
//               StartIntervalTimer(DO_SAMPLE, GetSampleInterval_mSecs());
//               SampleNumber = 0;
//               MonitorEntries = 0;  // reset the monitor system
//               TestElapsedSeconds = 0;

//               MotorControl.InitSweep();

//               Flag.SweepTestRunning = true;
            }
            else
            {
               // Button has just been pushed off. Stop the test. This clears the 
               // flags and resets the boundaries.
               //
//               MotorControl.StopTest();
            }
            break;

         case BUTTON_SWEEP_QUICKLOOK:    // button 14
            Serial.println("Sweep quicklook");
            {
               String jsonString;
               JSONVar values;
               char text[20];

               SetButtonState(state, index);
               Serial.printf("Sine quicklook %d\n", state.toInt());
               if (GetButtonState(index))
               {
                  // This starts a quicklook display of the proposed slug
                  // position.
                  // 1. Define the test boundaries with the current settings.
                  // 2. Set QuickLookSteps to equal the MasterSecondsTicks
                  //    value somewhere between 1 and 2 seconds from now.
                  // 3. Set QuickLookStep to the GetSubStartValue() value.
                  //    This defines the correct start value where the slug
                  //    is at its highest during the chosen period.
                  //
//                  MotorControl.DefineTestBoundaries(DB_SWEEP_RUNTIME_MINS, false, false);

//                  QuickLookStep = MotorControl.GetSubStartValue();
                  
//                  MotorControl.InitSweep();

                  vTaskDelay(1);

                  // Now set the min scale for the chart y axis.
                  //
                  // sprintf(text, "%d", (int32_t)MotorControl.GetAmplitude());
                  // values["minValue"] = String(text);
                  // jsonString = JSON.stringify(values);
                  // events.send(jsonString.c_str(), "set_min_value", millis());
                  
//                  RemoveOldData();   // remove any old data

//                  QuickLookTicks = 0;
//                  Flag.DoSweepQuicklook = true;
               }
               else
               {
//                  Flag.DoSweepQuicklook = false;     // to be sure
               }
            }
            break;

         case BUTTON_LIN_EXP:        // button 15
            SetButtonState(state, index);
            Serial.println("Sweep lin/exp");
            break;

         case BUTTON_SHOW_PLOTS:    // button 16
            Serial.println("Show plots");
            break;

         default:
            break;
      }
   }
}

//----------------------------------------------------------------------------------------------
//
void CButtons::ManageMotorUp(uint32_t index, String state)
{
   if (state == "1")
   {
      // Request to start the motor going up. Will not be
      // allowed if down button already on.
      //
      if (Button[index + 1].state != "1")
      {
         Button[index].state = state;
      }
      else
      {
         Button[index].state = "0";    // ensure off
      }
   }
   else
   {
      Button[index].state = "0";
   }

   if (Button[index].state == "1")
   {
      float speed;

      // First update the motor speed from the current user setting.
      //
//      Motor.SetWinchSpeedRPM(DB[DB_MOTOR_LIFT_RPM].HtmlValue.toInt());

      // Get the actual winch speed (rps) back.
      //
//      speed = Motor.GetWinchSpeed();

      Serial.println(speed);
      
      // Run the motor at that speed.
      //
//      Motor.RunMotorAtRevsPerSec(speed);
//      Motor.EnergiseBrake();
   }
   else
   {
//      Motor.SoftStop();
//      Motor.BrakePowerOff();
   }
}

//----------------------------------------------------------------------------------------------
//
void CButtons::ManageMotorDown(uint32_t index, String state)
{
   Serial.println("ManageMotorDown");
   if (state == "1")
   {
      // Request to start the motor going down. Will not be
      // allowed if up button already on.
      //
      if (Button[index - 1].state != "1")
      {
         Button[index].state = state;
      }
      else
      {
         Button[index].state = "0";    // ensure off
      }
   }
   else
   {
      Button[index].state = "0";
   }

   if (Button[index].state == "1")
   {
      float speed;

      // First update the motor speed from the current user setting.
      //
//      Motor.SetWinchSpeedRPM(DB[DB_MOTOR_LIFT_RPM].HtmlValue.toInt());

      // Get the actual winch speed (rps) back.
      //
//      speed = Motor.GetWinchSpeed();

      // Run the motor at that speed only now in the other direction.
      //
//      Motor.RunMotorAtRevsPerSec(-speed);
//      Motor.EnergiseBrake();
   }
   else
   {
//      Motor.SoftStop();
//      Motor.BrakePowerOff();
   }
}

//-----------------------------------------------------------------------------
// Get a button state.
//
boolean CButtons::GetButtonState(uint32_t index)
{
   if (Button[index].state.toInt() == 0)
   {
      return false;
   }
   else
   {
      return true;
   }
}

//----------------------------------------------------------------------------------------------
// Use this function to make a button's state permanent, ie a press on/press off
// button type.
//
boolean CButtons::SetButtonState(String state, const uint32_t index)
{
   if (state == "1")
   {
      Button[index].state = "1";
      return true;
   }
   else
   {
      Button[index].state = "0";
      return false;
   }
}
