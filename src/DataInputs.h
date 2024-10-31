// CDataInputs.h
//
#include "Main.h"


//-----------------------------------------------------------------------------
// Analog structure definitions.
//
// typedef struct
// {
//    const char  *InputParameter;  // matches the input name from html
//    String      HtmlValue;        // item value
//    char        *PathName;        // SPIFFS name where stored
//    boolean     Update;           // must update SPIFFS    
// } DataEntry;
typedef struct
{
   String      InputParameter;   // matches the input name from html
   String      HtmlValue;        // item value
   boolean     Update;           // must update SPIFFS    
} DataEntry;


enum  // index for numeric database
{
   DB_PERIOD1_MINS,           // 0
   DB_PERIOD1_CMS,            // 1
   DB_PERIOD1_DEGS,           // 2
   DB_PERIOD2_MINS,           // 3
   DB_PERIOD2_CMS,            // 4
   DB_PERIOD2_DEGS,           // 5
   DB_PERIOD3_MINS,           // 6
   DB_PERIOD3_CMS,            // 7
   DB_PERIOD3_DEGS,           // 8

   DB_SINE_RUNTIME_MINS,      // 9
   DB_DEPTH_CMS,              // 10

   DB_TEST_DESCRIPTION,       // 11
   DB_S1_DESCRIPTION,         // 12
   DB_S2_DESCRIPTION,         // 13
   DB_S3_DESCRIPTION,         // 14
   
   DB_SAMPLE_INTERVAL,        // 15

   DB_ACCEL_POWER,            // 16
   DB_DECEL_POWER,            // 17
   DB_RUN_POWER,              // 18
   DB_STOP_POWER,             // 19
   DB_DRUM_DIA,               // 20
   DB_DRIVE_RATIO,            // 21
   
   DB_CONTROLLER_TEMP,        // 22
   DB_MOTOR_LIFT_RPM,         // 23
   DB_TEST_ELASED_TIME,       // 24
   DB_SWEEP_START,            // 25
   DB_SWEEP_STOP,             // 26
   DB_SWEEP_DEPTH,            // 27
   DB_SWEEP_PREAMBLE,         // 28
   DB_SWEEP_RUNTIME_MINS,     // 29

   DB_DATAFILE_NAME,          // 30
} DBIndex;


//-------------------------------------------------------------------
#define  SENSOR_1_PRESS       "press1"
#define  SENSOR_1_PRESS_UNIT  "p1unit"
#define  SENSOR_1_TEMP        "temp1"
#define  SENSOR_1_TEMP_UNIT   "t1unit"

#define  SENSOR_2_PRESS       "press2"
#define  SENSOR_2_PRESS_UNIT  "p2unit"
#define  SENSOR_2_TEMP        "temp2"
#define  SENSOR_2_TEMP_UNIT   "t2unit"

#define  SENSOR_3_PRESS       "press3"
#define  SENSOR_3_PRESS_UNIT  "p3unit"
#define  SENSOR_3_TEMP        "temp3"
#define  SENSOR_3_TEMP_UNIT   "t3unit"

#define  SLUG_DEPTH           "sdepth"

#define  DATAFILE_NAME        "filename"



class CDataInputs
{
public:
   CDataInputs(void);

   void RestoreSavedValues(void);

   DataEntry GetEntry(uint32_t index) const;

   void PutEntry(uint32_t index, String value);

   void MatchAndUpdate(String &name, String &newValue);

   String GetCurrentSpiffsValues(void);

   String CreateDirectoryName(uint32_t index);

private:  

   boolean ActionParameter(int index);

private:
   String         mOldValue;
   uint32_t       mSampleInterval;  // interval between reading samples

};
