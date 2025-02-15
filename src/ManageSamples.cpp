// ManageSamples.cpp
//
#include "ManageSamples.h"
#include "Buttons.h"
#include "SDCard.h"
#include "Sensors.h"
#include "Motor.h"
#include "MotorControl.h"
#include "Data.h"
#include "Network.h"
#include "Timers.h"
#include "LimaxTime.h"
#include "WellSimulator.h"

//#define  PRINT_SAMPLES
#ifdef PRINT_SAMPLES
#define  P_SAMP(x)   x;
#else
#define  P_SAMP(x)   // x
#endif


extern AsyncEventSource events;
extern CButtons Buttons;
extern CSDCard SDCard;
extern CSensors Sensors;
extern CMotor Motor;
extern CMotorControl MotorControl;
extern CNetwork Network;
extern CData Data;
extern CTimers Timers;
extern CLimaxTime LimaxTime;

extern xQueueHandle xNewSampleQueue;
extern void NotifyClients(String state);

extern String DataFilename;    // contains the current data file path
extern JSONVar jSonReadings;
extern Flags Flag;

extern CManageSamples ManageSamples;
extern SensorSampleType SampleSet;
extern SemaphoreHandle_t xSDCardAccessMutex;

extern DynamicData Temp;

extern float DummyTime;
extern uint32_t TimeInSeconds;

#ifdef SIMULATING
extern CWellSimulator WellSimulator;
#endif


//-------------------------------------------------------------------
//
CManageSamples::CManageSamples(void)
:  mMonitorEntries(0),
   mTempFileContents(0)
{
}

//-----------------------------------------------------------------------------
//
void CManageSamples::SendSummary(void)
{
   String jsonString;

//   Serial.printf("Sending summary. %d entries\n", Temp.entries);
   for (int i=0; i<Temp.entries; i++)
   {
      jsonString = ManageSamples.GetCurrentPlotValuesForClient(&SampleSet, i);
      events.send(jsonString.c_str(), "new_values", millis());  // send to client
   }
   // events.send("ping", NULL, millis());
   // events.send(jsonString.c_str(), "just_readings", millis());  // send to client
}

