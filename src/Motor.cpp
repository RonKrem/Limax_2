/*****************************************************************************

 NAME:         Motor.cpp

 DESCRIPTION:  Provides an interface to the Powerstep01 command set.


 AUTHOR:       Ron Kreymborg
               Kremford Pty Ltd

 REVISIONS:

******************************************************************************/

#include "Motor.h"
#include "DataInputs.h"
#include "Buttons.h"


// Commands.
//
#define  PWRSTP_SET_PARAM              0x00
#define  PWRSTP_GET_PARAM              0x20
#define  PWRSTP_RUN                    0x50
#define  PWRSTP_STEP_CLOCK             0x58
#define  PWRSTP_MOVE                   0x40
#define  PWRSTP_GOTO                   0x60
#define  PWRSTP_GOTO_DIR               0x68
#define  PWRSTP_GO_UNTIL               0x82
#define  PWRSTP_RELEASE_SW             0x92
#define  PWRSTP_GO_HOME                0x70
#define  PWRSTP_GO_MARK                0x78
#define  PWRSTP_RESET_POS              0xd8
#define  PWRSTP_RESET_DEVICE           0xc0
#define  PWRSTP_HARD_STOP              0xb8
#define  PWRSTP_SOFT_HIZ               0xa0
#define  PWRSTP_HARD_HIZ               0xa8
#define  PWRSTP_SOFT_STOP              0xb0
#define  PWRSTP_GET_STATUS             0xd0

#define  VOLTAGE_MODE                  0
#define  PRED_EN                       ((uint32_t)1<<15)
#define  SWITCH_PERIOD(x)              ((uint32_t)(x & 0x1f)<<10)
#define  VCCVAL_15V                    ((uint32_t)1<<9)
#define  UVLOVAL_10V                   ((uint32_t)1<<8)
#define  OC_CD_SHUTDOWN                ((uint32_t)1<<7)
#define  EN_TQREG                      ((uint32_t)1<<5)
#define  SW_MODE_DISABLE               ((uint32_t)1<<4)
#define  EXT_CLK                       ((uint32_t)1<<3)
#define  OSC_SEL(x)                    ((uint32_t)(x & 0x7)<<0)

#define  SYNC_EN                       ((uint32_t)1<<7)
#define  SYNC_SEL(x)                   ((uint32_t)(0x7 & x)<<4)
#define  CM_VM                         ((uint32_t)1<<3)     // CM_VM (current mode)


#define  DEFAULT_SWITCH_RPM            (300)
#define  SYSTEM_MAX_RPM                (1200)

#define  WD_EN                         ((uint32_t)1<<11)
#define  TBOOST(x)                     ((uint32_t)(0x7 & x)<<8)
#define  BOOST_ON                      ((uint32_t)(0x1 << 10))
#define  IGATE(x)                      ((uint32_t)(0x7 & x)<<5)
#define  TCC(x)                        ((uint32_t)(0x1f & x))


//#define  PRINT_MOTOR
#ifdef PRINT_MOTOR
#define  P_MOTR(x)   x;
#else
#define  P_MOTR(x)   // x
#endif

extern CMotor Motor;         // ourself
extern SPIClass theSPI;        // SPI
extern CDataInputs DataInputs;
extern CButtons Buttons;

// Registers and the number of bytes required. This table
// is indexed by register name via MOTOR_REGISTER_SET enums.
//
const PowerstepCommsSetType PowerstepCommsSet[] =
{
      {0x01,3},      // ABS_POS     0
      {0x02,2},      // EL_POS      1
      {0x03,3},      // MARK        2
      {0x04,3},      // SPEED       3
      {0x05,2},      // ACC         4
      {0x06,2},      // DEC         5
      {0x07,2},      // MAX_SPEED   6
      {0x08,2},      // MIN_SPEED   7
      {0x12,1},      // ADC_OUT     8
      {0x13,1},      // OCD_TH      9
      {0x15,2},      // FS_SPD      10
      {0x16,1},      // STEP_MODE   11
      {0x17,1},      // ALARM_EN    12
      {0x18,2},      // GATECFG1    13
      {0x19,1},      // GATECFG2    14
      {0x1b,2},      // STATUS      15
      {0x1a,2},      // CONFIGA     16
      {0x09,1},      // KVAL_HOLD   17
      {0x0a,1},      // KVAL_RUN    18
      {0x0b,1},      // KVAL_ACC    19
      {0x0c,1},      // KVAL_DEC    20
      {0x0d,2},      // INT_SPEED   21
      {0x0e,1},      // ST_SLP      22
      {0x0f,1},      // FN_SLP_ACC  23
      {0x10,1},      // FN_SLP_DEC  24
      {0x11,1},      // K_THERM     25
      {0x14,1},      // STALL_TH    26
      {0x1a,2},      // CONFIGB     27
      {0x09,1},      // TVAL_HOLD   28
      {0x0a,1},      // TVAL_RUN    29
      {0x0b,1},      // TVAL_ACC    30
      {0x0c,1},      // TVAL_DEC    31
      {0x0e,1},      // T_FAST      32
      {0x0f,1},      // TON_MIN     33
      {0x10,1},      // TOFF_MIN    34
      {0x1a,2},      // CONFIGC     35
};


