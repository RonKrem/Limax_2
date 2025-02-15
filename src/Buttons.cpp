// Buttons.cpp
//
#include "Buttons.h"
#include "Data.h"
#include "SDCard.h"
#include "Eprom.h"
#include "Timers.h"
#include "Motor.h"
#include "MotorControl.h"
#include "ManageSamples.h"
#include "WellSimulator.h"
#include "Network.h"

#define  PRINT_BUTTONS
#ifdef PRINT_BUTTONS
#define  P_BUT(x)   x;
#else
#define  P_BUT(x)   // x
#endif

extern CSDCard SDCard; 
extern CEprom Eprom;
extern CMotor Motor;
extern CMotorControl MotorControl;
extern CTimers Timers;
extern CData Data;
extern CButtons Buttons;
extern CManageSamples ManageSamples;
extern CNetwork Network;

#ifdef SIMULATING
extern CWellSimulator WellSimulator;
#endif
extern void NotifyClients(String state);
extern Flags Flag;

extern DynamicData Temp;

// Entries are:
//   id        html id
//   checked   checked or not (a boolean)
//   onText    text shown to turn it ON
//   offText   text shown to turn it OFF
//   on color
//   off color
//   State state is saved if "T"
//   isGPIO    "T" if ID is a GPIO port
//
ButtonDetails Button[] = 
{
   {  "0", false, "INVALID",        "INVALID",     "ON",      "OFF",     0},    // zero entry unused
   {  "1", false, "MOVING UP",      "UP",          "#3e8e41", "#ee4040", IS_PUSHON_PUSHOFF},     //  1 motor up
   {  "2", false, "STOP",           "OFF",         "#3e8e41", "#ee4040", IS_PUSHON_PUSHOFF},     //  2 motor off 
   {  "3", false, "MOVING DOWN",    "DOWN",        "#3e8e41", "#ee4040", IS_PUSHON_PUSHOFF},     //  3 motor down
   {  "4", false, "CW",             "CCW",         "#3e658e", "#30cc3d", IS_PUSHON_PUSHOFF | STATE_IS_SAVED},        //  4 motor direction
   {  "5", false, "ON",             "OFF",         "#ee4040", "#3e8e41", IS_PUSHON_PUSHOFF},        //  5 brake overide
   {  "6", false, "OFF",            "ON",          "#ee4040", "#3e8e41", IS_PUSHON_PUSHOFF},        //  6 dynamic depth
   {  "7", false, "VIEW",           "ON",          "#3e8e41", "#ee4040", 0},                     //  7 view data
   {  "8", false, "DOWNLOAD",       "ON",          "#3e8e41", "#ee4040", 0},                     //  8 download data 
   {  "9", false, "DELETE",         "ON",          "#3e8e41", "#ee4040", 0},                     //  9 delete data
   { "10", false, "INCLUDED",       "EXCLUDED",    "#ee4040", "#3e8e41",  IS_PUSHON_PUSHOFF},     // 10 sine 2 enable
   { "11", false, "INCLUDED",       "EXCLUDED",    "#ee4040", "#3e8e41",  IS_PUSHON_PUSHOFF},     // 11 sine 3 enable
   { "12", false, "STOP",           "START",       "#ee4040", "#3e8e41", IS_PUSHON_PUSHOFF},     // 12 start/stop the test
   { "13", false, "STOP",           "START",       "#3e8e41", "#ee4040", IS_PUSHON_PUSHOFF},     // 13 start the quicklook
   { "14", false, "STOP",           "START",       "#ee4040", "#3e8e41", IS_PUSHON_PUSHOFF},     // 14 start/stop the sweep
   { "15", false, "START",          "STOP",        "#3e8e41", "#ee4040", 0},                     // 15 sweep quicklook
   { "16", false, "LINEAR",         "EXPNTL",      "#3e658e", "#30cc3d", IS_PUSHON_PUSHOFF | STATE_IS_SAVED},        // 16 linear/exponential
   { "17", false, "PLOTS",          "",            "#3e8e41", "#ee4040", 0},                     // 17 show the 4 place plot
};
int NumButtons = sizeof(Button) / sizeof(ButtonDetails);