//-----------------------------------------------------------------------------
// This task receives the sample dataset from CNetwork via the <xNewSampleQueue>
// into the global <SensorSampleType> structure called <SampleSet>. 
// This structure is first unpacked as a jsonString to send to the client to 
// display the current sample, and then unpacked again for writing to the
// SDCard for long-term storage.
//
void CManageSamples::Task_ManageSampleResults(void* parameter)
{
   uint16_t entries;
   String mSDCardLine;
   String jsonString;
   String line;
   char theString[50];
   JSONVar values;
   char text[20];
   File fd;
   int i, j;
   uint16_t n;
   float slugDepth;

   Serial.println("Task_ManageSampleResults running...");

   while (true)
   {
      // Wait for an incoming <SensorDataSet> message containing the results
      // of the lastest well node set samples. 
      //
      xQueueReceive(xNewSampleQueue, &SampleSet, portMAX_DELAY);

      P_SAMP(Serial.println("Task_ManageSampleResults: Sample Set arrived"));

      //---------------------------------------------------------------------------------
      // Process the sample set of data. Before sending to the client, we remove
      // any unexpected zero entries.
      //
      ManageSamples.MonitorForZeroes(&SampleSet);

      // Serial.println(SampleSet.actualEntries);
      // for(int i=0; i<SampleSet.actualEntries; i++)
      // {
      //    Serial.printf("%f\n", SampleSet.SensorData[i].pressure.value);
      // }
      slugDepth = MotorControl.GetSlugDepth();

      //-------------------------------------------------------------------------------------
      // Because the SDCard contents can also be written to the client, SDCard access
      // is protected from those writes by a semaphore mutex.
      //
      if (xSemaphoreTake(xSDCardAccessMutex, (TickType_t)SDCARD_ACCESS_MUTEX) == pdTRUE)
      {
         entries = Temp.entries;

         // // Manage the refresh data base.
         // //
         Temp.sample[entries].value[0] = MotorControl.GetElapsedSeconds();

         // We post-process the 
         Temp.sample[entries].value[1] = slugDepth;
         Temp.sample[entries].value[2] = SampleSet.SensorData[0].pressure.value;
         Temp.sample[entries].value[3] = SampleSet.SensorData[1].pressure.value;
         Temp.sample[entries].value[4] = SampleSet.SensorData[2].pressure.value;

         entries++;

         // If the Temp data structure has filled, push the oldest data out.
         //
         if (entries > DISPLAY_SAMPLES)
         {
            for (i=1; i<entries; i++)
            {
               for (j=0; j<5; j++)
               {
                  Temp.sample[i-1].value[j] = Temp.sample[i].value[j];
               }
            }
            entries = DISPLAY_SAMPLES;
         }

         Temp.entries = entries;

         // // Print the current temp file entries.
         // //
         // Serial.printf("Current Temp file entries: %d\n", entries);
         // for (i=0; i<entries; i++)
         // {
         //    for (j=0; j<5; j++)
         //    {
         //       Serial.printf("%8.4f  ", Temp.sample[i].value[j]);
         //    }
         //    Serial.println();
         // }
         // Serial.println();

         vTaskDelay(1);
         
         // Ensure the min scale for the chart y axis.
         //
         // sprintf(text, "%d", (int32_t)MotorControl.GetAmplitude());
         // values["minValue"] = String(text);
         // jsonString = JSON.stringify(values);
         // events.send(jsonString.c_str(), "set_min_value", millis());
         // vTaskDelay(1);

         if (Flag.RePlot)
         {
            Flag.RePlot = false;

            ManageSamples.SendSummary();
         }
         else
         {
//            Serial.println("Plotting");
            
            jsonString = ManageSamples.GetCurrentPlotValuesForClient(&SampleSet, Temp.entries - 1);
//         Serial.println(Temp.entries);
//         Serial.println(jsonString);
            events.send(jsonString.c_str(), "new_values", millis());  // send to client
            // events.send("ping", NULL, millis());
            // events.send(jsonString.c_str(), "just_readings", millis());  // send to client
         }

         //---------------------------------------------------------------------------------
         // Sample has been sent to the client. Check whether we are recording and so
         // need to also write to the SDCard..
         // 
         if (Flag.Recording)
         {
            // Here we re-assemble the sample set as required by the SDCard.
            //
//            Serial.println("Writing to SDCard");
#ifndef DATALOGGER_IOT      
//           theSPI.beginTransaction(SPISettings(Motor.GetSpiClk(), MSBFIRST, SPI_MODE0));
//           SD.begin(SDCARD_CS, theSPI);
#endif

            // The GetTime function will eventually return the current time that
            // was downloaded from the laptop at startup.
            //
            sprintf(theString, "%s", MotorControl.SecondsToTime(LimaxTime.GetLimaxTime()));

            // Write the current time as the first entry in the line.
            //
            mSDCardLine = theString;        
            mSDCardLine.concat(",");

            // Write the current slug position.
            //
            sprintf(theString, "%8.6e,", slugDepth);  // note trailing comma
            mSDCardLine.concat(theString);

            // Add in the pressure and temperature data from each sensor.
            //
            sprintf(theString, "%8.6e,", SampleSet.SensorData[SENSOR_1].pressure.value);
            mSDCardLine.concat(theString);
               
            vTaskDelay(1);

            sprintf(theString, "%8.6e,", SampleSet.SensorData[SENSOR_2].pressure.value);
            mSDCardLine.concat(theString);
               
            vTaskDelay(1);

            sprintf(theString, "%8.6e\n", SampleSet.SensorData[SENSOR_3].pressure.value);
            mSDCardLine.concat(theString);
            P_SAMP(Serial.print(mSDCardLine));
            SDCard.AppendFile(DataFilename, mSDCardLine);
               
            vTaskDelay(1);
#ifndef DATALOGGER_IOT
//            SD.end();
//            theSPI.endTransaction();
#endif      
            }

         // return the mutex.
         //
         xSemaphoreGive(xSDCardAccessMutex);
      }
      else
      {
         Serial.println("COULD NOT ACCESS MUTEX IN TIME");
      }
   }

   vTaskDelete(NULL);
}