//-----------------------------------------------------------------------------
//
CMotor::CMotor(void)
{
   mFirstTime = true;
   mAccelRate = 1000;
   mDecelRate = 1000;
   spiClk = 100000;
   mSpeedRevsPerSec = 0;
}

//-----------------------------------------------------------------------------
//
void CMotor::Configure(void)
{
   P_MOTR(Serial.println("Motor: Configure"));

   // Fill out the step table. It only has entries for the current drive which
   // are FULL_STEP, HALF_STEP, x4, x8, and x16.
   //
   for (int i=0; i<STEP_TABLE_ENTRIES; i++)
   {
      mStepTable[i].motorStep = i;
      mStepTable[i].stepsPerRev = (uint16_t)(pow(2.0, i) * MOTOR_STEPS);
   }

   ResetMotor();     // reset pulse

   // Provide the initial configuration.
   //
   ConfigureDefaultPowerstep();

   uint32_t status = GetMotorStatus();
   Serial.printf("Motor status: %04X\n", status);
}

//-----------------------------------------------------------------------------
//
void CMotor::SetParam(MOTOR_REGISTER_SET theRegister, uint32_t settings)
{
   SpiCommandType command;
   uint32_t status;

   ClearSpiMessage(&command);
   command.command = PWRSTP_SET_PARAM | PowerstepCommsSet[theRegister].RegNum;
   command.bytes = PowerstepCommsSet[theRegister].RegBytes;
   command.flag.sendParam = true;
   command.flag.getParam = false;
   command.outValue = settings;

   P_MOTR(Serial.printf("Motor:SetParam: %02X  Bytes: %d  Val: %6d (%04X)\n", command.command, command.bytes, settings, settings));
   SendSpiCommand(&command);
}

//-----------------------------------------------------------------------------
//
uint32_t CMotor::GetParam(uint16_t theRegister)
{
   uint32_t retval;
   SpiCommandType command;

   ClearSpiMessage(&command);
   command.command = PWRSTP_GET_PARAM | PowerstepCommsSet[theRegister].RegNum;
   command.bytes = PowerstepCommsSet[theRegister].RegBytes;
   command.flag.sendParam = false;
   command.flag.getParam = true;
   SendSpiCommand(&command);

   retval = GetReply(PowerstepCommsSet[theRegister].RegBytes);
   P_MOTR(Serial.printf("Motor:GetParam:  Register: %02X  Result: %4X\n", command.command, retval));

   return retval;
}

//-----------------------------------------------------------------------------
//
int32_t CMotor::FloatToInt32(float value)
{
   return (int32_t)(value + 0.5);
}

//-----------------------------------------------------------------------------
//
uint32_t CMotor::GetStepsPerRev(uint32_t index)
{
   if (index >= STEP_TABLE_ENTRIES)
   {
      index = 0;     // just return first entry
   }

   return mStepTable[index].stepsPerRev;
}

//-----------------------------------------------------------------------------
// Init the SPI interface and send the init stream to the motor. 
//
void CMotor::ResetMotor(void)
{
   // Provide a RESET pulse for the POWERSTEP01.
   //
   digitalWrite(STEPPER_CS, HIGH);
   digitalWrite(STEPPER_RESET, HIGH);
   delay(20);
   digitalWrite(STEPPER_RESET, LOW);
   delay(20);
   digitalWrite(STEPPER_RESET, HIGH);
   delay(200);
}

