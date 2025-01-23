// CData.cpp
//
#include "Data.h"
#include "Eprom.h"
#include "EspNow.h"
#include "Sensors.h"
#include "Motor.h"
#include "MotorControl.h"
#include "SensorData.h"
#include "ManageSamples.h"

//#define  PRINT_DATA
#ifdef PRINT_DATA
#define  P_DATA(x)   x;
#else
#define  P_DATA(x)   // x
#endif


extern CEprom Eprom;
extern Flags Flag;
extern JSONVar jSonValues;
extern JSONVar jSonReadings;
extern CEspNow EspNow;
extern CSensors Sensors;
extern CMotor Motor;
extern CMotorControl MotorControl;

extern DynamicData Temp;

//-------------------------------------------------------------------
// String      JsonValue;   // matches the input name from html
// String      HtmlValue;   // item value
// boolean     Update;      // must update SPIFFS    
//
DataEntry DB[] = 
{
   {"p1_mins",    "10",    true},               //  DB_PERIOD1_MINS,        
   {"p1_cms",     "1",     true},               //  DB_PERIOD1_CMS,         
   {"p1_degs",    "0",     true},               //  DB_PERIOD1_DEGS,        
   {"p2_mins",    "35",    true},               //  DB_PERIOD2_MINS,        
   {"p2_cms",     "1",     true},               //  DB_PERIOD2_CMS,         
   {"p2_degs",    "0",     true},               //  DB_PERIOD2_DEGS,        
   {"p3_mins",    "32",    true},               //  DB_PERIOD3_MINS,        
   {"p3_cms",     "1",     true},               //  DB_PERIOD3_CMS,         
   {"p3_degs",    "0",    true},                //  DB_PERIOD3_DEGS,        

   {"rt_mins",    "120",   true},               //  DB_SINE_RUNTIME_MINS,   
   {"deepness",   "3.5",   true},               //  DB_DEPTH_MS,           

   {"interval",   "5",    true},                //  DB_SAMPLE_INTERVAL,     
   {"accelpwr",   "10",    true},               //  DB_ACCEL_POWER,         
   {"decelpwr",   "10",    true},               //  DB_DECEL_POWER,         
   {"runpwr",     "10",    true},               //  DB_RUN_POWER,           
   {"stoppwr",    "5",     true},               //  DB_STOP_POWER,          
   {"drivedia",   "47.74", true},               //  DB_DRUM_DIA,            
   {"drvratio",   "2.6",   true},               //  DB_DRIVE_RATIO,         
   
   {"temp",       "0",     false},              //  DB_CONTROLLER_TEMP,     
   {"rpm",        "0",     false},              //  DB_MOTOR_LIFT_RPM,      
   {"etime",      "0",     false},              //  DB_TEST_ELASED_TIME,    
   {"swstart",    "15",    true},               //  DB_SWEEP_START,         
   {"swstop",     "30",    true},               //  DB_SWEEP_STOP,          
   {"swdepth",    "3",     true},               //  DB_SWEEP_DEPTH,         
   {"swpre",      "1",     true},               //  DB_SWEEP_PREAMBLE,      
   {"swtime",     "15",    true},               //  DB_SWEEP_RUNTIME_MINS,  
};
int DB_Entries = sizeof(DB) / sizeof(DataEntry);

// A safe reply for too big index.
DataEntry Blank[]
{
   {"blank", "0", false}          // NULL return
};

DataEntry HD[] =
{
   {"descrip",    "test site description",  true},   // HD_TEST_DESCRIPTION
   {"detail1",    "well details",  true},            // HD_S1_DESCRIPTION
   {"detail2",    "well details",  true},            // HD_S2_DESCRIPTION
   {"detail3",    "well details",  true},            // HD_S3_DESCRIPTION
   {"filename",   "File.csv",      true},            // HD_DATAFILE_NAME  
};
int HD_Entries = sizeof(HD) / sizeof(DataEntry);

//-------------------------------------------------------------------
//
CData::CData(void)
{
   mOldValue = "0";
}

//-------------------------------------------------------------------
//
float CData::GetDataEntryNumericValue(uint32_t index)
{
   return DB[index].HtmlValue.toFloat();
}

//-------------------------------------------------------------------
//
String CData::GetDataEntryStringValue(uint32_t index)
{
   return DB[index].HtmlValue;
}

//-------------------------------------------------------------------
//
String CData::GetHeaderEntryString(uint32_t index)
{
   return HD[index].HtmlValue;
}