//-----------------------------------------------------------------------------
// Write the site data as preamble to the data file. Uses the filename
// stored in DB[DATAFILE_NAME].
//
void CManageSamples::WriteSitePreamble(void)
{
   char theString[30];
   String mSDCardLine;

   // First get and verify the datafile name.
   //
   DataFilename = "/LimaxData/";
   DataFilename.concat(Data.GetHeaderEntryString(HD_DATAFILE_NAME));

   // Assemble data file header. Start with the site and individual well
   // text descriptions.
   //
   // Note this next starts a new version of the data.
   //
   mSDCardLine = "Limax site test data file.\n";
   SDCard.WriteFile(DataFilename, mSDCardLine);
   mSDCardLine = Data.GetHeaderEntryString(HD_TEST_DESCRIPTION);
   mSDCardLine.concat("\n");
   SDCard.AppendFile(DataFilename, mSDCardLine);
   mSDCardLine = Data.GetHeaderEntryString(HD_S1_DESCRIPTION);
   mSDCardLine.concat("\n");
   SDCard.AppendFile(DataFilename, mSDCardLine);
   mSDCardLine = Data.GetHeaderEntryString(HD_S2_DESCRIPTION);
   mSDCardLine.concat("\n");
   SDCard.AppendFile(DataFilename, mSDCardLine);
   mSDCardLine = Data.GetHeaderEntryString(HD_S3_DESCRIPTION);
   mSDCardLine.concat("\n");
   SDCard.AppendFile(DataFilename, mSDCardLine);

   // The mac addresses of the individual relay box processors are recorded.
   //
   Network.ByteToMACAddress((uint8_t*)&SampleSet.SensorData[1].mac, theString);
   mSDCardLine = theString;
   mSDCardLine.concat(",");
   Network.ByteToMACAddress((uint8_t*)&SampleSet.SensorData[SENSOR_2].mac, theString);
   mSDCardLine.concat(theString);
   mSDCardLine.concat(",");
   Network.ByteToMACAddress((uint8_t*)&SampleSet.SensorData[SENSOR_3].mac, theString);
   mSDCardLine.concat(theString);
   mSDCardLine.concat("\n");

   SDCard.AppendFile(DataFilename, mSDCardLine);
   P_SAMP(Serial.print(mSDCardLine.c_str()));

   vTaskDelay(1);

   // Now record the individual units of each sensor.
   //
   mSDCardLine = "seconds,m,";
   sprintf(theString, "%s,", Sensors.GetSensorPressUnits(SampleSet.SensorData[SENSOR_1].pressure.unit).c_str());
   mSDCardLine.concat(theString);
   sprintf(theString, "%s,", Sensors.GetSensorPressUnits(SampleSet.SensorData[SENSOR_2].pressure.unit).c_str());
   mSDCardLine.concat(theString);
   sprintf(theString, "%s,", Sensors.GetSensorPressUnits(SampleSet.SensorData[SENSOR_3].pressure.unit).c_str());
   mSDCardLine.concat(theString);
   mSDCardLine.concat("\n");

   SDCard.AppendFile(DataFilename, mSDCardLine);
   P_SAMP(Serial.print(mSDCardLine.c_str()));
   
   vTaskDelay(1);
}