//-----------------------------------------------------------------------------
//
void CMotor::SoftStop(void)
{
   SpiCommandType command;

   ClearSpiMessage(&command);
   command.command = PWRSTP_SOFT_STOP;
   command.bytes = 1;
   SendSpiCommand(&command);
}

//-----------------------------------------------------------------------------
//
void CMotor::SetRunPwr(uint32_t value) 
{
   SetParam(TVAL_RUN, Set_TVAL_X_Value((uint32_t)DataInputs.GetEntry(DB_RUN_POWER).HtmlValue.toInt()));    // run power
}

//-----------------------------------------------------------------------------
//
void CMotor::SetStoppedPwr(uint32_t value) 
{
   SetParam(TVAL_HOLD, Set_TVAL_X_Value((uint32_t)DataInputs.GetEntry(DB_STOP_POWER).HtmlValue.toInt()));   // hold power
}

//-----------------------------------------------------------------------------
//
void CMotor::SetAccelPwr(uint32_t value) 
{
   SetParam(TVAL_ACC, Set_TVAL_X_Value((uint32_t)DataInputs.GetEntry(DB_ACCEL_POWER).HtmlValue.toInt()));    // accel power
}

//-----------------------------------------------------------------------------
//
void CMotor::SetDecelPwr(uint32_t value) 
{
   SetParam(TVAL_DEC, Set_TVAL_X_Value((uint32_t)DataInputs.GetEntry(DB_DECEL_POWER).HtmlValue.toInt()));    // decel power
}

//-----------------------------------------------------------------------------
//
void CMotor::SetAccelRate(uint32_t value) 
{
   SetParam(ACCEL, ConvertAcceleration(value));   // accel in steps/tick^2
}

//-----------------------------------------------------------------------------
//
void CMotor::SetDecelRate(uint32_t value) 
{
   SetParam(DECEL, ConvertAcceleration(value));   // accel in steps/tick^2
}

//-----------------------------------------------------------------------------
//
uint32_t CMotor::RPStoSPS(uint32_t rps)
{
   uint32_t sps;

   // Convert to steps/second.
   //
   sps = rps * MOTOR_STEPS;

   return sps;
}

//-----------------------------------------------------------------------------
// Convert a given RPM to Steps/Sec.
//
float CMotor::RPMtoRPS(int32_t rpm)
{
   // Convert to steps/sec and store.
   //
   mSpeedRevsPerSec = (float)rpm / 60.0;

   return mSpeedRevsPerSec;
}

//-----------------------------------------------------------------------------
// Convert a given RPM to Steps/Sec.
//
uint32_t CMotor::RPMtoSPS(uint16_t rpm)
{
   uint16_t rps;

   // Convert to steps/sec.
   //
   rps = (uint16_t)FloatToInt32((float)rpm / 60.0 * MOTOR_STEPS);

   return rps;
}

//-----------------------------------------------------------------------------
//
uint32_t CMotor::StepsPerSecondToChipSpeed(uint32_t stepsPerSecond)
{
   return FloatToInt32((float)stepsPerSecond * 67.108864);
}

//-----------------------------------------------------------------------------
// Set the SPEED register to this steps/sec setting.
//
void CMotor::SetWinchSpeedRPM(int32_t rpm)
{
   mSpeedRevsPerSec = RPMtoRPS(rpm);     // change to motor speed
}

//-----------------------------------------------------------------------------
//
float CMotor::GetWinchSpeed(void)
{
   return mSpeedRevsPerSec;
}

//-----------------------------------------------------------------------------
// Run motor using the tick rate at a given turns per second.
//
void CMotor::RunMotorAtRevsPerSec(float revsPerSec)
{
   float tickRate;

   tickRate = revsPerSec * 67.108864 * MOTOR_STEPS;

   if (Buttons.GetButtonState(BUTTON_DIRECTION))
   {
      tickRate = -tickRate;
   }

   RunAtTickRate(tickRate);
}

// All below are private...
//
//-----------------------------------------------------------------------------
//
int16_t CMotor::CheckStepBoundaries(int16_t value)
{
   // Microstepping is limited in Current mode. Range for setting is 0 to 4.
   // Note that this must be used as 0 to 4.
   //
   if (value < 0)
   {
      value = 0;
   }
   // if (value > 4)
   // {
   //    value = 4;
   // }

   return value;
}

