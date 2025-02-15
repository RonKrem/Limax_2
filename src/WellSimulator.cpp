// CSimulate.cpp
//
#include "WellSimulator.h"
#include "Data.h"
#include "Network.h"
#include "ManageSamples.h"
#include "MotorControl.h"

//#define  PRINT_SIM
#ifdef PRINT_SIM
#define  P_SIM(x)   x;
#else
#define  P_SIM(x)   // x
#endif

extern CData Data;
extern CWellSimulator WellSimulator;
extern CManageSamples ManageSamples;
extern CMotorControl MotorControl;

extern uint32_t DummyTime;
extern AddressTableEntryType NodeAddressTable[];
extern int NodeTableEntries;

extern xQueueHandle xIncomingQueue;
extern uint32_t TimeInSeconds;

#ifdef SIMULATING


//-------------------------------------------------------------------
//
CWellSimulator::CWellSimulator(void)
: mRecSuccess(0),
  mSendSuccess(0),
  mTableEntry(0),
  mTestElapsedTime(0)
{
   // These are the actual max amplitudes of the individual well 
   // sensor readings in the required reading units.
   //
   mAmplitude[0] = 10;
   mAmplitude[1] = 2;
   mAmplitude[2] = 0.6;

   // Mapping between sensor number (1, 2..) and the Data structure
   // entries.
   //
   mLookup[0] = 0;   // sensor 1 is Data entry 0
   mLookup[1] = 3;   // etc
   mLookup[2] = 6;

   mPhase[0] = 0;
   mPhase[1] = -60;
   mPhase[2] = -80;
   mSensorTimerState = SENSOR_NOTHING;

   mSlugDepth = 0;
}

//-----------------------------------------------------------------------------
// We arrive here after the simulated sensor response timeout.
//
void CWellSimulator::TimerDone(xTimerHandle pxTimer)
{
   switch (mSensorTimerState)
   {
         break;

      case SENSOR_PROBE:
         // The simulated sensor has replied.
         //
         GetTestSampleSet(mTableEntry, MotorControl.GetSlugDepth());

         mSensorTimerState = SENSOR_NOTHING;    // reset
         break;

      case SENSOR_NOTHING:
      default:
         break;
   }
}

//-----------------------------------------------------------------------------
// Callback function that will be executed when data is received
//
void CWellSimulator::OnDataReceive(const uint8_t* mac_addr, const uint8_t* incomingData, int len) 
{
   RXQuePacket q;

   P_SIM(Serial.println(" WellSimulator: Packet received"));
   memcpy((uint8_t*)&q.addr, mac_addr, BYTES_IN_ADDRESS);
   memcpy((uint8_t*)&q.pkt, incomingData, sizeof(PacketType));
   if (xQueueSendToBack(xIncomingQueue, (uint8_t*)&q, portMAX_DELAY) != pdTRUE)
   {
      Serial.println("xIncomingQueue full");
   }
}

//-----------------------------------------------------------------------------
//
uint32_t CWellSimulator::RegisterPeer(uint8_t* address)
{
   uint32_t retVal = 1;
   
   memcpy(mPeerInfo.peer_addr, address, BYTES_IN_ADDRESS);
   // Serial.printf("Peer address: ");
   // for (int i=0; i<BYTES_IN_ADDRESS; i++)
   // {
   //    Serial.printf("%02X ", mPeerInfo.peer_addr[i]);
   // }
   // Serial.println();
   mPeerInfo.channel = mChannel;
   mPeerInfo.encrypt = false;
   
   return retVal;
}

//-----------------------------------------------------------------------------
// This is the call from CNetwork to take a sample. Here we save the destination
// mac address and its index in the NodeAddressTable.
// The address is a 6 character array.
//
boolean CWellSimulator::SendData(uint8_t* address, uint8_t* packet, int size)
{
   int i;

   P_SIM(Serial.println("WellSimulator.SendData"));
//   for(i=0; i<BYTES_IN_ADDRESS; i++) Serial.printf("%02X ", *(address + i)); Serial.printf("Id: %d\n", *(packet + 4));
   
   memcpy((uint8_t*)&mNodeByteAddress, address, BYTES_IN_ADDRESS);
   memcpy((uint8_t*)&mTxPacket, packet, sizeof(mTxPacket));
   mTheToken = mTxPacket.Token;

   // Copy the byte address as MAC address to us.
   //
   ByteToMACAddress(address, (char*)&mMACaddr);

   // for (i=0; i<CHARS_IN_ADDRESS_STRING; i++)
   // {
   //    Serial.printf("%c", (char*)&mMACaddr[i]);
   // }
   // Serial.println();
//   Serial.printf("%s\n", mMACaddr);

   // Record its entry address in the NodeAddressTable.
   //
   mTableEntry = FindMatchingNode();

   switch (mTxPacket.id)
   {
      case MESSAGE_SET_MASTER:
         mReplyState = MESSAGE_MASTER_CONFIRMED;
         break;

      case MESSAGE_TAKE_SAMPLE:
         mReplyState = MESSAGE_SAMPLE;
         break;
   }
   // Start the timer that times out the sensor response time.
   //
   StartSampleTimer();

   return true;
}

