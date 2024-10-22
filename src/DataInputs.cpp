// CDataInputs.cpp
//
#include "DataInputs.h"



// This is the reference database that contains all input variables.
//  InputParameter   matches the html name
//  HtmlValue        item value (a String)
//  PathName         SPIFFS name where stored
//  Update           must update SPIFFS    
//
// Test numbers are:
// 40,0,20
// 35,0,20
// 32,90,20
// 300
//
DataEntry DB[] = 
{
   {PERIOD1_MINS,          "40",  NULL,  true},          // P1_MINS
   {PERIOD1_CMS,            "1",  NULL,  true},          // P1_CMS
   {PERIOD1_DEGS,           "0",  NULL,  true},          // P1_DEGS
   {PERIOD2_MINS,          "35",  NULL,  true},          // P2_MINS
   {PERIOD2_CMS,            "1",  NULL,  true},          // P2_CMS
   {PERIOD2_DEGS,           "0",  NULL,  true},          // P2_DEGS
   {PERIOD3_MINS,          "32",  NULL,  true},          // P3_MINS
   {PERIOD3_CMS,            "1",  NULL,  true},          // P3_CMS
   {PERIOD3_DEGS,          "90",  NULL,  true},          // P3_DEGS

   {SINE_TIME_MINS,        "120",  NULL,  true},          // 9
   {DEEPNESS,              "50",  NULL,  true},          // 10 (cms)

   {TEST_DESCRIPTION, "test description", NULL, true},   // 11
   {S1_DESCRIPTION,   "details",  NULL,  true},          // 12
   {S2_DESCRIPTION,   "details",  NULL,  true},          // 13
   {S3_DESCRIPTION,   "details",  NULL,  true},          // 14

   {SAMPLE_INTERVAL,       "60",  NULL,  true},          // (minutes)

   {ACCEL_POWER,           "10",  NULL,  true},          // 16
   {DECEL_POWER,           "10",  NULL,  true},          // 17
   {RUN_POWER,             "10",  NULL,  true},          // 18
   {STOP_POWER,             "5",  NULL,  true},          // 19
   {DRUM_DIA,         "47.74",  NULL,  true},          // 20
   {DRIVE_RATIO,          "2.6",  NULL,  true},          // 21
   {CONTROLLER_TEMP,        "0",  NULL,  false},         // 22

   {MOTOR_LIFT_RPM,        "20",  NULL,  false},         // 23

   {TEST_ELAPSED_TIME,      "0",  NULL,  false},         // 24

   {SWEEP_START,           "15",  NULL,  true},          // 25
   {SWEEP_STOP,            "30",  NULL,  true},          // 26
   {SWEEP_DEPTH,          "100",  NULL,  true},          // 27
   {SWEEP_PREAMBLE,         "1",  NULL,  true},          // 28
   {SWEEP_TIME_MINS,       "15",  NULL,  true},          // 29

   {DATAFILE_NAME,         "MyData", NULL, true},        // 30
};
int DB_Entries = sizeof(DB) / sizeof(DataEntry);

DataEntry Blank[]
{
   {"blank", "0", "blank", false}          // NULL return
};

//-------------------------------------------------------------------
//
CDataInputs::CDataInputs(void)
{
}

//-------------------------------------------------------------------
//
void CDataInputs::SetInputPaths(void)
{
   for (int i=0; i<DB_Entries; i++)
   {
      String name;
      char* pathName;

      // Using new here is safe as this function is only called once
      // at program strtup.
      //
      pathName = new char [MAX_PATHNAME_LENGTH];
      name = DB[i].InputParameter;
      strcpy(pathName, "/");
      strcat(pathName, name.c_str());
      strcat(pathName, ".txt");
      DB[i].PathName = pathName;
      Serial.printf("%s\n", DB[i].PathName);
   }
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