//-----------------------------------------------------------------------------
//
void CMotor::RunAtSPS(uint32_t stepsPerSec)
{
   P_MOTR(Serial.printf("StepsPerSec %d\n",stepsPerSec));
   if (stepsPerSec > 0)
   {
      Run(StepsPerSecondToChipSpeed(stepsPerSec));     // change to motor speed
   }
}

// //-----------------------------------------------------------------------------
// //
// void CMotor::SetMaxSpeed(uint32_t stepsPerSec)
// {
//    uint32_t temp;

//    temp = StepsPerSecondToMaxSpeed(stepsPerSec);   // change to motor speed
//    Serial.printf("Motor steps: %d\n", temp);
//    SetParam(MAX_SPEED, temp);
// }

// //-----------------------------------------------------------------------------
// //
// void CMotor::SetMinSpeed(uint32_t stepsPerSec)
// {
//    uint32_t temp;

//    temp = StepsPerSecondToMinSpeed(stepsPerSec);   // change to motor speed
//    SetParam(MIN_SPEED, temp);
// }

//-----------------------------------------------------------------------------
//
void CMotor::SetStepMode(int16_t mode)
{
   HardHiZ();

   // Final mode check.
   //
   if (mode < 0)
   {
      mode = 0;
   }
   if (mode > 4)
   {
      mode = 4;
   }
   SetParam(STEP_MODE, CM_VM | mode);    // set step mode
}

//-----------------------------------------------------------------------------
//
uint32_t CMotor::GetMotorStatus(void)
{
   SpiCommandType command;
   uint32_t retval;

   ClearSpiMessage(&command);
   command.command = PWRSTP_GET_STATUS;        // set control byte
   command.bytes = 3;
   command.flag.getParam = true;
   SendSpiCommand(&command);

   retval = GetReply(2);

   return retval;
}

// //-----------------------------------------------------------------------------
// // The <speed> parameter is in steps/second.
// //
// void CMotor::Run(uint32_t speed)
// {
//    SpiCommandType command;

//    Serial.printf("Speed %d\n", speed);

//    ClearSpiMessage(&command);
//    command.command = PWRSTP_RUN;
//    if (Button[BUTTON_DIRECTION].state)
//    {
//       command.command |= 1;
//    }
//    command.bytes = 3;
//    command.flag.sendParam = true;
//    command.outValue = (speed & 0xfffff);
//    SendSpiCommand(&command);
// }

//-----------------------------------------------------------------------------
//
void CMotor::RunAtTickRate(float fTicksPerSec)
{
   SpiCommandType command;
   int32_t ticksPerSec;

   ClearSpiMessage(&command);
   ticksPerSec = (int32_t)(fTicksPerSec + 0.5);
   command.command = PWRSTP_RUN;
   if (ticksPerSec < 0)
   {
      command.command |= 1;
      ticksPerSec = abs(ticksPerSec);
   }

   command.bytes = 3;
   command.flag.sendParam = true;
   command.outValue = (ticksPerSec & 0xfffff);
   SendSpiCommand(&command);
}

//-----------------------------------------------------------------------------
//
void CMotor::ResetDevice(void)
{
   SpiCommandType command;

   ClearSpiMessage(&command);
   command.command = PWRSTP_RESET_DEVICE;
   command.bytes = 1;
   SendSpiCommand(&command);
}

//-----------------------------------------------------------------------------
//
void CMotor::SoftHiZ(void)
{
   SpiCommandType command;

   ClearSpiMessage(&command);
   command.command = PWRSTP_SOFT_HIZ;
   command.bytes = 1;
   SendSpiCommand(&command);
}

//-----------------------------------------------------------------------------
//
void CMotor::HardHiZ(void)
{
   SpiCommandType command;

   ClearSpiMessage(&command);
   command.command = PWRSTP_HARD_HIZ;
   command.bytes = 1;
   SendSpiCommand(&command);
}

//-----------------------------------------------------------------------------
//
uint32_t CMotor::StepsPerSecondToMaxSpeed(uint32_t stepsPerSecond)
{
   return FloatToInt32(stepsPerSecond * 0.065536);
}