//-------------------------------------------------------------------
//
DataEntry CData::GetDataEntry(uint32_t index)
{
   if (index < DB_Entries)
   {
      return DB[index];
   }
   return Blank[0];
}

//-----------------------------------------------------------------------------
void CData::PutDataEntryValue(uint32_t index, String value)
{
   if (index < DB_Entries)
   {
      DB[index].HtmlValue = value;
   }
}

//-----------------------------------------------------------------------------
// Return the DB[i].JsonValue as a directory name.
//
String CData::CreateFileName(DataEntry* p)
{
   String name;
   
   name = "/";
   name += p->JsonValue;
   name += ".txt";
   P_DATA(Serial.printf("  %s\n", name.c_str()));
   return name;
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
void CData::RestoreSavedValues(void)
{
   uint32_t i;
   DataEntry* p;

   Serial.println(" Data.RestoreSavedValues");

   // Read the input DB values.
   //
   p = &DB[0];
   for (i = 0; i < DB_Entries; i++)
   {
      Restore(p++);
   }

   // Read the input HD values.
   //
   p = &HD[0];
   for (i = 0; i < HD_Entries; i++)
   {
      Restore(p++);
   }
}

//-----------------------------------------------------------------------------
//
void CData::Restore(DataEntry* p)
{
   String directory;
   String entry;

   directory = CreateFileName(p);       // assemble directory name
   entry = Eprom.ReadFile(directory);   // read value

   // If no entry exists it will be due to a new entry being created in
   // the DB database definition above. Here we create the first time
   // new entry in LittleFS.
   //
   if (entry == "NULL")
   {
      // If no entry, write the defnition value.
      //
      P_DATA(Serial.printf("Writing new entry <%s>\n", p->HtmlValue.c_str()));
      Eprom.WriteFile(directory.c_str(), p->HtmlValue.c_str());
   }
   else
   {
      P_DATA(Serial.printf("Using LittleFS entry <%s>\n", p->HtmlValue.c_str()));
      p->HtmlValue = entry;                     // copy to database
   }
}

//-----------------------------------------------------------------------------
// Called from the HandleWebSocketMessage function for the case where indexing
// word is "isheader". Locate the matching entry in HD and update both the HD
// database and the matching Eprom database.
//
void CData::MatchAndUpdateHeaders(String &name, String &newValue)
{
   Serial.printf(" Data.MatchAndUpdateHeaders: <%s> <%s>\n", name.c_str(), newValue.c_str());

   for (int i = 0; i < HD_Entries; i++)
   {
      if (name == HD[i].JsonValue)
      {
         String directory;

         // If a test is running then certain parameters that control the test
         // cannot be altered.
         //
         switch (i)
         {
               // While a test is running, these values cannot be altered.
               //
            case HD_TEST_DESCRIPTION:
            case HD_S1_DESCRIPTION:
            case HD_S2_DESCRIPTION:
            case HD_S3_DESCRIPTION:
            case HD_DATAFILE_NAME:

               if (Flag.SineTestRunning | Flag.SweepTestRunning)
               {
                  return;
               }
               break;

            default:
               return;
               break;
         }

         directory = CreateFileName(&HD[i]);
         mOldValue = Eprom.ReadFile(directory);
         P_DATA(Serial.printf(" Item <%s> LittleFS value: <%s>\n", name.c_str(), mOldValue.c_str()));

         // Check if this value differs from the database. Update any entry that
         // has changed for subsequent updates to the controller.
         //
         if (newValue == mOldValue)
         { 
            // Same as Eprom entry - no need to change.
            HD[i].Update = false; 
         } 
         else 
         {
            HD[i].Update = true;
            HD[i].HtmlValue = newValue;
         }
          
         // New header text is simply written and forgotten.
         //
         P_DATA(Serial.printf("Eprom entry %s new value: <%s>\n", name.c_str(), newValue.c_str()));
         Eprom.WriteFile(directory, newValue);
      }
   }
}


//-----------------------------------------------------------------------------
// Called from the HandleWebSocketMessage function for the case where indexing
// word is "isdata" or "istext". Locate the matching entry in DB and update both 
// the DB database and the matching Eprom database.
//
void CData::MatchAndUpdateData(String &name, String &newValue)
{
   Serial.printf(" Data.MatchAndUpdateData: <%s> <%s>\n", name.c_str(), newValue.c_str());
   for (int i = 0; i < DB_Entries; i++)
   {
      if (name == DB[i].JsonValue)
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
            case DB_DEPTH_MS:
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

         directory = CreateFileName(&DB[i]);
         mOldValue = Eprom.ReadFile(directory);
         P_DATA(Serial.printf(" Item %s New value %s  Old value: %s\n", name.c_str(), newValue.c_str(), mOldValue.c_str()));

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
         // we call ActionDataParameter.
         //
         if (ActionDataParameter(i))
         {
            // Only update Eprom if ActionDataParameter returns true.
            //
            P_DATA(Serial.printf("Eprom entry %s new value: %s\n", name.c_str(), newValue.c_str()));
            Eprom.WriteFile(directory, newValue);
         }
      }
   }
}

