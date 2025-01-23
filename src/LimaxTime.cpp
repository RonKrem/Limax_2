// CLimaxTime.cpp
//
#include "LimaxTime.h"



//-------------------------------------------------------------------
//
CLimaxTime::CLimaxTime(void)
 : mTimeInSeconds(0)
{
}

//-------------------------------------------------------------------
//
void CLimaxTime::SetTimeInSeconds(uint32_t seconds)
{
   mTimeInSeconds = seconds;
}

//-------------------------------------------------------------------
//
void CLimaxTime::StepLimaxTime(void)
{
   mTimeInSeconds++;
//   Serial.printf("CLOCK TICKED %d\n", mTimeInSeconds);
}
