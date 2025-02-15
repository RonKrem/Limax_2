// Timers.h
//
#ifndef  _TIMERS_H
#define  _TIMERS_H

#include "Main.h"



class CTimers
{
public:
   CTimers(void);

   void Init(void);

   // Start the 1 second step timer. Default is automatic.
   void StartStepTimer(StepTimerType type, uint32_t delayMSecs);

   // The step (1 second) timer has concluded.
   static void StepTimerDone(xTimerHandle pxTimer);

   // Get what state the step timer was in when it concluded.
   StepTimerType GetStepTimerState(void) const;

   void SetRecordInterval(uint32_t interval);
   uint32_t GetRecordInterval(void) const;
   uint32_t GetRecordIntervalTicks(void) const;
   void StepRecordIntervalTicks(void);
   void ClearRecordIntervalTicks(void);
   void SetPlotInterval(uint32_t interval);
   uint32_t GetPlotInterval(void) const;
   uint32_t GetPlotIntervalTicks(void) const;
   void StepPlotIntervalTicks(void);
   void ClearPlotIntervalTicks(void);

private:
   xTimerHandle         xStepTimer;

   StepTimerType        mStepTimerState;

   uint32_t             mPlotInterval;    // interval between plottin a sample
   uint32_t             mRecordInterval;  // interavl between recording a sample
   uint32_t             mRecordIntervalTicks;
   uint32_t             mPlotIntervalTicks;
};

//-------------------------------------------------------------------
//
inline void CTimers::SetRecordInterval(uint32_t interval)
{
   mRecordInterval = interval;
}

//-------------------------------------------------------------------
//
inline uint32_t CTimers::GetRecordInterval(void) const
{
   return mRecordInterval;
}

//-------------------------------------------------------------------
//
inline uint32_t CTimers::GetRecordIntervalTicks(void) const
{
   return mRecordIntervalTicks;
}

//-------------------------------------------------------------------
//
inline void CTimers::StepRecordIntervalTicks(void)
{
   mRecordIntervalTicks++;
}

//-------------------------------------------------------------------
//
inline void CTimers::ClearRecordIntervalTicks(void)
{
   mRecordIntervalTicks = 0;
}

//-------------------------------------------------------------------
//
inline void CTimers::SetPlotInterval(uint32_t interval)
{
   mPlotInterval = interval;
}

//-------------------------------------------------------------------
//
inline uint32_t CTimers::GetPlotInterval(void) const
{
   return mPlotInterval;
}

//-------------------------------------------------------------------
//
inline uint32_t CTimers::GetPlotIntervalTicks(void) const
{
   return mPlotIntervalTicks;
}

//-------------------------------------------------------------------
//
inline void CTimers::StepPlotIntervalTicks(void)
{
   mPlotIntervalTicks++;
}

//-------------------------------------------------------------------
//
inline void CTimers::ClearPlotIntervalTicks(void)
{
   mPlotIntervalTicks = 0;
}

//-------------------------------------------------------------------
//
inline StepTimerType CTimers::GetStepTimerState(void) const
{
   return mStepTimerState;
}

#endif   // _TIMERS_H