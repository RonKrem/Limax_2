/*
 * Motor.h
 *
 *  Created on: 22 Oct 2017
 *      Author: Kremford
 */
#ifndef MOTOR_H_
#define MOTOR_H_

#include "Main.h"

#define  SPEED_CORRECTION        1.0495

#define  PWRSTP_BUFFER_SIZE      4

#define  RSENSE                  (0.1)
#define  MOTOR_STEPS             200
#define  FULL_STEP_SWITCH_RPM    500

#define  STEP_TABLE_ENTRIES      8  // Only 1st 5 used. 1=full, 2=half, 3=1/4, 4=1/8 and 5=1/16

#define  SPI_BUFFER_SIZE         5


typedef enum
{
   DirReverse,
   DirForward
} PWRSTP_DIRECTION;

typedef struct
{
   uint16_t    motorStep;
   uint16_t    stepsPerRev;
} StepTableType;

typedef struct
{
   uint8_t     RegNum;
   uint8_t     RegBytes;
} PowerstepCommsSetType;


typedef struct
{
   uint8_t           getParam : 1;
   uint8_t           sendParam : 1;
} PwrStepFlagType;

typedef struct
{
   uint8_t           command;
   uint8_t           option;
   uint8_t           bytes;
   PwrStepFlagType   flag;
} BytesType;

typedef union
{
   float             fValue;
   uint32_t          iValue;
} ValueType;

typedef struct
{
   uint32_t          int32Value;
   void*             ptr;
   ValueType         v;
   BytesType         b;
} MessageType;
typedef MessageType*   MessagePtr;

typedef struct
{
   uint32_t    sendParam   : 1;
   uint32_t    getParam    : 1;
} MotorFlags;

typedef struct
{
   byte        command;
   byte        bytes;
   MotorFlags  flag;
   uint32_t    outValue;
   uint32_t    inValue;
} SpiCommandType;
typedef SpiCommandType*  SpiCommandPtr;


// These enums index the RegisterSet array. The values are not
// related to the actual register number. The comments show
// the actual register number.
//
typedef enum
{
   ABS_POS,       //              0
   EL_POS,        //              2
   MARK,          //              3
   SPEED,         //              4
   ACCEL,         //              5
   DECEL,         //              6
   MAX_SPEED,     //              7
   MIN_SPEED,     //              8
   ADC_OUT,       // (0x12)       9
   OCD_TH,        // (0x13)      10
   FS_SPD,        // (0x15)      11
   STEP_MODE,     // (0x16)      12
   ALARM_EN,      // (0x17)      13
   GATECFG1,      // (0x18)      14
   GATECFG2,      // (0x19)      15
   STEP_STATUS,   // (0x1b)      16
   CONFIG_A,      // (0x1a)      17
   KVAL_HOLD,     // (0x09)      18
   KVAL_RUN,      // (0x0a)      19
   KVAL_ACC,      // (0x0b)      20
   KVAL_DEC,      // (0x0c)      21
   INT_SPEED,     // (0x0d)      22
   ST_SLP,        // (0x0e)      23
   FN_SLP_ACC,    // (0x0f)      24
   FN_SLP_DEC,    // (0x10)      25
   K_THERM,       // (0x11)      26
   STALL_TH,      // (0x14)      27
   CONFIG_B,      // (0x1a)      28
   TVAL_HOLD,     // (0x09)      29
   TVAL_RUN,      // (0x0a)      30
   TVAL_ACC,      // (0x0b)      31
   TVAL_DEC,      // (0x0c)      32
   T_FAST,        // (0x0e)      33
   TON_MIN,       // (0x0f)      34
   TOFF_MIN,      // (0c10)      35
   CONFIG_C,      // (0x1a)      36
} MOTOR_REGISTER_SET;


//-------------------------------------------------------------------
class CMotor
{
public:
   CMotor(void);

   void Configure(void);
   
   uint32_t GetDirection(void);           // get direction
   uint32_t GetStoppedPwr(void);          // get stopped power
   uint32_t GetAccelPower(void) const;            // get accel power
   uint32_t GetDecelPwr(void);            // get decel power
   uint32_t GetRunPwr(void);              // get run power
   uint32_t GetStepIndex(void);           // get step index
   uint32_t GetAccelRate(void);           // get accel rate
   uint32_t GetDecelRate(void);           // get decel rate

   void SetDirection(uint32_t value);     // set direction
   void SetStoppedPwr(uint32_t value);    // set stopped power
   void SetAccelPwr(uint32_t value);      // set accel power
   void SetDecelPwr(uint32_t value);      // set decel power
   void SetRunPwr(uint32_t value);        // set run power
   void SetAccelRate(uint32_t value);     // set accel rate
   void SetDecelRate(uint32_t value);     // set decel rate

   void ResetMotor(void);                 // reset powerstep01

   // Returns the value of the given parameter.
   uint32_t GetParam(uint16_t theRegister);

   // Sets the given parameter to the given value. Uses the semaphore
   // to handle the controller done.
   void SetParam(MOTOR_REGISTER_SET theRegister, uint32_t value);

   // Return the corresponding steps/rev for the current index.
   uint32_t GetStepsPerRev(uint32_t index);

   // Check the input step setting is allowed.
   int16_t CheckStepBoundaries(int16_t value);