PushOnPushOff PPButtons[]
{
   { 10, "B_SINE2_ENABLE"},
   { 11, "B_SINE3_ENABLE"},
   { 12, "B_TEST_CONTROL"},
   { 14, "B_SWEEP_CONTROL"},
   { 13, "B_SINE_QUICKLOOK"},
   { 16, "B_LIN_EXP"},
   {  4, "B_DIRECTION"},
   {  5, "B_BRAKE_OVERIDE"},
   {  6, "B_DYNAMIC_DEPTH"},
};
int NumPPButtons = sizeof(PPButtons) / sizeof(PushOnPushOff);

//-------------------------------------------------------------------
//
CButtons::CButtons(void)
{
}

//-------------------------------------------------------------------
// Replaces button placeholders with default "state" value.
//
String CButtons::HomeProcessor(const String& var) 
{
   P_BUT(Serial.printf("HomeProcessor: "));
   P_BUT(Serial.println(var));
   if (var == "STATE_UP")     // 1
   {
      return Buttons.GetState(B_GO_UP);
   }
   if (var == "STATE_OFF")    // 2
   {
      return Buttons.GetState(B_STOP);
   }
   if (var == "STATE_DOWN")   // 3
   {
      return Buttons.GetState(B_GO_DOWN);
   }
   return String();
}

//-------------------------------------------------------------------
// Replaces button placeholders with default "state" value.
//
String CButtons::BasicSineProcessor(const String& var) 
{
   JSONVar values;

   P_BUT(Serial.printf("BasicSineProcessor: "));
   P_BUT(Serial.println(var));
   if (var == "STATE_EX2")    // 10
   {
      return Buttons.GetState(B_SINE2_ENABLE);
   }
   else if (var == "STATE_EX3")    // 11
   {
      return Buttons.GetState(B_SINE3_ENABLE);
   }
   else if (var == "STATE_CNTRL")  // 12
   {
      return Buttons.GetState(B_TEST_CONTROL);
   }
   else if (var == "SINE_QUICKLOOK_CNTRL")  // 13
   {
      return Buttons.GetState(B_SINE_QUICKLOOK);
   }
   return String();
}

//-----------------------------------------------------------------------------
//
String CButtons::SineSweepProcessor(const String& var)
{
   P_BUT(Serial.printf("SineSweepProcessor: "));
   P_BUT(Serial.println(var));
   if (var == "STATE_SW_ON")    // 14
   {
      return Buttons.GetState(B_SWEEP_CONTROL);
   }
   if (var == "STATE_LIN_EXP")    // 16
   {
      return Buttons.GetState(B_LIN_EXP);
   }
   return String();
}

//-------------------------------------------------------------------
// Replaces button placeholders with default "state" value.
//
String CButtons::EquipmentProcessor(const String& var) 
{
   P_BUT(Serial.printf("EquipmentProcessor: "));
   P_BUT(Serial.println(var));
   if (var == "BRAKE_OVERIDE_ID")    // 5
   {
      return Buttons.GetState(B_BRAKE_OVERIDE);
   }
   if (var == "DYNAMIC_DEPTH_ID")    // 6
   {
      return Buttons.GetState(B_DYNAMIC_DEPTH);
   }
   if (var == "DIRECTION_ID")       // 4
   {
      return Buttons.GetState(B_DIRECTION);
   }
   return String();
}

//-----------------------------------------------------------------------------
//
void CButtons::SendButtonState(uint32_t index)
{
   if (GetBooleanState(index))
   {
      ButtonOff(index);
   }
   else
   {
      ButtonOff(index);
   }
}

