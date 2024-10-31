// CDataInputs.cpp
//
#include "DataInputs.h"
#include "Eprom.h"
#include "EspNow.h"
#include "Sensors.h"
#include "Motor.h"
#include "MotorControl.h"

extern CEprom Eprom;
extern Flags Flag;
extern JSONVar jSonValues;
extern JSONVar jSonReadings;
extern CEspNow EspNow;
extern CSensors Sensors;
extern CMotor Motor;
extern CMotorControl MotorControl;


//-------------------------------------------------------------------
// String      InputParameter;   // matches the input name from html
// String      HtmlValue;        // item value
// boolean     Update;           // must update SPIFFS    
//
DataEntry DB[] = 
{
   {"p1_mins",    "40",    true},               // 0    DB_PERIOD1_MINS,           // 0
   {"p1_cms",     "1",     true},               // 1   DB_PERIOD1_CMS,            // 1
   {"p1_degs",    "0",     true},               // 2   DB_PERIOD1_DEGS,           // 2
   {"p2_mins",    "35",    true},               // 3    DB_PERIOD2_MINS,           // 3
   {"p2_cms",     "1",     true},               // 4   DB_PERIOD2_CMS,            // 4
   {"p2_degs",    "0",     true},               // 5   DB_PERIOD2_DEGS,           // 5
   {"p3_mins",    "32",    true},               // 6   DB_PERIOD3_MINS,           // 6
   {"p3_cms",     "1",     true},               // 7   DB_PERIOD3_CMS,            // 7
   {"p3_degs",    "90",    true},               // 8   DB_PERIOD3_DEGS,           // 8

   {"rt_mins",    "120",   true},               // 9   DB_SINE_RUNTIME_MINS,      // 9
   {"deepness",   "50",    true},               // 10   DB_DEPTH_CMS,              // 10

   {"descrip",    "test description",  true},   // 11   DB_TEST_DESCRIPTION,       // 11
   {"detail1",    "details",  true},            // 12   DB_S1_DESCRIPTION,         // 12
   {"detail2",    "details",  true},            // 13   DB_S2_DESCRIPTION,         // 13
   {"detail3",    "details",  true},            // 14   DB_S3_DESCRIPTION,         // 14
   
   {"interval",   "60",    true},               // 15   DB_SAMPLE_INTERVAL,        // 15

   {"accelpwr",   "10",    true},               // 16   DB_ACCEL_POWER,            // 16
   {"decelpwr",   "10",    true},               // 17   DB_DECEL_POWER,            // 17
   {"runpwr",     "10",    true},               // 18   DB_RUN_POWER,              // 18
   {"stoppwr",    "5",     true},               // 19   DB_STOP_POWER,             // 19
   {"drivedia",   "47.74", true},               // 20   DB_DRUM_DIA,               // 20
   {"drvratio",   "2.6",   true},               // 21   DB_DRIVE_RATIO,            // 21
   
   {"temp",       "0",     false},              // 22   DB_CONTROLLER_TEMP,        // 22
   {"rpm",        "0",     false},              // 23   DB_MOTOR_LIFT_RPM,         // 23
   {"etime",      "0",     false},              // 24   DB_TEST_ELASED_TIME,       // 24
   {"swstart",    "15",    true},               // 25   DB_SWEEP_START,            // 25
   {"swstop",     "30",    true},               // 26   DB_SWEEP_STOP,             // 26
   {"swdepth",    "100",   true},               // 27   DB_SWEEP_DEPTH,            // 27
   {"swpre",      "1",     true},               // 28   DB_SWEEP_PREAMBLE,         // 28
   {"swtime",     "15",    true},               // 29   DB_SWEEP_RUNTIME_MINS,     // 29
};
int DB_Entries = sizeof(DB) / sizeof(DataEntry);

// A safe reply for too big index.
DataEntry Blank[]
{
   {"blank", "0", false}          // NULL return
};

// A set of indexes for sensors.
enum  
{
   SENSOR_1,
   SENSOR_2,
   SENSOR_3,
};

//-------------------------------------------------------------------
//
CDataInputs::CDataInputs(void)
{
   mOldValue = "0";
}

//-------------------------------------------------------------------
//
DataEntry CDataInputs::GetEntry(uint32_t index) const
{
   if (index < DB_Entries)
   {
      return DB[index];
   }
   return Blank[0];
}

//-----------------------------------------------------------------------------
void CDataInputs::PutEntry(uint32_t index, String value)
{
   if (index < DB_Entries)
   {
      DB[index].HtmlValue = value;
   }
}

