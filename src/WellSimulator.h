// CWellSimukator.h
//
// If SIMULATING is defined, this device simulates the ESPNow
// network of the three sensors, generating psuedo readings
// in response to requests.
//
#ifndef WELLSIMULATOR_H
#define WELLSIMULATOR_H

#include "Main.h"
#include "SensorData.h"

#define  SENSOR_RESPONSE_TIME       50       // millisecs

typedef enum
{
   SENSOR_NOTHING,
   SENSOR_PROBE,
} SensorStateEnum;



class CWellSimulator
{
public:
   CWellSimulator(void);

   boolean Init(int channel);

   void OnDataReceive(const uint8_t* mac_addr, const uint8_t* incomingData, int len);

   uint32_t RegisterPeer(uint8_t* address);

   boolean SendData(uint8_t* address, uint8_t* packet, int size);

   void SetSendState(int state);

   int GetSendState(void) const;

   static void SensorTimerLink(xTimerHandle pxTimer);

   void TimerDone(xTimerHandle pxTimer);

   void StartSampleTimer(void);

   float GetSlugDepth(void);

   void ClearSampleNumber(void);

   uint32_t GetSampleNumber(void) const;

private:

   void GetTestSampleSet(uint32_t welNumber);

   int FindMatchingNode(void);

   void ByteToMACAddress(uint8_t* input, char* output);

   void InsertCRC(PacketType* packet);
   uint16_t ComputeCRC16(PacketType* packetPtr, uint32_t count);
   uint16_t CrcInnerLoop(uint16_t crc);

   float DoCosine(const float amplitude, const float period, const float phase);

private:   
   esp_now_peer_info_t  mPeerInfo;
   PacketType           mPacket;
   int                  mSendSuccess;
   int                  mRecSuccess;
   int                  mChannel;

   uint32_t             mTheToken;

   xTimerHandle         xTimer;

   float                mAmplitude[NUM_SENSORS + 1];
   float                mLookup[NUM_SENSORS + 1];
   float                mPhase[NUM_SENSORS + 1];
   uint8_t              mTableEntry;

   SensorStateEnum      mSensorTimerState;

   uint8_t              mMACaddr[CHARS_IN_ADDRESS_STRING];  // matches entries in NodeAddressTable

   uint8_t              mNodeByteAddress[BYTES_IN_ADDRESS];

   PacketType           mTxPacket;
   MessageIdType        mReplyState;
   uint32_t             mTestElapsedTime;
   float                mSlugDepth;
};

//-------------------------------------------------------------------
//
inline void CWellSimulator::SetSendState(int state)
{
   mSendSuccess = state;
}

//-------------------------------------------------------------------
//
inline int CWellSimulator::GetSendState(void) const
{
   return mSendSuccess;
}

//-------------------------------------------------------------------
//
inline void CWellSimulator::ClearSampleNumber(void)
{
   mTestElapsedTime = 0;
}

//-------------------------------------------------------------------
//
inline uint32_t CWellSimulator::GetSampleNumber(void) const
{
   return mTestElapsedTime;
}

#endif   // WELLSIMULATOR_H