//-----------------------------------------------------------------------------
//
void CButtons::SetBooleanState(const uint32_t index, boolean state)
{
   Button[index].checked = state;
}

//-----------------------------------------------------------------------------
//
String CButtons::GetState(uint32_t index)
{
   if (Button[index].checked)
   {
      return Button[index].onText;
   }
   else
   {
      return Button[index].offText;
   }
}

//-----------------------------------------------------------------------------
//
void CButtons::RestoreSavedStates(void)
{
   // Create filenames in EEprom for those buttons that actually save their state.
   //
   for (int i=0; i < NumButtons; i++)
   {
      P_BUT(Serial.printf("%d  ", i));

      // Only a few buttons have their states saved
      if (Button[i].states & STATE_IS_SAVED)
      {
         String directory;
         String entry;

         directory = CreateFileName(i);  // assemble directory name
         entry = Eprom.ReadFile(directory);   // read value

         // If the name does not exists we create a match here.
         //
         if (entry == "NULL")
         {
            String state;

            P_BUT(Serial.printf("Creating: %s ", directory.c_str()));
            state = GetButtonStateAsString(i);
            Eprom.WriteFile(directory, state);
         }
         else if (entry == "1")
         {
            Button[i].checked = true;
         }
         else
         {
            Button[i].checked = false;
         }
      }
      else
      {
         // Otherwise all non-saved buttons are off.
         //
         Button[i].checked = false;
      }

      P_BUT(Serial.println(Button[i].checked));
   }
}

//-----------------------------------------------------------------------------
// Copy current Button states to a String for the client.
//
String CButtons::GetButtonStates(void)
{
   JSONVar myArray;
   String state;

   P_BUT(Serial.printf(" Buttons.GetButtonStates (%d)\n", NumButtons));

   for (int i=0; i<NumButtons - 1; i++)
   {
      // Note the i+1 giving a 1 based access.
      //
      state = GetButtonStateAsString(i+1);

      myArray["button"][i]["id"] = Button[i+1].id;
      myArray["button"][i]["checked"] = state;
      myArray["button"][i]["onText"] = Button[i+1].onText;
      myArray["button"][i]["offText"] = Button[i+1].offText;
      myArray["button"][i]["onColor"] = Button[i+1].onColor;
      myArray["button"][i]["offColor"] = Button[i+1].offColor;
      P_BUT(Serial.printf("Button %d checked %d\n", i+1, Button[i+1].checked));
   }

   String jsonString = JSON.stringify(myArray);
   return jsonString;
}

//-----------------------------------------------------------------------------
//
void CButtons::ProcessCheckbox(const String identifier, const String index)
{
   int i;

   P_BUT(Serial.printf(" Buttons.ProcessCheckbox id=%d\n", index.toInt()));

   // First locate the matching Button[] entry so we can check
   // if its state should be saved in SPIFFS.
   //
   for (i=0; i<NumButtons; i++)
   {
      // Find the matching button.
      //
      if (index == Button[i].id)
      {
         String state;

         P_BUT(Serial.printf(" Match at Button database index: %d, checked %s\n", i, identifier.c_str()));
       }
   }
}

//-----------------------------------------------------------------------------
//
void CButtons::ProcessButton(const String identifier, const String index)
{
   int i;

   P_BUT(Serial.printf(" Buttons.ProcessButton identifier: <%s>  id=%d\n", identifier.c_str(), index.toInt()));

   // First locate the matching Button[] entry.
   //
   for (i=0; i<NumButtons; i++)
   {
      // Find the matching button.
      //
      if (index == Button[i].id)
      {
         // This button has been clicked. 
         //
         if (Button[i].states & IS_PUSHON_PUSHOFF)
         {
            // Because it is a push on/push off button, its new state will be
            // the opposite of what it is now.
            //
            if (GetBooleanState(i))
            {
               // Current state is pressed, so
               Button[i].checked = false;
            }
            else
            {
               Button[i].checked = true;
            }
         }
         ActionButton(i, identifier);           // action the button
         HandleWebButton(index, identifier);    // fix up the display

         // Some buttons can have their state saved over a power down.
         //
         if (Button[i].states & STATE_IS_SAVED)
         {
            String state;
            String name;

            name = CreateFileName(i);
            Eprom.DeleteFile(name);
            state = GetButtonStateAsString(i);
            P_BUT(Serial.printf(" Recordin button %d state %s\n", i, state.c_str()));
            Eprom.WriteFile(name, state);
            state = Eprom.ReadFile(name);
            P_BUT(Serial.printf("<%s> was written\n", state.c_str()));
         }
      }
   }
}