//-----------------------------------------------------------------------------
// Collect here all data that should be displayed at sample update time.
//
String CManageSamples::GetCurrentPlotValuesForClient(SensorSampleType* SampleSetPtr, uint32_t index)
{
   String jsonString;
   char theString[50];
   String temp;
   uint32_t i, entries;
   
//   P_SAMP(Serial.println(" ManageSamples.GetCurrentPlotValuesForClient"));

   // The display shows the last DISPLAY_SAMPLES. Here we extract the <index> contents
   // of the <Temp> datastructure and return it as a JSON string. This is an array of 
   // 4 values per entry, the slug depth followed by the three pressures in order.
   // Because the the whole data structure contents are written at every sample,
   // we can extract the <units> from the sample set (which contains just the sample).
   //
   entries = Temp.entries;

   if (entries > 0)
   {
      // Assemble the ith entry as a json string. 
      //
      i = index;
      sprintf(theString, "%s", MotorControl.SecondsToTime(MotorControl.GetElapsedSeconds()));
      jSonReadings[Data.GetDataEntry(DB_TEST_ELAPSED_TIME).JsonValue] = theString;      // time

      sprintf(theString, "%.2f", Data.GetDataEntryNumericValue(DB_CONTROLLER_TEMP));
      jSonReadings[Data.GetDataEntry(DB_CONTROLLER_TEMP).JsonValue] = theString;

      sprintf(theString, "%e",  Temp.sample[i].value[1]);                         // slug depth
      jSonReadings[SLUG_DEPTH] = theString;

      // Insert the pressure and temperature data from each sensor along
      // with their corresponding units.
      //
      sprintf(theString, "%e ", Temp.sample[i].value[2]);
      jSonReadings[SENSOR_1_PRESS] = theString; 
      sprintf(theString, "%s", Sensors.GetSensorPressUnits(SampleSetPtr->SensorData[SENSOR_1].pressure.unit).c_str());
      jSonReadings[SENSOR_1_PRESS_UNIT] = theString;

      sprintf(theString, "%e ", Temp.sample[i].value[3]);
      jSonReadings[SENSOR_2_PRESS] = theString; 
      sprintf(theString, "%s", Sensors.GetSensorPressUnits(SampleSetPtr->SensorData[SENSOR_2].pressure.unit).c_str());
      jSonReadings[SENSOR_2_PRESS_UNIT] = theString;

      sprintf(theString, "%e ", Temp.sample[i].value[4]);
      jSonReadings[SENSOR_3_PRESS] = theString;
      sprintf(theString, "%s", Sensors.GetSensorPressUnits(SampleSetPtr->SensorData[SENSOR_3].pressure.unit).c_str());
      jSonReadings[SENSOR_3_PRESS_UNIT] = theString;

      jsonString = JSON.stringify(jSonReadings);
   }
   else
   {
      sprintf(theString, "%d", entries);
      jSonReadings[Data.GetDataEntry(DB_TEST_ELAPSED_TIME).JsonValue] = theString;      // time
      jSonReadings[SLUG_DEPTH] = theString;
      jSonReadings[SENSOR_1_PRESS] = theString;    // first for JSON             pressure
      jSonReadings[SENSOR_2_PRESS] = theString;    // first for JSON             pressure
      jSonReadings[SENSOR_3_PRESS] = theString;    // first for JSON             pressure
      jSonReadings[Data.GetDataEntry(DB_CONTROLLER_TEMP).JsonValue] = Data.GetDataEntryNumericValue(DB_CONTROLLER_TEMP);
      vTaskDelay(1);

      // The JSON string is now complete.
      //
      jsonString = JSON.stringify(jSonReadings);
   }

   vTaskDelay(1);

   return jsonString;
}