//-----------------------------------------------------------------------------
//
uint32_t CMotor::StepsPerSecondToMinSpeed(uint32_t stepsPerSecond)
{
   return FloatToInt32(stepsPerSecond * 4.1943);
}

//-----------------------------------------------------------------------------
//
uint32_t CMotor::ConvertAcceleration(uint16_t stepsPerSecondSquared)
{
   return FloatToInt32((float)stepsPerSecondSquared * 0.0687);
}

//-----------------------------------------------------------------------------
//
uint32_t CMotor::OnOffTimes(float microSecs)
{
   return (uint32_t)((microSecs - 0.5) / 0.5) & 0x7f;
}

//-----------------------------------------------------------------------------
//
uint32_t CMotor::BridgeTimes(uint32_t blankTime, uint32_t deadTime)
{
   uint32_t value;

   value = ((uint32_t)((blankTime - 125) / 125) & 0x7) << 5;
   value |= (uint32_t)((deadTime - 125) / 125) & 0x1f;

   return value;
}

//-----------------------------------------------------------------------------
//
uint32_t CMotor::FastStepTimes(float tOffFastuSecs, float fastStepuSecs)
{
   uint32_t value;

   value = ((uint32_t)((tOffFastuSecs - 2.0) / 2.0) & 0xf) << 4;
   value |= (uint32_t)((fastStepuSecs - 2.0) / 2.0) & 0xf;

   return value;
}

//-----------------------------------------------------------------------------
// Sets the output mosfet switch maximum Vds voltage. This is independant of
// the Vsense current control.
//
uint32_t CMotor::Set_VDS_Voltage(uint32_t v_OCD_TH)
{
   // Ensure max value is 31.
   //
   if (v_OCD_TH > 31)
   {
      v_OCD_TH = 31;
   }

   return v_OCD_TH;
}

//-----------------------------------------------------------------------------
// TVAL_X range is 0-127 measuring 0-1 volts. The max current is 10A. 
// With Rsense coming from a 0.1ohm resistor, 10A occurs at 1.0 volts.
// The resulting relation between the input range of 0-127 called S and the 
// equivelent measurement is:
//    Vo = 0.0078 * S + 0.0078
//
// In the Limax, the caller range is 0-100 requiring a multiplier of 1.27, so 
// the above becomes:
//    Vo = 0.009906 * s + 0.0078
//
// This is rounded off to:
//    Vo = 0.00991 * s
//  giving a max current of 9.91 amps.
//
// A few 0-100 values mapped into current are:
//    5     0.496
//   20     1.98
//   10     0.991
//   50     4.96
//   75     7.43
//  100     9.91
//
uint32_t CMotor::Set_TVAL_X_Value(uint32_t value)
{
   // Arrives with 100 as full scale.
   // Ensure no illegal values.
   //
   if (value > 100)
   {
      value = 100;
   }

   // Map the 0-100 range to 0-127.
   //
   return (uint32_t)((float)value * 1.27);
}

//-----------------------------------------------------------------------------
//
void CMotor::ClearSpiMessage(SpiCommandPtr ptr)
{
   memset((void*)ptr, 0, sizeof(SpiCommandType));
}