//-----------------------------------------------------------------------------
// On startup, initialise the DB[] variable database values by reading from the 
// LittleFS database in eprom. The DB[] array will therefore contain the 
// HtmlValue contents saved in eprom during previous runs. 
//
// New values can be added in the declaration above and they will be automaticlly
// saved in LittleFS as this point.
//
// Only used at startup - see GetCurrentValuesForClient, etc for transactions
// once running.
//
void CDataInputs::RestoreSavedValues(void)
{
   uint32_t i;
   String entry;

//   Serial.printf("RestoreSavedValues  %d\n", DB_Entries);

   // Read the input values.
   //
   for (i = 0; i < DB_Entries; i++)
   {
      String directory;

      directory = CreateDirectoryName(i);          // assemble directory name
      entry = Eprom.ReadFile(directory);   // read value

      // If no entry exists it will be due to a new entry being created in
      // the DB database definition above. Here we create the first time
      // new entry in LittleFS.
      //
      if (entry == "NULL")
      {
         // If no entry, write the defnition value.
         //
//         Serial.printf("Writing new entry <%s>\n", DB[i].HtmlValue.c_str());
         Eprom.WriteFile(directory.c_str(), DB[i].HtmlValue.c_str());
      }
      else
      {
//         Serial.printf("Using LittleFS entry <%s>\n", DB[i].HtmlValue.c_str());
         DB[i].HtmlValue = entry;                     // copy to database
      }
   }
}

//-----------------------------------------------------------------------------
// Called from the HandleWebSocketMessage function for the case where indexing
// word is either "isnumber" or "istext". Locate the matching entry in DB and
// update both the DB numeric database and the matching Eprom database.
//
void CDataInputs::MatchAndUpdate(String &name, String &newValue)
{
   Serial.printf("MatchAndUpdate: %s %s", name.c_str(), newValue.c_str());
   for (int i = 0; i < DB_Entries; i++)
   {
      if (name == DB[i].InputParameter)
      {
         String directory;

         // If a test is running then certain parameters that control the test
         // cannot be altered.
         //
         switch (i)
         {
            case DB_PERIOD1_MINS:
            case DB_PERIOD1_CMS:
            case DB_PERIOD1_DEGS:
            case DB_PERIOD2_MINS:
            case DB_PERIOD2_CMS:
            case DB_PERIOD2_DEGS:
            case DB_PERIOD3_MINS:
            case DB_PERIOD3_CMS:
            case DB_PERIOD3_DEGS:
            case DB_SINE_RUNTIME_MINS:
            case DB_DEPTH_CMS:
               // While a test is running, these values cannot be altered.
               //
               if (Flag.SineTestRunning | Flag.SweepTestRunning)
               {
                  return;
               }
               break;

            default:
               break;
         }

         directory = CreateDirectoryName(i);
         mOldValue = Eprom.ReadFile(directory);
         Serial.printf(" Item %s SPIFFS value: %s\n", name.c_str(), mOldValue.c_str());

         // Check if this value differs from the database. Update any entry that
         // has changed for subsequent updates to the controller.
         //
         if (newValue == mOldValue)
         { 
            // Same as Eprom entry - no need to change.
            DB[i].Update = false; 
         } 
         else 
         {
            DB[i].Update = true;
            DB[i].HtmlValue = newValue;
         }
          
         // Regardless of whether the database should be updated, here
         // we call ActionParameter.
         //
         if (ActionParameter(i))
         {
            // Only update Eprom if ActionParameter returns true.
            //
            Serial.printf("Eprom entry %s new value: %s\n", name.c_str(), newValue.c_str());
            Eprom.WriteFile(directory.c_str(), newValue.c_str());
         }
      }
   }
}

//----------------------------------------------------------------------------------------------
// Action a parameter change. The index is the database DB[] index. On arrival here the 
// corresponding DB ands Eprom value has been either validated or updated and the 
// DB[index].Update boolean will reflect which one.
//
boolean CDataInputs::ActionParameter(int index)
{
   String jsonString;

   Serial.printf("ActionParameter %d: %s = %s\n", index, DB[index].InputParameter, DB[index].HtmlValue.c_str());

   // First update the motor parameters.
   //
   if (!MotorControl.MotorActionParameters(index))
   {
      // If we get here, then the index does not apply to variables
      // handled by CMotorControl.
      //
      switch (index)
      {
         case DB_PERIOD1_MINS:
         case DB_PERIOD1_CMS:
         case DB_PERIOD1_DEGS:
         case DB_PERIOD2_MINS:
         case DB_PERIOD2_CMS:
         case DB_PERIOD2_DEGS:
         case DB_PERIOD3_MINS:
         case DB_PERIOD3_CMS:
         case DB_PERIOD3_DEGS:
         case DB_SINE_RUNTIME_MINS:
         case DB_DEPTH_CMS:
            break;

         case DB_TEST_DESCRIPTION:  // blank2Value
            break;

         case DB_S1_DESCRIPTION:  // blank3Value
            break;

         case DB_S2_DESCRIPTION:  // blank4Value
            break;

         case DB_S3_DESCRIPTION:  // blank4Value
            break;

         case DB_SAMPLE_INTERVAL:  // blank4Value
            mSampleInterval = DB[index].HtmlValue.toInt() * 1000;    // interval in mSecs
            Flag.UpdateIntervalTimer = true;
            break;

         case DB_CONTROLLER_TEMP:
            break;

         case DB_MOTOR_LIFT_RPM:
            // Convert RPM to SPS the set speed.
            //     
            Motor.SetWinchSpeedRPM(DB[DB_MOTOR_LIFT_RPM].HtmlValue.toInt());
            break;
            
         case DB_DATAFILE_NAME:
            Serial.println("ActionParameter: DB_DATAFILE_NAME");
            break;

         default:
            break;
      }
   }

   return true;
}