   void RunMotorAtRevsPerSec(float rps);

   // The sign specifies the direction.
   void RunAtTickRate(float TicksPerSec);

   // Set the SPS rate the motor will run at.
   void SetMaxSpeed(uint32_t maxSpeed);

   float GetWinchSpeed(void);

   // Set the SPS rate the motor will run at.
   void SetMinSpeed(uint32_t minSpeed);

   // Perform a soft stop.
   void SoftStop(void);

   // Return the current controller status.
   uint32_t GetMotorStatus(void);

   // set the chip speed.
   void SetWinchSpeedRPM(int32_t rpm);

   // run at the given SPS speed.
   void RunAtSPS(uint32_t stepsPerSec);

   // Set the step mode.
   void SetStepMode(int16_t mode);

   // return RPM in RPS.
   float RPMtoRPS(int32_t rpm);

   // return RPM to SPS
   uint32_t RPMtoSPS(uint16_t rpm);

   // return RPS to SPS
   uint32_t RPStoSPS(uint32_t rps);

   uint32_t GetSpiClk(void) const;

   void BrakePowerOff(void);
   void EnergiseBrake(void);

private:
   // Return the given accel/decel figure in controller terms.
   uint32_t ConvertAcceleration(uint16_t stepsPerSecondSquared);

   // Configure the default motor state.
   void ConfigureDefaultPowerstep(void);

   // Start running in the given direction at the given speed.
   void Run(uint32_t speed);

   // Reset the device to the power up state.
   void ResetDevice(void);

   // Perform a soft stop and switch the bridges to HI-Z.
   void SoftHiZ(void);

   // Perform a hard stop and switch the bridges to HI-Z.
   void HardHiZ(void);

   // Return the given steps/sec converted to the controller's
   // SPEED format. Use only this to set the motor speed.
   uint32_t StepsPerSecondToChipSpeed(uint32_t stepsPerSecond);

   uint32_t StepsPerSecondToMaxSpeed(uint32_t stepsPerSecond);

   uint32_t StepsPerSecondToMinSpeed(uint32_t stepsPerSecond);

   // Use to set the TON_MIN and TOFF_MIN times.
   uint32_t OnOffTimes(float microSecs);

   // Use to set the TOFF_FAST and FAST_STEP times.
   uint32_t FastStepTimes(float tOffFastuSecs, float fastStepuSecs);

   // Use to set the TBLANK and TDT times.
   uint32_t BridgeTimes(uint32_t blankTime, uint32_t deadTime);

   // Use to set TVAL registers HOLD, RUN, ACC and DEC as mV.
   uint32_t SetTorqueByPercent(uint32_t percent);

   // Use to set TVAL registers HOLD, RUN, ACC and DEC.
   // Bounds input range to 0-100. Maps 0-100 to 0-127.
   uint32_t Set_TVAL_X_Value(uint32_t value);

   // Returns the bounded OCD_TH value.
   uint32_t Set_VDS_Voltage(uint32_t v_OCD_TH);

   // SPI interface.
private:
   void ClearSpiMessage(SpiCommandPtr ptr);

   // Send the command via a busy/wait SPI message.
   void SendSpiCommand(SpiCommandPtr commPtr);

   void ClearResultBuffer(uint32_t index);

   void SetupResultBuffer(uint32_t index);

   uint32_t GetReply(uint32_t bytes);

   uint32_t SendSpiByte(uint8_t* ptr);

   int32_t FloatToInt32(float value);


//-----------------------------------------------------------------------------
private:

   uint8_t                 mPwrStpTxBuffer[PWRSTP_BUFFER_SIZE];   // SPI TX message
   bool                    mFirstTime;
   StepTableType           mStepTable[STEP_TABLE_ENTRIES];
   uint16_t                mStepIndex;
   uint32_t                mAccelRate;
   uint32_t                mDecelRate;

   float                   mSpeedRevsPerSec;

   // SPI data structures.
   uint32_t                spiClk;
   uint8_t                 TxBuffer[SPI_BUFFER_SIZE];
   uint8_t                 RxBuffer[SPI_BUFFER_SIZE];
   uint8_t                 mResultBuffer[SPI_BUFFER_SIZE];
   uint8_t                 mResultBufferIndex;

};


//-----------------------------------------------------------------------------
//
inline uint32_t CMotor::GetStepIndex(void) 
{
   return (uint32_t)mStepIndex;
}

//-----------------------------------------------------------------------------
//
inline uint32_t CMotor::GetAccelRate(void) 
{
   return (uint32_t)mAccelRate;
}

//-----------------------------------------------------------------------------
//
inline uint32_t CMotor::GetDecelRate(void) 
{
   return (uint32_t)mDecelRate;
}

//-----------------------------------------------------------------------------
//
inline uint32_t CMotor::GetSpiClk(void) const
{
   return (uint32_t)spiClk;
}

//-----------------------------------------------------------------------------
// To apply the brake we just turn the relay off.
//
inline void CMotor::BrakePowerOff(void)
{
   digitalWrite(BRAKE_PORT, LOW);
}
   
//-----------------------------------------------------------------------------
// To release the brake we must power the realy -> brake.
//
inline void CMotor::EnergiseBrake(void)
{
   digitalWrite(BRAKE_PORT, HIGH);
}


#endif /* MOTOR_H_ */