//-----------------------------------------------------------------------------
//
void CMotor::SendSpiCommand(SpiCommandPtr commPtr)
{
   volatile uint8_t i, j, k;
   int8_t count = 0;

   // Check for a legal command.
   //
   if (commPtr->flag.sendParam)
   {
      if (commPtr->bytes < 1 || commPtr->bytes > 3)
      {
         return;
      }
   }

//   theSPI.begin(STEPPER_SCK, STEPPER_CIPO, STEPPER_COPI, STEPPER_CS);

   // Clear the working buffer.
   //
   memset((void*)mPwrStpTxBuffer, 0, PWRSTP_BUFFER_SIZE);

   mPwrStpTxBuffer[0] = commPtr->command;      // command byte

   // Decide whether a parameter is to be sent.
   //
   if (commPtr->flag.sendParam)
   {
      switch (commPtr->bytes)
      {
      case 3:
         mPwrStpTxBuffer[1] = (commPtr->outValue >> 16) & 0xff;
         mPwrStpTxBuffer[2] = (commPtr->outValue >>  8) & 0xff;
         mPwrStpTxBuffer[3] = (commPtr->outValue >>  0) & 0xff;
         count = 4;
         break;

      case 2:
         mPwrStpTxBuffer[1] = (commPtr->outValue >>  8) & 0xff;
         mPwrStpTxBuffer[2] = (commPtr->outValue >>  0) & 0xff;
         count = 3;
         break;

      case 1:
         mPwrStpTxBuffer[1] = (commPtr->outValue >>  0) & 0xff;
         count = 2;
         break;

      default:
         break;
      }
   }
   // Decide whether a parameter will be returned.
   //
   else if (commPtr->flag.getParam)
   {
      count = commPtr->bytes + 1;    // request bytes plus command
   }
   else
   {
      // All others just send a command byte.
      //
      count = 1;
   }


   P_MOTR(Serial.printf("Count: %d\n", count));
   for (i=0; i<count; i++)
   {
      P_MOTR(Serial.printf("%02X  ", mPwrStpTxBuffer[i]));
   }
   P_MOTR(Serial.println(" "));
   

   // Copy the buffer into the SPI's buffer.
   //
   SetupResultBuffer(count);     // clear SPI input buffer

   // Send the number of bytes in the output buffer a byte at a time.
   // All done with busy/waiting.
   //
   theSPI.beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE3));
   i = 0;                              // clear output buffer index
   while (count-- > 0)
   {
      digitalWrite(STEPPER_CS, LOW);       // enable the powerstep chip select
      SendSpiByte(&mPwrStpTxBuffer[i++]);    // send this command byte
      digitalWrite(STEPPER_CS, HIGH);      // disable the chip select

      // Small delay for the powerstep.
      //
      for (k=0; k<50; k++) 
      { 
         j = k; 
      }
   }
   theSPI.endTransaction();
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
// Send a command a byte at a time. The <index> is used to specify into which
// output byte any reply to this command must go. Does not use interrupts.
//
uint32_t CMotor::SendSpiByte(uint8_t* ptr)
{
   uint8_t retVal;

   // Setup the byte transfer.
   //
   retVal = theSPI.transfer(*ptr);   // transfer in/out

   // Copy any received byte into output buffer.
   //
   mResultBuffer[mResultBufferIndex++] = retVal;

   return (uint32_t)retVal;
}
//-----------------------------------------------------------------------------
//
uint32_t CMotor::GetReply(uint32_t bytes)
{
   uint32_t reply;

   switch (bytes)
   {
   case 1:
      reply = (uint32_t)mResultBuffer[1];
      break;

   case 2:
      reply = (uint32_t)mResultBuffer[1]<<8 | (uint32_t)mResultBuffer[2];
      break;

   case 3:
      reply = (uint32_t)mResultBuffer[1]<<16 | (uint32_t)mResultBuffer[2]<<8 | (uint32_t)mResultBuffer[3];
      break;

   default:
      reply = 0;
      break;
   }

   return reply;
}


//-----------------------------------------------------------------------------
//
void CMotor::ClearResultBuffer(uint32_t index)
{
   mResultBufferIndex = 0;
   memset((void*)mResultBuffer, 0, SPI_BUFFER_SIZE);
}

//-----------------------------------------------------------------------------
//
void CMotor::SetupResultBuffer(uint32_t index)
{
   mResultBufferIndex = 0;
   memset((void*)mResultBuffer, 0, SPI_BUFFER_SIZE);
}