//-----------------------------------------------------------------------------
// Load the SampleSet structure with the test data. Called from the timer.
//
void CWellSimulator::GetTestSampleSet(uint32_t wellNumber, float depth)
{
   RXQuePacket q;
   float period;
   float phase;
   float amplitude;
   float reading;
   
   // The intent here is to provide a single sine wave that is progressively
   // later at wells 2 and 3, and with progressivly lower amplitudes.
   // However, they are all the same frequency.
   // For well 0 the cosine is the slug depth with the sensor data
   // 180 degrees out of phase.
   // Well 1 is the cosine at -210 degrees and well 2 at -230 degrees.
   //
   P_SIM(Serial.printf("WellSimulator.GetTestSampleSet for %d\n", wellNumber));

   if (wellNumber == 0)
   {
      reading = -depth;
      reading *= 2.2;
      reading += ((float)(rand() % 100) - 50.0) / 500.0;
   } 
   else if (wellNumber == 1)
   {
      reading = -depth * 0.3;
      reading += ((float)(rand() % 100) - 50.0) / 400.0;
} 
   else if (wellNumber == 2)
   {
      reading = -depth * 0.09;
      reading += ((float)(rand() % 100) - 50.0) / 450.0;
}
      
   // Step the test elapsed time.
   //      
//   mTestElapsedTime += Data.GetDataEntryNumericValue(DB_RECORD_INTERVAL);

   memcpy((uint8_t*)&q.addr, (char*)&mNodeByteAddress, BYTES_IN_ADDRESS);
   q.pkt.Token = mTxPacket.Token;
   q.pkt.pressure = reading;
   q.pkt.pressureUnits = 24;

   q.pkt.id = mReplyState;

   InsertCRC(&q.pkt);
//   Serial.printf(" < WELL SAMPLE FOR %d >\n", wellNumber);

   if (xQueueSendToBack(xIncomingQueue, (uint8_t*)&q, portMAX_DELAY) != pdTRUE)
   {
      Serial.println("xIncomingQueue full");
   }
}

//-----------------------------------------------------------------------------
//
float CWellSimulator::DoCosine(float amplitude, float period, float phase)
{
   float value;

   value = amplitude * cos((2.0 * mTestElapsedTime / period + phase / 180) * PI);

   return value;
}

//-----------------------------------------------------------------------------
// Search the NodeAddressTable for a match to the parameter.
//
int CWellSimulator::FindMatchingNode(void)
{
   for (int i=0; i < NodeTableEntries; i++)
   {
      if (strncmp((const char*)&mMACaddr, (const char*)&NodeAddressTable[i].address, CHARS_IN_ADDRESS_STRING) == 0)
      {
//         Serial.printf("WellSimulator. Node is %d\n", i);
         return i;
      }
   }

   return NodeTableEntries;
}

//-------------------------------------------------------------------
// Unpack an input byte string such as b[0], b[1], .. , b[5] into
// an output string such as "24:6f:28:8f:7c:74".
//
void CWellSimulator::ByteToMACAddress(uint8_t* input, char* output)
{
   // Copies the sender mac address to a string.
   //
   snprintf(output, CHARS_IN_ADDRESS_STRING, "%02X:%02X:%02X:%02X:%02X:%02X", 
            *(input+0), *(input+1), *(input+2), 
            *(input+3), *(input+4), *(input+5));
}

//-----------------------------------------------------------------------------
//
void CWellSimulator::InsertCRC(PacketType* packet)
{
   uint16_t crc;

   packet->CRChi = 0;
   packet->CRClo = 0;
   crc = ComputeCRC16(packet, sizeof(PacketType));
//   Serial.printf("Sent CRC: %4X\n", crc);
   packet->CRChi = (uint8_t)(crc >> 8);
   packet->CRClo = (uint8_t)(crc & 0xff);

   // // Add an error.
   // if ((rand() % 200) < 60)
   // {
   //    packet->message.CRClo++;
   //    Serial.println1("Added CRC error -------------------------------------");
   // }
}

//-----------------------------------------------------------------------------
// Moving from the table driven version to the computed version changes the
// compute time from about 15 uSecs to about 67 uSecs, nearly five times
// longer. Still short enough for the application.
// Remember that the CRC table is in ROM and will not take up ram space.
//
uint16_t CWellSimulator::ComputeCRC16(PacketType* packetPtr, uint32_t count)
{
   uint8_t j;
   uint16_t CRCWord = 0xffff;
   uint8_t* ptr = (uint8_t*)packetPtr;

   for (j=0; j<count; j++)
   {
      CRCWord ^= (uint16_t)*ptr++;
      CRCWord = CrcInnerLoop(CRCWord);
   }

//   Serial.printf("\nComputed CRC: %4X\n", CRCWord);
   return CRCWord;
}

//-----------------------------------------------------------------------------
// Shared common inner CRC loop.
//
uint16_t CWellSimulator::CrcInnerLoop(uint16_t crc)
{
   uint32_t i;

   for (i = 8; i != 0; i--)
   {
      if ( (crc & 0x0001) != 0)
      {
         crc >>= 1;
         crc ^= 0xa001;
      }
      else
      {
         crc >>= 1;
      }
   }

   return crc;
}


#endif   // SIMULATING

