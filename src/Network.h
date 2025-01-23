// Network.h
//
#ifndef  _NETWORK_H
#define  _NETWORK_H

#include "Main.h"
#include "EspNow.h"
#include "SensorData.h"


typedef struct 
{
   uint8_t           address[CHARS_IN_ADDRESS_STRING + 1];
   SensorStateType   sensorState;
   uint8_t           retries;
} AddressTableEntryType;



class CNetwork
{
public:
   CNetwork(void);

   boolean Init(uint32_t channel);

   void StartSample(void);

   static void Task_ProcessReceivedPacket(void* parameter);

   static void Task_SampleManager(void* parameter);

   void ByteToMACAddress(uint8_t* input, char* output);
      
   static void TimerLink(xTimerHandle pxTimer);

   boolean IsNodePresent(uint16_t node);

   void TimerDone(xTimerHandle pxTimer);

   void PrintID(MessageIdType id);

   void SetCurrentTask(TaskType task);
   TaskType GetCurrentTask(void) const;


   void GetTestSampleSet(void);  // if SIMULATING


//-------------------------------------------------------------------
private:

   void RunSampleManagerTask(TaskType task);

   void SendThePacket(uint32_t node);

   void InsertCRC(PacketType* packet);

   boolean CheckMessageCRC(PacketType* packet);

   uint16_t ComputeCRC16(PacketType* packetPtr, uint32_t count);

   uint16_t CrcInnerLoop(uint16_t crc);

   void CrcHandler(void);

   void StartTimer(TimerIdType type, TickType_t delay);

   xQueueHandle GetProcessQueueHandle(void) const;

   int FindMatchingNode(uint8_t* address);

   void CancelTimeout(void);

   void MACToByteAddress(uint8_t* input, uint8_t* output);

   void PrintTaskType(TaskType id);

//-------------------------------------------------------------------
private:
   xQueueHandle      xProcessQueue;

   xTimerHandle      xTimer;

   PacketType        mPacketSent;

   TimerIdType       mCurrentTimer;

   TaskType          mCurrentTask;
};



//-------------------------------------------------------------------
//
inline xQueueHandle CNetwork::GetProcessQueueHandle(void) const
{
   return xProcessQueue;
}

//-------------------------------------------------------------------
//
inline void CNetwork::SetCurrentTask(TaskType task)
{
   mCurrentTask = task;
}

//-------------------------------------------------------------------
//
inline TaskType CNetwork::GetCurrentTask(void) const
{
   return mCurrentTask;
}

#endif   // _NETWORK_H
