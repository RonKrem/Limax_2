// CLimaxTime.h
//
// Encapsulates the current time.
//
#ifndef TIME_H
#define TIME_H

#include "Main.h"


class CLimaxTime
{
public:
   CLimaxTime(void);

   void SetTimeInSeconds(uint32_t seconds);

   void StepLimaxTime(void);

   uint32_t GetLimaxTime(void) const;


private:
   
   uint32_t       mTimeInSeconds;

};


inline uint32_t CLimaxTime::GetLimaxTime(void) const
{
   return mTimeInSeconds;
}
#endif   // TIME_H