//-----------------------------------------------------------------------------
// Read the current jSonValues to the client.
//
String CDataInputs::GetCurrentSpiffsValues(void)
{
   String jsonString;

   Serial.println(" Control:GetCurrentSpiffValues");

   // Initialise the js myObj jSonValues. Note the DB indexes must match
   // the actual inputs, and the xxx in xxxValues must match the
   // html <text> input name.
   //
   for (int i = 0; i < DB_Entries; i++)
   {
      jSonValues[DB[i].InputParameter] = Eprom.ReadFile(CreateDirectoryName(i).c_str());
   }

   jsonString = JSON.stringify(jSonValues);

   return jsonString;
}

//-----------------------------------------------------------------------------
// Return the DB[i].InputParameter as a directory name.
//
String CDataInputs::CreateDirectoryName(uint32_t index)
{
   String name;
   
   name = "/";
   name += DB[index].InputParameter;
   name += ".txt";
//   Serial.printf("  %s\n", name.c_str());
   return name;
}

//-----------------------------------------------------------------------------
// Collect here all data that should be displayed at sample update time.
//
String GetCurrentValuesForClient(SensorSampleType* sampleSetPtr)
{
   String jsonString;
   char theString[50];

//   PRNTLN(Serial.println(" GetCurrentVariableValues"));

   vTaskDelay(1);

//   jSonReadings[TEST_ELAPSED_TIME] = MotorControl.SecondsToTime(TestElapsedSeconds).c_str();

//   sprintf(theString, "%.3f",  MotorControl.GetSlugDepth());
//   jSonReadings[SLUG_DEPTH] = theString;

   // Insert the pressure and temperature data from each sensor along
   // with their corrosponding units.
   //
   sprintf(theString, "%.2f ", sampleSetPtr->SensorData[SENSOR_1].pressure.value);
   jSonReadings[SENSOR_1_PRESS] = theString;       // first for JSON
   sprintf(theString, "%s", Sensors.GetSensorPressUnits(sampleSetPtr->SensorData[SENSOR_1].pressure.unit).c_str());
   jSonReadings[SENSOR_1_PRESS_UNIT] = theString;  // for JSON
   sprintf(theString, "%.1f ", sampleSetPtr->SensorData[SENSOR_1].temperature.value);
   jSonReadings[SENSOR_1_TEMP] = theString;        // for JSON
   sprintf(theString, "%s", Sensors.GetSensorTempUnits(sampleSetPtr->SensorData[SENSOR_1].temperature.unit).c_str());
   jSonReadings[SENSOR_1_TEMP_UNIT] = theString;   // for JSON

   vTaskDelay(1);

   sprintf(theString, "%.2f ", sampleSetPtr->SensorData[SENSOR_2].pressure.value);
   jSonReadings[SENSOR_2_PRESS] = theString;    // first for JSON
   sprintf(theString, "%s", Sensors.GetSensorPressUnits(sampleSetPtr->SensorData[SENSOR_2].pressure.unit).c_str());
   jSonReadings[SENSOR_2_PRESS_UNIT] = theString;  // for JSON
   sprintf(theString, "%.1f ", sampleSetPtr->SensorData[SENSOR_2].temperature.value);
   jSonReadings[SENSOR_2_TEMP] = theString;     // for JSON
   sprintf(theString, "%s", Sensors.GetSensorTempUnits(sampleSetPtr->SensorData[SENSOR_2].temperature.unit).c_str());
   jSonReadings[SENSOR_2_TEMP_UNIT] = theString;   // for JSON

   vTaskDelay(1);

   sprintf(theString, "%.2f ", sampleSetPtr->SensorData[SENSOR_3].pressure.value);
   jSonReadings[SENSOR_3_PRESS] = theString;    // first for JSON
   sprintf(theString, "%s", Sensors.GetSensorPressUnits(sampleSetPtr->SensorData[SENSOR_3].pressure.unit).c_str());
   jSonReadings[SENSOR_3_PRESS_UNIT] = theString;  // for JSON
   sprintf(theString, "%.1f ", sampleSetPtr->SensorData[SENSOR_3].temperature.value);
   jSonReadings[SENSOR_3_TEMP] = theString;     // for JSON
   sprintf(theString, "%s", Sensors.GetSensorTempUnits(sampleSetPtr->SensorData[SENSOR_3].temperature.unit).c_str());
   jSonReadings[SENSOR_3_TEMP_UNIT] = theString;   // for JSON

   vTaskDelay(1);

   // The controller temperature is not included in the data file.
   //
//   sprintf(theString, "%.1f", MotorControl.DriveTemperature());
//   jSonReadings[CONTROLLER_TEMP] = theString;

   vTaskDelay(1);

   // The JSON string is now complete.
   //
   jsonString = JSON.stringify(jSonReadings);

   vTaskDelay(1);

   return jsonString;
}