//----------------------------------------------------------------------------------------------
// Action a parameter change. The index is the database DB[] index. On arrival here the 
// corresponding DB ands Eprom value has been either validated or updated and the 
// DB[index].Update boolean will reflect which one.
//
boolean CData::ActionDataParameter(int index)
{
   String jsonString;

   Serial.printf("ActionDataParameter %d: %s = %s\n", index, DB[index].JsonValue, DB[index].HtmlValue.c_str());

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
         case DB_DEPTH_MS:
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

         default:
            Serial.println("NO MATCH.");
            break;
      }
   }

   return true;
}

//-----------------------------------------------------------------------------
// Read the current jSonValues to the client.
//
String CData::GetCurrentEpromDataValues(void)
{
   DataEntry* p;
   String jsonString;

   Serial.println(" Data.GetCurrentEpromDataValues");

   // Initialise the js myObj jSonValues. Note the DB indexes must match
   // the actual inputs, and the xxx in xxxValues must match the
   // html <text> input name.
   //
   P_DATA(Serial.println("DB values"));
   p = &DB[0];
   for (int i = 0; i < DB_Entries; i++)
   {
      jSonValues[p->JsonValue] = Eprom.ReadFile(CreateFileName(p));
      p++;
   }

   jsonString = JSON.stringify(jSonValues);

   return jsonString;
}

//-----------------------------------------------------------------------------
// Read the current jSonValues to the client.
//
String CData::GetCurrentEpromHeaderValues(void)
{
   DataEntry* p;
   String jsonString;

   Serial.println(" Data.GetCurrentEpromHeaderValues");

   // Initialise the js myObj jSonValues. Note the DB indexes must match
   // the actual inputs, and the xxx in xxxValues must match the
   // html <text> input name.
   //
   p = &HD[0];
   for (int i = 0; i < HD_Entries; i++)
   {
      jSonValues[p->JsonValue] = Eprom.ReadFile(CreateFileName(p));
      p++;
   }

   jsonString = JSON.stringify(jSonValues);

   return jsonString;
}

//-----------------------------------------------------------------------------
// Respond to a client Submit request.
//
void CData::UpdateDatabase(const AsyncWebParameter* q)
{
   DataEntry* p;

   Serial.println(" Data.UpdateDatabase");

   p = &DB[0];
   for (int i = 0; i < DB_Entries; i++)
   {
//      Serial.printf("%s  %s\n", q->name().c_str(), p->JsonValue.c_str());
      if (q->name() == p->JsonValue)
      {
         Update(p, q);
         break;
      }
      p++;
   }
   p = &HD[0];
   for (int i = 0; i < HD_Entries; i++)
   {
      if (q->name() == p->JsonValue)
      {
         Update(p, q);
         break;
      }
      p++;
   }
}

//-----------------------------------------------------------------------------
//
void CData::Update(DataEntry* p, const AsyncWebParameter* q)
{
   String directoryName;

   directoryName = CreateFileName(p);

   P_DATA(Serial.printf("%s  %s\n", p->value().c_str(), Spiff.ReadFile(SPIFFS, p->PathName)));
   P_DATA(Serial.printf("%s\n", p->name().c_str()));

   // Check if this value differs from the database. Update any entry that
   // has changed for subsequent updates to the controller.
   //
   if (q->value() == Eprom.ReadFile(directoryName))
   { 
      // Same as eprom entry - no need to change.
      p->Update = false; 
   } 
   else 
   {
      // New value. Flag the entry for later Controller update.
      P_DATA(Serial.printf("Entry %s for Controller update\n", q->name().c_str()));
      p->Update = true;

      // Update eprom.
      p->HtmlValue = q->value().c_str();
      Eprom.DeleteFile(directoryName);
      Eprom.WriteFile(directoryName, p->HtmlValue);
   }
}

//-----------------------------------------------------------------------------
// Collect here all data that should be displayed at sample update time.
//
String GetCurrentValuesForClient(SensorSampleType* sampleSetPtr)
{
   String jsonString;
   char theString[50];

   Serial.println(" GetCurrentValuesForClient");

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
