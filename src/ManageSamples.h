// ManageSamples.h
//
#ifndef  _MANAGESAMPLES_H
#define  _MANAGESAMPLES_H

#include "Main.h"
#include "SensorData.h"

#define  SIMPLE_ZERO_FIX

#ifdef SIMPLE_ZERO_FIX
#else
#define  DB_MONITOR_ENTRIES      7

typedef struct 
{
   float    pressure[DB_MONITOR_ENTRIES];
   float    m;
   float    c;
} PressureEntries;
#endif

typedef struct
{
   float value[5];
} DisplayData;
typedef struct 
{
   uint16_t       entries;
   DisplayData    sample[DISPLAY_SAMPLES + 1];
} DynamicData;


class CManageSamples
{
public:
   CManageSamples(void);

   static void Task_ManageSampleResults(void* parameter);

   String GetCurrentPlotValuesForClient(SensorSampleType* sampleSetPtr, uint32_t index);

   String GetCurrentValuesForClient(SensorSampleType* sampleSetPtr);

   void GetTestSampleSet(void);

   void SendSummary(void);

   void WriteSitePreamble(void);

private:   
   void MonitorForZeroes(SensorSampleType* sampleSetPtr);

   xQueueHandle GetNewSampleQueueHandle(void) const;

private:

   uint16_t          mTempFileContents;
   uint16_t          mMonitorEntries;
#ifdef SIMPLE_ZERO_FIX
   float             mPreviousPressures[NUM_SENSORS];
#else
   PressureEntries   mMonitor[NUM_SENSORS];
#endif
};




#endif   // _MANAGESAMPLES_H