//----------------------------------------------------------------------------------------------
// Process a button change.
// <checked> is either "true" or "false".
// <state> is either "1" or "0"
//
void CButtons::ActionButton(const uint16_t index, const String identifier)
{

   P_BUT(Serial.printf(" Buttons.ActionButton index <%d>  name <%s>\n", index, identifier.c_str()));
   if (index < NumButtons)
   {
      // Note the parameter index jSonValues are 1 based, whereas the
      // buttons are 0 based. Here we use the button index as we will be
      // processing with the button database Button array.
      //
      switch (index)
      {
         case B_GO_UP:    // button 1 - UP/OFF
            P_BUT(Serial.println("UP/RUNNING Button"));
            ManageMotorState(index);
            break;

         case B_STOP:    // button 2 - STOP
            P_BUT(Serial.println("OFF/STOP Button"));
            ManageMotorState(index);
            break;

         case B_GO_DOWN:     // button 3 - DOWN/OFF
            P_BUT(Serial.println("DOWN/RUNNING Button"));
            ManageMotorState(index);
            break;

         case B_DIRECTION:  // button 4 direction
            break;

         case B_BRAKE_OVERIDE:     // button 5 - brake function
            P_BUT(Serial.println("Brake overide"));
//            SetButtonState(index, state);
            if (GetBooleanState(index))
            {
               Motor.EnergiseBrake();
            }
            else
            {
               Motor.BrakePowerOff();
            }
            break;

         case B_DYNAMIC_DEPTH:     // button 6 - dynamic depth
            break;

         case B_DATA_VIEW:     // button 7 - view data
            P_BUT(Serial.println("Data view"));
            break;

         case B_DATA_DOWNLOAD:     // button 8 - download data
            P_BUT(Serial.println("Data download"));
            break;

         case B_DATA_DELETE:     // button 9 - delete data
            P_BUT(Serial.println("Data delete"));
            break;

         case B_SINE2_ENABLE:   // button 10
            Serial.printf("Sensor 2 ");
            if (Button[index].checked)
               Serial.printf("enable. ");
            else
               P_BUT(Serial.printf("disable. "));
               P_BUT(Serial.printf(" <%d> <%s>\n", index, identifier.c_str()));
//            SetButtonState(index, state);
            break;

         case B_SINE3_ENABLE:   // button 11
            P_BUT(Serial.printf("Sensor 3 enable. <%d> <%s>\n", index, identifier.c_str()));
//            SetButtonState(index, state);
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
         case B_TEST_CONTROL:     // button 12
//            SetButtonState(index, state);
            Serial.printf("Sine start/stop  %d\n", index);
            if (GetBooleanState(index))
            {
               StartTheTest();

               Flag.SineTestRunning = true;
            }
            else
            {
               StopTheTest();
            }
            break;

         //----------------------------------------------------------
         case B_SINE_QUICKLOOK:        // button 13
            {
               String jsonString;
               JSONVar values;
               char text[20];

//               SetButtonState(index, state);
               P_BUT(Serial.printf("Sine quicklook %d\n", identifier.toInt()));
               if (GetBooleanState(index))
               {
                  MotorControl.StartProfileTimer();

                  // Now set the min scale for the chart y axis.
                  //
                  // sprintf(text, "%d", (int32_t)MotorControl.GetAmplitude());
                  // values["minValue"] = String(text);
                  // jsonString = JSON.stringify(values);
                  // events.send(jsonString.c_str(), "set_min_value", millis());
                  
//                  RemoveOldData();   // remove any old data
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
         case B_SWEEP_CONTROL:        // button 14
            P_BUT(Serial.println("Sweep start/stop"));
            if (GetBooleanState(index))
            {
               MotorControl.InitSweep();
               StartTheTest();
               
               Flag.SweepTestRunning = true;
            }
            else
            {
               StopTheTest();
            }

//             if (GetBooleanState(index))
//             {
//                // Button has just been pushed on.
//                //
// //               StartIntervalTimer(DO_SAMPLE, GetSampleInterval_mSecs());
// //               SampleNumber = 0;
// //               MonitorEntries = 0;  // reset the monitor system
// //               TestElapsedSeconds = 0;


//                Flag.SweepTestRunning = true;
//             }
//             else
//             {
//                // Button has just been pushed off. Stop the test. This clears the 
//                // flags and resets the boundaries.
//                //
//                MotorControl.StopTest();
//             }
            break;

         case B_SWEEP_QUICKLOOK:    // button 15
            Serial.println("Sweep quicklook");
            {
               String jsonString;
               JSONVar values;
               char text[20];

//               SetButtonState(index, state);
               Serial.printf("Sine quicklook %d\n", identifier.toInt());
               if (GetBooleanState(index))
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

         case B_LIN_EXP:        // button 16
//            SetButtonState(index, state);
            Serial.printf("Sweep lin/exp %d\n", GetBooleanState(index));
            break;

         case B_SHOW_PLOTS:    // button 17
            Serial.println("Show plots");
            break;

         default:
            break;
      }
   }
}

//----------------------------------------------------------------------------------------------
// Enter with <index> pointing at the correct entry in the Button[] array, and <state> containing
// the current button state.
//
void CButtons::HandleWebButton(const String index, const String identifier)
{
   uint16_t i;

   P_BUT(Serial.printf(" Buttons.HandleWebButton index <%d>  identifier <%s>\n", index.toInt(), identifier.c_str()));

   i = index.toInt();

   switch (i)
   {
      case 1:
         // The UP button.
         //
         P_BUT(Serial.printf("%d: %d %d %d\n", i, Button[1].checked, Button[2].checked, Button[3].checked));
         if (!GetBooleanState(B_GO_DOWN))     // if not going down
         {
            if (GetBooleanState(B_GO_UP))   // if going up
            {
               // Start motor running up.
               P_BUT(Serial.println("must start"));
               ButtonOn(B_GO_UP);
               ButtonOn(B_STOP);
            }
            else
            {
               // Stop motor
               P_BUT(Serial.println("must stop"));
               ButtonOff(B_GO_UP);
               ButtonOff(B_STOP);
            }
         }
         break;

      case 2:
         // The STOP button.
         //
         P_BUT(Serial.printf("%d: %d %d %d\n", i, Button[1].checked, Button[2].checked, Button[3].checked));
         if (GetBooleanState(B_GO_UP))
         {
            Button[B_GO_UP].checked = Button[B_GO_DOWN].checked = Button[B_STOP].checked = false;
            ButtonOff(B_GO_UP);
            ButtonOff(B_STOP);
         }
         else if (GetBooleanState(B_GO_DOWN))
         {
            Button[B_GO_UP].checked = Button[B_GO_DOWN].checked = Button[B_STOP].checked = false;
            ButtonOff(B_GO_DOWN);
            ButtonOff(B_STOP);
         }
         break;

      case 3:
         // The DOWN button.
         //
         P_BUT(Serial.printf("%d: %d %d %d\n", i, Button[1].checked, Button[2].checked, Button[3].checked));
         if (!GetBooleanState(B_GO_UP))
         {
            if (GetBooleanState(B_GO_DOWN))
            {
               // Start motor running down.
               P_BUT(Serial.println("must start"));
               ButtonOn(B_GO_DOWN);
               ButtonOn(B_STOP);
            }
            else
            {
               P_BUT(Serial.println("must stop"));
               ButtonOff(B_GO_DOWN);
               ButtonOff(B_STOP);
            }
         }
         break;
      case 4:
      case 5:
      case 6:
      case 10:
      case 11:
      case 12:
      case 14:
      case 13:
      case 16:
         P_BUT(Serial.printf("Index: %d: State: %d\n", i, Button[i].checked));
         if (Button[i].checked)
         {
            ButtonOn(i);
         }
         else
         {
            ButtonOff(i);
         }
         break;

      case 0:
      default:
         break;
   }

//   Serial.printf(">%d: %d %d %d\n", i, Button[1].checked, Button[2].checked, Button[3].checked);
}

//----------------------------------------------------------------------------------------------
//
void CButtons::StartTheTest(void)
{
   // Button has just been pushed on.
   //
   Serial.println("Starting test");   
   Temp.entries = 0;
#ifdef SIMULATING   
   WellSimulator.ClearSampleNumber();
#endif   
   ManageSamples.WriteSitePreamble();
   MotorControl.ComputeParameters();
   Flag.Recording = true;
   Network.StartSample();
}

//----------------------------------------------------------------------------------------------
//
void CButtons::StopTheTest(void)
{
   // Button has just been pushed off. Stop the test. This clears the 
   // flags and resets the boundaries.
   //
   Serial.println("Stopping test");       
   Flag.Recording = false;
   Flag.ReturnToZero = false;
   if (MotorControl.GetSlugDepth() > -PARK_SHUTDOWN_DISTANCE)
   {

   }
   else
   {
      MotorControl.DoParking();
   }
}

//----------------------------------------------------------------------------------------------
//
void CButtons::ButtonOn(const uint16_t i)
{
   JSONVar values;

   values["index"] = i;
   values["state"] = Button[i].checked;
   values["text"] = Button[i].onText;
   values["color"] = Button[i].onColor;
   NotifyClients(JSON.stringify(values));
}

//----------------------------------------------------------------------------------------------
//
void CButtons::ButtonOff(const uint16_t i)
{
   JSONVar values;

   values["index"] = i;
   values["state"] = Button[i].checked;
   values["text"] = Button[i].offText;
   values["color"] = Button[i].offColor;
   NotifyClients(JSON.stringify(values));
}

//----------------------------------------------------------------------------------------------
// This function must collect the current states of all push on/push off buttons and record
// their states in a json string. For each button there needs to be an entry for its text
// and an entry for its colour.
//
String CButtons::GetButtonTextAndColors(void)
{
   JSONVar data;
   char text[20];
   uint32_t index;
   
   P_BUT(sprintf(text, "%d", NumPPButtons));
   data["entries"] = text;
   for (int i=0; i<NumPPButtons; i++)
   {
      index = PPButtons[i].id;
      P_BUT(sprintf(text, "button%d", i));
      data[(const char *)&text] = AssembleStates(i, index);
   }
   return JSON.stringify(data);
}

//----------------------------------------------------------------------------------------------
//
String CButtons::AssembleStates(uint32_t i, uint32_t index)
{
   JSONVar data;

   data["index"] = PPButtons[i].id;
   data["entry"] = PPButtons[i].name;
   if (GetBooleanState(index))
   {
      P_BUT(Serial.printf("%d ON\n", index));
      data["text"] = Button[index].onText; 
      data["color"] = Button[index].onColor;
   }
   else
   {
      P_BUT(Serial.printf("%d OFF\n", index));
      data["text"] = Button[index].offText; 
      data["color"] = Button[index].offColor;
   }
   P_BUT(Serial.println(data));
   return JSON.stringify(data);
}

//----------------------------------------------------------------------------------------------
// Arrive here to action the MotorIsRunningUP and MotorIsRunningDOWN states with the
// correct motor actions..
//
void CButtons::ManageMotorState(const uint32_t index)
{
   P_BUT(Serial.printf(" Buttons.ManageMotorState:  index <%d>  checked <%d>\n", index, Button[index].checked));

   // The stop buuton overides the other buttons, but if the OFF button is
   // off and either one or the other is ON, the motor runs.
   //
   if ( !Button[B_STOP].checked &&
        ( (Button[B_GO_UP].checked && !Button[B_GO_DOWN].checked) || 
          (Button[B_GO_DOWN].checked && !Button[B_GO_UP].checked) ) )
   {
      float speed;

      // First update the motor speed from the current user setting.
      //
      Motor.SetWinchSpeedRPM(Data.GetDataEntry(DB_MOTOR_LIFT_RPM).HtmlValue.toInt());

      // Get the actual winch speed (rps) back.
      //
      speed = Motor.GetWinchSpeed();
      if (Button[B_GO_DOWN].checked)
      {
         // Negative speed implies going down.
         //
         speed = -speed;
         Serial.println("  Motor is started DOWN");
      }
      else if (Button[B_GO_UP].checked)
      {
         Serial.println("  Motor is started UP");
      }

      // Run the motor at that speed.
      //
      Motor.RunMotorAtRevsPerSec(speed);
      Motor.EnergiseBrake();
   }
   else
   {
      // The motor is stopped.
      //
      Serial.println("  Motor is stopped");      
      Motor.SoftStop();
      Motor.BrakePowerOff();
   }
}

//-----------------------------------------------------------------------------
// Return the DB[i].JsonValue as a directory name.
//
String CButtons::CreateFileName(const uint32_t index) const
{
   String name;
   
   name = "/Btn_";
   name += Button[index].id;
//   name += ".st";
   P_BUT(Serial.printf("  %s   ", name));
   return name;
}

//-----------------------------------------------------------------------------
//
void CButtons::EnsureNonSavedIOButtonsOff(void)
{
   Serial.println(" EnsureNonSavedIOButtonsOff");

   for (int i=1; i<NumButtons; i++)
   {
      // If this button should not have its state saved, ensure
      // it is really off on startup.
      //
      if ( (Button[i].states & IS_GPIO_PORT) && (Button[i].states & STATE_IS_SAVED) )
      {
         digitalWrite(Button[i].id.toInt(), 0);
      }
   }
}

//-----------------------------------------------------------------------------
// The <state> parameter is either "1" or "0", whereas the <checked> parameter
// is true or false.
// Some buttons are push on/push off, and in that case, each time the button 
// is pressed it changes state and the current state is recorded in the Button 
// array <checked> boolean variable.
//
void CButtons::SetButtonState(const uint32_t index, const String state)
{
   P_BUT(Serial.printf("Setting button %d state <%s>\n", index, state.c_str()));

   if (Button[index].states & IS_PUSHON_PUSHOFF)
   {
      if (state.toInt() == 0)
      {
         Button[index].checked = false;
      }
      else
      {
         Button[index].checked = true;
      }
   }
   else
   {
      Serial.println("State not catered");
   }
}

//-----------------------------------------------------------------------------
//
boolean CButtons::GetBooleanState(uint32_t index)
{
//   Serial.printf(" Button %d, state %d\n", index, Button[index].checked);
   return Button[index].checked;
}

//-----------------------------------------------------------------------------
//
String CButtons::GetButtonStateAsString(const uint32_t index) const
{
   String state;

   if (Button[index].checked)
   {
      state = "1";
   }
   else
   {
      state = "0";
   }

   return state;
}