//-----------------------------------------------------------------------------
// Collect here all data that should be displayed at sample update time.
//
String CManageSamples::GetCurrentValuesForClient(SensorSampleType* SampleSetPtr)
{
   String jsonString;
   char theString[50];
   String temp;
   uint32_t i, entries;

   Serial.println(" ManageSamples.GetCurrentValuesForClient");

//    // The display shows the last DISPLAY_SAMPLES. We first read the current contents
//    // of the <temp> file on the SDcard. This is an array of 4 values per entry, the
//    // slug depth followed by the three pressures in order.
//    //
//    entries = Temp.entries;

//    if (entries > 0)
//    {
//       // Assemble the ith entry as a json string. 
//       //
//       i = index;
// Serial.println(i);
//       sprintf(theString, "%.0f", Temp.sample[i].value[0]);
//       jSonReadings[Data.GetDataEntry(DB_TEST_ELAPSED_TIME).JsonValue] = theString;      // time

//       sprintf(theString, "%.3f",  Temp.sample[i].value[1]);                         // slug depth
//       jSonReadings[SLUG_DEPTH] = theString;

//       // Insert the pressure and temperature data from each sensor along
//       // with their corrosponding units.
//       //
//       sprintf(theString, "%.2f ", SampleSetPtr->SensorData[SENSOR_1].pressure.value);
//       jSonReadings[SENSOR_1_PRESS] = theString;       // pressure
//       Serial.printf("=> %d %s\n", SampleSetPtr->SensorData[SENSOR_1].pressure.unit, Sensors.GetSensorPressUnits(SampleSetPtr->SensorData[SENSOR_1].pressure.unit).c_str());
//       jSonReadings[SENSOR_1_PRESS_UNIT] = theString;  // for JSON)
//       sprintf(theString, "%s", Sensors.GetSensorPressUnits(SampleSetPtr->SensorData[SENSOR_1].pressure.unit).c_str());
//       jSonReadings[SENSOR_1_PRESS_UNIT] = theString;  // for JSON

//       sprintf(theString, "%.2f ", Temp.sample[i].value[3]);
//       jSonReadings[SENSOR_2_PRESS] = theString;    // first for JSON                pressure

//       sprintf(theString, "%.2f ", Temp.sample[i].value[4]);
//       jSonReadings[SENSOR_3_PRESS] = theString;    // first for JSON                pressure

//       jsonString = JSON.stringify(jSonReadings);
//    }
//    else
//    {
//       sprintf(theString, "%d", entries);
//       jSonReadings[Data.GetDataEntry(DB_TEST_ELAPSED_TIME).JsonValue] = theString;      // time
//       jSonReadings[SLUG_DEPTH] = theString;
//       jSonReadings[SENSOR_1_PRESS] = theString;    // first for JSON             pressure
//       jSonReadings[SENSOR_2_PRESS] = theString;    // first for JSON                pressure
//       jSonReadings[SENSOR_3_PRESS] = theString;    // first for JSON                pressure
//       vTaskDelay(1);

//       // The JSON string is now complete.
//       //
//       jsonString = JSON.stringify(jSonReadings);
//    }

//    // vTaskDelay(1);

//    // // Now send the data to the client as a jsonString. 
//    // //
//    // Serial.print(" Sending to web site: ");
//    // events.send("ping", NULL, millis());
//    // events.send(jsonString.c_str(), "new_values", millis());  // send to client
//    // Serial.println("Sent");

//    vTaskDelay(1);

   return jsonString;





//    vTaskDelay(1);

// //   jSonReadings[Data.GetDataEntry(DB_TEST_ELAPSED_TIME).JsonValue] = MotorControl.SecondsToTime(Timers.GetTestElapsedSeconds()).c_str();
//    sprintf(theString, "%d", dummyTime);
//    jSonReadings[Data.GetDataEntry(DB_TEST_ELAPSED_TIME).JsonValue] = theString;

//    sprintf(theString, "%.3f",  MotorControl.GetSlugDepth());
//    jSonReadings[SLUG_DEPTH] = theString;

//    // Insert the pressure and temperature data from each sensor along
//    // with their corrosponding units.
//    //
//    sprintf(theString, "%.2f ", SampleSetPtr->SensorData[SENSOR_1].pressure.value);
//    jSonReadings[SENSOR_1_PRESS] = theString;       // first for JSON
//    sprintf(theString, "%s", Sensors.GetSensorPressUnits(SampleSetPtr->SensorData[SENSOR_1].pressure.unit).c_str());
//    jSonReadings[SENSOR_1_PRESS_UNIT] = theString;  // for JSON
//    sprintf(theString, "%.1f ", SampleSetPtr->SensorData[SENSOR_1].temperature.value);
//    jSonReadings[SENSOR_1_TEMP] = theString;        // for JSON
//    sprintf(theString, "%s", Sensors.GetSensorTempUnits(SampleSetPtr->SensorData[SENSOR_1].temperature.unit).c_str());
//    jSonReadings[SENSOR_1_TEMP_UNIT] = theString;   // for JSON

//    vTaskDelay(1);

//    sprintf(theString, "%.2f ", SampleSetPtr->SensorData[SENSOR_2].pressure.value);
//    jSonReadings[SENSOR_2_PRESS] = theString;    // first for JSON
//    sprintf(theString, "%s", Sensors.GetSensorPressUnits(SampleSetPtr->SensorData[SENSOR_2].pressure.unit).c_str());
//    jSonReadings[SENSOR_2_PRESS_UNIT] = theString;  // for JSON
//    sprintf(theString, "%.1f ", SampleSetPtr->SensorData[SENSOR_2].temperature.value);
//    jSonReadings[SENSOR_2_TEMP] = theString;     // for JSON
//    sprintf(theString, "%s", Sensors.GetSensorTempUnits(SampleSetPtr->SensorData[SENSOR_2].temperature.unit).c_str());
//    jSonReadings[SENSOR_2_TEMP_UNIT] = theString;   // for JSON

//    vTaskDelay(1);

//    sprintf(theString, "%.2f ", SampleSetPtr->SensorData[SENSOR_3].pressure.value);
//    jSonReadings[SENSOR_3_PRESS] = theString;    // first for JSON
//    sprintf(theString, "%s", Sensors.GetSensorPressUnits(SampleSetPtr->SensorData[SENSOR_3].pressure.unit).c_str());
//    jSonReadings[SENSOR_3_PRESS_UNIT] = theString;  // for JSON
//    sprintf(theString, "%.1f ", SampleSetPtr->SensorData[SENSOR_3].temperature.value);
//    jSonReadings[SENSOR_3_TEMP] = theString;     // for JSON
//    sprintf(theString, "%s", Sensors.GetSensorTempUnits(SampleSetPtr->SensorData[SENSOR_3].temperature.unit).c_str());
//    jSonReadings[SENSOR_3_TEMP_UNIT] = theString;   // for JSON

//    vTaskDelay(1);

//    // The controller temperature is not included in the data file.
//    //
//    sprintf(theString, "%.1f", MotorControl.DriveTemperature());
//    jSonReadings[Data.GetDataEntry(DB_CONTROLLER_TEMP).JsonValue] = theString;

//    vTaskDelay(1);

   // // The JSON string is now complete.
   // //
   // jsonString = JSON.stringify(jSonReadings);

   // vTaskDelay(1);

   // return jsonString;
}