//-----------------------------------------------------------------------------
// The register set for the motor is always the pDevice.
//
void CMotor::ConfigureDefaultPowerstep(void)
{
   uint16_t sps;

   // Init the powerstep01
   //
   // 00001001
   // ^        SYNC_EN  sync clock disabled (BUSY is enabled)
   //  ^^^     SYNC_SEL
   //     ^    CM_VM    current mode
   //      ^^^ STEP_SEL half step
//   SetParam(STEP_MODE, CM_VM | mStepTable[StepIndex].entry.motorStep);    //
//   SetParam(STEP_MODE, CM_VM | 3);    // 1/8 th
   SetParam(STEP_MODE, 3);    // 1/8 th

   P_MOTR(Serial.printf("STEP MODE %02X\n", GetParam(STEP_MODE)));

   // 0001101110001000
   // ^                 PRED_EN  1  prediction enabled
   //  ^^^^^            TSW      6  switching period (6 = 24 uSec)
   //       ^           VCCVAL   1  15V
   //        ^          UVLOVAL  1  10V threshold
   //         ^         OC_SD    1  bridges shutdown on overcurrent (1)
   //          x
   //           ^       EN_TQREG 0  peak current adjust thru ADCIN (disabled)
   //            ^      SW_MODE  0  ext switch - (0 = disabled)
   //             ^     EXT_CLK  1  clk out on OSCOUT
   //              ^^^  OSC_SEL  3  use internal 16MHz osc
   SetParam(CONFIG_C, PRED_EN | SWITCH_PERIOD(6) | VCCVAL_15V | SW_MODE_DISABLE | UVLOVAL_10V | OC_CD_SHUTDOWN | EXT_CLK | OSC_SEL(3));

   // x0000101    5 = 3 uS (5 * 0.5 + 0.5)
//   SetParam(TON_MIN, OnOffTimes(3.0));      //

   // x00101001   41 = 21 uS (41 * 0.5 + 0.5)
   SetParam(TOFF_MIN, OnOffTimes(21.0));     //

   // 01000111
   // ^^^^     TOFF_FAST   4 = 10 uS (4 * 2 + 2)
   //     ^^^^ FAST_STEP   7 = 16 uS (7 * 2 + 2)
   SetParam(T_FAST, FastStepTimes(10, 16));       //

   // xxxx000011000110
   //     ^             WD_EN    0  ext clock monitoring disabled
   //      ^^^          TBOOST   0  overboost setting
   //         ^^^       IGATE    6  gate current (6 = 64 mA)
   //            ^^^^^  TCC      6
   SetParam(GATECFG1, IGATE(6) | TCC(6));     //

   // 01000000
   // ^^^      TBLANK   current sense blanking  375 nS (2 * 125 + 125)
   //    ^^^^^ TDT      switch dead time        125 nS (0 * 125 + 125)
   SetParam(GATECFG2, BridgeTimes(375, 125));     //

   // xxx01000
   //    ^^^^^ OCD_TH   overcurrent sense. range is 0-31.
   SetParam(OCD_TH, Set_VDS_Voltage(25));

   // x0000100
   //  ^^^^^^^ holding current, range is 0-127, 127 = 30A => 42=10A, range here is 0-42
   SetParam(TVAL_HOLD, Set_TVAL_X_Value(DataInputs.GetEntry(DB_STOP_POWER).HtmlValue.toInt()));

   // x0010010
   //  ^^^^^^^ run current, range is 0-127, 127 = 30A => 42=10A, range here is 0-42
   SetParam(TVAL_RUN, Set_TVAL_X_Value(DataInputs.GetEntry(DB_RUN_POWER).HtmlValue.toInt()));

   // x0011001
   //  ^^^^^^^ accel current, range is 0-127, 127 = 30A => 42=10A, range here is 0-42
   SetParam(TVAL_ACC, Set_TVAL_X_Value(DataInputs.GetEntry(DB_ACCEL_POWER).HtmlValue.toInt()));

   // x0011001
   //  ^^^^^^^ decel current, range is 0-127, 127 = 30A => 42=10A, range here is 0-42
   SetParam(TVAL_DEC, Set_TVAL_X_Value(DataInputs.GetEntry(DB_DECEL_POWER).HtmlValue.toInt()));

   // xxx0 0001 0100 1000
   //    ^ ^^^^ ^^^^ ^^^^  max speed   328  => 5000 * 0.065536 = 328
   SetParam(MAX_SPEED, StepsPerSecondToMaxSpeed(15000.0));
   // Note that reset value for MIN_SPEED is 0.

   // xxx0 0100 0000 0000
   //    ^ ^^^^ ^^^^ ^^^^
   sps = RPMtoSPS(FULL_STEP_SWITCH_RPM);
   P_MOTR(Serial.printf("Switch SPS:  %d\n", sps));
   SetParam(FS_SPD, BOOST_ON | StepsPerSecondToMaxSpeed(sps));

   // xxxx 0000 1000 1010
   //      ^^^^ ^^^^ ^^^^  accel rate  138  => 2000 * 0.068719 = 138
   SetParam(ACCEL, ConvertAcceleration(mAccelRate));
   SetParam(DECEL, ConvertAcceleration(mDecelRate));
}
