// CData.h
//
#include "Main.h"


//-----------------------------------------------------------------------------
// Analog structure definitions.
//
// typedef struct
// {
//    const char  *JsonValue;  // matches the input name from html
//    String      HtmlValue;        // item value
//    char        *PathName;        // SPIFFS name where stored
//    boolean     Update;           // must update SPIFFS    
// } DataEntry;
typedef struct
{
   String      JsonValue;     // matches the input name from html
   String      HtmlValue;     // item value
   boolean     Update;        // must update SPIFFS    
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
   DB_DEPTH_MS,               // 10

   // Missing numbers now in HDIndex
   
   DB_RECORD_INTERVAL,        // 15

   DB_ACCEL_POWER,            // 16
   DB_DECEL_POWER,            // 17
   DB_RUN_POWER,              // 18
   DB_STOP_POWER,             // 19
   DB_DRUM_DIA_MM,            // 20
   DB_DRIVE_RATIO,            // 21
   
   DB_CONTROLLER_TEMP,        // 22
   DB_MOTOR_LIFT_RPM,         // 23
   DB_TEST_ELAPSED_TIME,      // 24
   DB_SWEEP_START,            // 25
   DB_SWEEP_STOP,             // 26
   DB_SWEEP_DEPTH,            // 27
   DB_SWEEP_PREAMBLE,         // 28
   DB_SWEEP_RUNTIME_MINS,     // 29
   
   DB_PLOT_INTERVAL,          // 30  
} DBIndex;

enum
{
   HD_TEST_DESCRIPTION,       // 0
   HD_S1_DESCRIPTION,         // 1
   HD_S2_DESCRIPTION,         // 2
   HD_S3_DESCRIPTION,         // 3
   HD_DATAFILE_NAME,          // 4
} HDIndex;

enum DataType
{
   LIMAX_DATA,
   LIMAX_HEADER,
};

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




class CData
{
public:
   CData(void);

   void Init(void);
   
   void RestoreSavedValues(void);

   DataEntry GetDataEntry(const uint32_t index);

   String GetHeaderEntryString(uint32_t index);

   float GetDataEntryNumericValue(uint32_t index);

   String GetDataEntryStringValue(uint32_t index);

   void PutDataEntryValue(const uint32_t index, const String value);

   void MatchAndUpdateData(String &name, String &newValue);
   void MatchAndUpdateHeaders(String &name, String &newValue);

   String GetCurrentEpromDataValues(void);

   String GetCurrentEpromHeaderValues(void);

   void UpdateDatabase(const AsyncWebParameter* p);

private:  

   void Restore(DataEntry* p);

   void Update(DataEntry* p, const AsyncWebParameter* q);

   boolean ActionDataParameter(String directory, int index);

   String CreateFileName(DataEntry* p);

private:
   String         mOldValue;

};

