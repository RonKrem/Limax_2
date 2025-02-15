// Motor Drive Interface
//
#include "Main.h"



typedef enum
{
   PROFILE_NOTHING,
   PROFILE_EVENT,
} ProfileStateEnum;


class CMotorControl
{
public:
   CMotorControl();

   boolean Init(void);

   static void Task_ProcessStepTimer(void* parameter);

   void ManageSineTest(void);

   void ManageSweepTest(void);

   void DisplaySineProfile(void);

   void DisplaySweepProfile(void);

   float DriveTemperature(void);

   float GetSlugDepth(void) const;

   void InitSweep(void);

   void StopTest(void);

   // Do the cosine sums for the channels defined in
   // the Flag bit pattern.
   float ComputeSlugPosition(uint32_t index);

   boolean MotorActionParameters(int index);

   void DefineTestBoundaries(uint32_t runtimeIndex, boolean sine2, boolean sine3);

   uint32_t GetSubStartValue(void) const;

   uint32_t GetSubEndValue(void) const;

   float GetMaxValue(void) const;

   float GetScale(void) const;

   void ClearBoundariesDefined(void);
   boolean AreBoundariesDefined(void) const;

   uint32_t GetSteps(void) const;

   String SecondsToTime(uint32_t seconds);

   float NewSum(uint32_t index, boolean sine2, boolean sine3);

   uint32_t GetElapsedSeconds(void);

   void ComputeParameters(void);

   void DoParking(void);

   void SetNormalAccelRate(void);
   void SetSlowAccelRate(void);

   static void TimerDone(xTimerHandle pxTimer);
   void StartProfileTimer(void);
   ProfileStateEnum GetProfileTimerState(void) const;

private:  

   void BeginProfileTimer(void);

   void ComputeSineDepth(void);

   void ComputeSweepDepth(void);

   void PutSlugDepth(float position);

   float CoSine(float seconds, float period, float phase, float amplitude) const;

private:

   boolean mBoundariesDefined;
   float mSlugDepth;

   SemaphoreHandle_t mSlugMutex;
   xTimerHandle xProfileIntervalTimer;
   
   ProfileStateEnum mProfileTimerState;
   uint32_t mMaxProfileTime;
   uint32_t mTestSeconds;
   uint32_t mProfileElapsedTime;

   float mPrevious;

   float mTemperature;
   uint32_t mSubStart, mSubEnd;
   uint32_t mSteps;
   uint32_t mAnalysisSteps;
   float mTheMax;
   float mAmplitude1, mAmplitude2, mAmplitude3;
   float mPhase1, mPhase2, mPhase3;
   float mPeriod1, mPeriod2, mPeriod3;
   float mScale, mDepth;
   float mMax, mMin;

   float mf0, mf1, mTime, mR;

   float mSpeedMultiplier;
   float mParkingSpeedRPS;
   float mParkingIncrement;
};




//-------------------------------------------------------------------
//
inline ProfileStateEnum CMotorControl::GetProfileTimerState(void) const
{
   return mProfileTimerState;
}

//-------------------------------------------------------------------
//
inline uint32_t CMotorControl::GetSteps(void) const
{
   return mSteps;
}

//-------------------------------------------------------------------
//
inline boolean CMotorControl::AreBoundariesDefined(void) const
{
   return mBoundariesDefined;
}

//-------------------------------------------------------------------
//
inline void CMotorControl::ClearBoundariesDefined(void)
{
   mBoundariesDefined = false;
}

//-------------------------------------------------------------------
//
inline uint32_t CMotorControl::GetSubStartValue(void) const
{
   return mSubStart;
}

//-------------------------------------------------------------------
//
inline uint32_t CMotorControl::GetSubEndValue(void) const
{
   return mSubEnd;
}

//-------------------------------------------------------------------
//
inline float CMotorControl::GetMaxValue(void) const
{
   return mMax;
}

//-------------------------------------------------------------------
//
inline float CMotorControl::GetScale(void) const
{
   return mScale;
}

