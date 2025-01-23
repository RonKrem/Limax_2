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

   // Start the interval timer. Default is automatic.
   void StartIntervalTimer(IntervalTimerType type, uint32_t delaymSecs);

   // The interval timer has concluded.
   static void IntervalTimerDone(xTimerHandle pxTimer);

   // Get what stae the interval timer was in on conclusion.   
   IntervalTimerType GetIntervalTimerState(void) const;

   // Start the 1 second step timer. Default is automatic.
   void StartStepTimer(StepTimerType type, uint32_t delayMSecs);

   // The step (1 second) timer has concluded.
   static void StepTimerDone(xTimerHandle pxTimer);

   // Get what state the step timer was in when it concluded.
   StepTimerType GetStepTimerState(void) const;

   // The 1 second step timer maintains an internal 32-bit
   // counter. This function will step it.
   void IncrementSecondsTimer(void);

   // Set current seconds timer to zero.
   void ClearSecondsTimer(void);

   // Return the current seconds count.
   uint32_t GetTestElapsedSeconds(void) const;

private:
   xTimerHandle         xStepTimer;
   xTimerHandle         xIntervalTimer;

   IntervalTimerType    mIntervalTimerState;
   StepTimerType        mStepTimerState;
   uint32_t             mTestElapsedSeconds;
};

//-------------------------------------------------------------------
//
inline StepTimerType CTimers::GetStepTimerState(void) const
{
   return mStepTimerState;
}

//-------------------------------------------------------------------
//
inline IntervalTimerType CTimers::GetIntervalTimerState(void) const
{
   return mIntervalTimerState;
}

//-------------------------------------------------------------------
//
inline void CTimers::ClearSecondsTimer(void)
{
   mTestElapsedSeconds = 0;
}

//-------------------------------------------------------------------
//
inline void CTimers::IncrementSecondsTimer(void)
{
   mTestElapsedSeconds++;
}

//-------------------------------------------------------------------
//
inline uint32_t CTimers::GetTestElapsedSeconds(void) const
{
   return mTestElapsedSeconds;
}

#endif   // _TIMERS_H