// Motor Drive Interface
//
#include "Main.h"


class CMotorControl
{
public:
   CMotorControl();

   void Init(void);

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

   int32_t GetAmplitude(void);

   String SecondsToTime(uint32_t seconds);

   float NewSum(uint32_t index, boolean sine2, boolean sine3);

private:  

   void ComputeDepth(uint32_t seconds);

   void SweepState(uint32_t seconds);

   void PutSlugDepth(float position);

   
private:

   boolean mBoundariesDefined;
   float mSlugDepth;
   SemaphoreHandle_t mSlugMutex;

   float mOneDrumTurnInCms;
   float mPrevious;

   float mTemperature;
   uint32_t mSubStart, mSubEnd;
   uint32_t mSteps;
   float mPhaseScale;
   float mTheMax;
   float mAngularFrequency1, mAngularFrequency2, mAngularFrequency3;
   float mAmplitude;
   float mAmplitude1, mAmplitude2, mAmplitude3;
   float mPhase1, mPhase2, mPhase3;
   float mPeriod1, mPeriod2, mPeriod3;
   float mScale, mDepth;
   float mMax, mMin;
   uint32_t mTestSeconds;

   float mf0, mf1, mTime, mR;

};


//-------------------------------------------------------------------
//
inline int32_t CMotorControl::GetAmplitude(void)
{
   return (int32_t)(-mAmplitude);;
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