#ifdef SIMPLE_ZERO_FIX
//-----------------------------------------------------------------------------
// Provide for a missing pressure reading - simple provide the last reading.
//
void CManageSamples::MonitorForZeroes(SensorSampleType* SampleSetPtr)
{
   int i;

   // Check for any zero entries. If any replace with the respective
   // node previous value.
   //
   if (mMonitorEntries > 1)
   {
      for (i=0; i < NUM_SENSORS; i++)
      {
         if (SampleSetPtr->SensorData[i].pressure.value == 0)
         {
            SampleSetPtr->SensorData[i].pressure.value = mPreviousPressures[i];
         }
      }
   }
   mMonitorEntries++;

   // Save the current set.
   //
   for (i=0; i < NUM_SENSORS; i++)
   {
      mPreviousPressures[i] = SampleSetPtr->SensorData[i].pressure.value;
   }
}
#else
//-----------------------------------------------------------------------------
// Provide a data-based estimate for a missing pressure reading.
//
void CManageSamples::MonitorForZeroes(SensorSampleType* SampleSetPtr)
{
   int i, j;
   float Sx, Sy, Sxy, Sx2;

//   digitalWrite(ESP_LED, HIGH);

   Serial.printf("MonitorForZeroes  %d\n", mMonitorEntries);

   // Copy the current pressure readings from the sample set to our Monitor dataset
   // in the mMonitorEntries position up until the mMonitorEntries index equals 
   // the DB_MONITOR_ENTRIES.
   //
   mMonitor[0].pressure[mMonitorEntries] = SampleSetPtr->SensorData[0].pressure.value;
   mMonitor[1].pressure[mMonitorEntries] = SampleSetPtr->SensorData[1].pressure.value;
   mMonitor[2].pressure[mMonitorEntries] = SampleSetPtr->SensorData[2].pressure.value;

   // We examine these new entries for an unexpected zero. When one occurs it
   // is replaced by an estimate based on y = mx+c
   //
   for (j=0; j < NUM_SENSORS; j++)
   {
      if (Network.IsNodePresent(j))
      {
         if (mMonitor[j].pressure[mMonitorEntries] == 0)
         {
            float newY;

            // When a zero does occur, we must also fix the sampled
            // data entry as well.
            //
            newY = mMonitor[j].m * (DB_MONITOR_ENTRIES - 1) + mMonitor[j].c;
            SampleSetPtr->SensorData[j].pressure.value = newY;
            mMonitor[j].pressure[mMonitorEntries] = newY;
            Serial.printf("Zero entry for channel %d replaced with y=mx+c: %f\n", j, newY);
         }
      }
   }

   mMonitorEntries++;

   if (mMonitorEntries == DB_MONITOR_ENTRIES)
   {
      Serial.println(" Updating monitor values");
      // We now compute the linear least squares terms for these entries. We 
      // ignore the possibility that a particular entry may be an unexpected
      // zero, as this will be smoothed out in time, and a channel that is
      // non-existant will also come good with time.
      //
      for (j=0; j < NUM_SENSORS; j++)
      {
         if (Network.IsNodePresent(j))
         {
            Sxy = Sy = Sx = Sx2 = 0;

            for (i=0; i < DB_MONITOR_ENTRIES - 1; i++)
            {
               Sx += i + 1;
               Sx2 += (i+1)*(i+1);
               Sy += mMonitor[j].pressure[i];
               Sxy += mMonitor[j].pressure[i] * (i + 1);
            }
               
            mMonitor[j].m = (Sxy - (Sx * Sy) / (DB_MONITOR_ENTRIES - 1)) / (Sx2 - (Sx * Sx) / (DB_MONITOR_ENTRIES - 1));
            mMonitor[j].c = (Sx * Sxy - Sy * Sx2) / (Sx * Sx - (DB_MONITOR_ENTRIES - 1) * Sx2);
         }
      }

      // Debug printout...
      for (j=0; j<NUM_SENSORS; j++)
      {
         float Sxy = 0;
         Sy = 0;
         for (i=0; i<DB_MONITOR_ENTRIES; i++)
         {
            Serial.printf("%7.4f  ", mMonitor[j].pressure[i]);
            Sxy += mMonitor[j].pressure[i] * (i + 1);
            Sy += mMonitor[j].pressure[i];
         }
         Serial.println("");
         Serial.printf(" %.6f\n", mMonitor[j].m * 5.0 + mMonitor[j].c);
      }

      // We have computed the m and c values. Next we shift all values
      // back one to make room for the next new sample.
      //
      for (j=0; j < NUM_SENSORS; j++)
      {
         for (i=1; i < DB_MONITOR_ENTRIES; i++)
         {
            mMonitor[j].pressure[i-1] = mMonitor[j].pressure[i];
         }
      }

      // Now decrement the entry index.
      //
      mMonitorEntries--;
   }
}
#endif
