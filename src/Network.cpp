// Network.cpp
//
#include "Network.h"
#include "SensorData.h"
#include "ManageSamples.h"
#include "Timers.h"
#include "Ledscreen.h"
#include "LimaxTime.h"
#include "MotorControl.h"

#include "WellSimulator.h"

//#define  PRINT_NETWORK
#ifdef PRINT_NETWORK
#define  P_NET(x)   x;
#else
#define  P_NET(x)   // x
#endif

//#define  CHECK_FOR_MISSING_NODES

#define  TIMEOUT_INTERVAL        1000
#define  MAX_RETRIES             2
#define  INCOMING_QUEUE_SIZE     3
#define  PROCESS_QUEUE_SIZE      3

extern CNetwork Network;
extern CManageSamples ManagemSamples;
extern CTimers Timers;
#ifdef LED_SCREEN      
extern CLedScreen LedScreen;
#endif
extern CLimaxTime LimaxTime;

#ifdef SIMULATING
extern CWellSimulator WellSimulator;
extern CMotorControl MotorControl;
#else
extern CEspNow EspNow;
#endif

extern TaskHandle_t ReceivedPacketTask;
extern TaskHandle_t ProcessorTask;
extern TaskHandle_t ManageSamplesTask;

extern xQueueHandle xIncomingQueue;
extern xQueueHandle xNewSampleQueue;

extern DynamicData Temp;
extern Flags Flag;
extern char OledText[];

// Currently the set of well nodes are defined here by their addresses.
// Probably will be changed to automatically find these addresses.
//
AddressTableEntryType NodeAddressTable[] =
{
//   {"B0:B2:1C:9D:50:AC", SENSOR_NOT_PRESENT, 0},
//   {"B0:B2:1C:9D:62:2C", SENSOR_NOT_PRESENT, 0},
   {"B0:B2:1C:9D:62:50", SENSOR_NOT_PRESENT, 0},


   {"C0:5D:89:AA:63:00", SENSOR_NOT_PRESENT, 0},
   {"C0:5D:89:A9:2C:04", SENSOR_NOT_PRESENT, 0},
};
int NodeTableEntries = sizeof(NodeAddressTable) / sizeof(AddressTableEntryType);


SensorSampleType mSamples;
uint8_t mReplyAddress[BYTES_IN_ADDRESS];
PacketType mTxPacket;
PacketType mRxPacket;
uint8_t mCurrentNode;


//-------------------------------------------------------------------
//
CNetwork::CNetwork(void)
:  mCurrentTimer(TIMER_NO_TIMEOUT)
{
   for (uint16_t i=0; i < NodeTableEntries; i++)
   {
      NodeAddressTable[i].sensorState = SENSOR_NOT_PRESENT;
      NodeAddressTable[i].retries = 0;
   }
   mCurrentNode = 0;

   Temp.entries = 0;
}

//-------------------------------------------------------------------
//
boolean CNetwork::Init(uint32_t channel)
{
   CNetwork* instance = static_cast<CNetwork*>(this);

   // Must be able to init the EspNow wireless interface.
   //
   Serial.println("Init EspNow");

#ifdef SIMULATING
   if (WellSimulator.Init(channel))
#else   
   if (EspNow.Init(channel))
#endif
   {
      Serial.printf("There are %d network nodes.\n", NodeTableEntries);

      // Must be able to create the queue for incoming EspNow messages.
      //
      if (xIncomingQueue = xQueueCreate(INCOMING_QUEUE_SIZE, sizeof(RXQuePacket)))
      {
         // Create the task that will manage the incoming EspNow messages.
         //
         if (xTaskCreatePinnedToCore(  CNetwork::Task_ProcessReceivedPacket, 
                                       "Receiver", 
                                       3000, 
                                       NULL, 
                                       4, 
                                       &ReceivedPacketTask, 
                                       1) != pdPASS)
         {
            Serial.println("Cannot create Task_ProcessReceivedPacket");
            return false;
         }

         // The xProcessQueue is used to start the Task_SampleManager that manages
         // network events.
         //
         if (xProcessQueue = xQueueCreate(PROCESS_QUEUE_SIZE, sizeof(MessageIdType)))
         {
            // Queue ok - create the task.
            //
            if (xTaskCreatePinnedToCore(  CNetwork::Task_SampleManager, 
                                          "Process", 
                                          3000, 
                                          NULL, 
                                          3, 
                                          &ProcessorTask, 
                                          1) != pdPASS)
            {
               Serial.println("Cannot create Task_SampleManager");
               return false;
            }

            // Create the queue that transfers the well readings from this EspNow interface
            // to the client and sdcard update code. As the client can request the current
            // SDCard contents be uploaded, this may take longer than the inter-sample time,
            // and so this queue must absorb readings that occur while the data dump is
            // happening. 
            //
            // STILL TO WORK OUT IS HOW TO PREVENT THE Task_ManageSampleResults in ManageSamples
            // FROM TAKING RESULTS FROM THE QUEUE.
            //
            if (xNewSampleQueue = xQueueCreate(SAMPLE_QUEUE_SIZE, sizeof(SensorSampleType)))
            {
               // While this task is in ManageSamples, it is created here as the
               // xNewSampleQueue must be created before the task.
               //
               if (xTaskCreatePinnedToCore(  CManageSamples::Task_ManageSampleResults, 
                                             "DataHndlr", 
                                             3000, 
                                             NULL, 
                                             2, 
                                             &ManageSamplesTask, 
                                             0) != pdPASS)
               {
                  Serial.println("Cannot create task Task_ManageSampleResults");
                  return false;
               }

               if (xTimer = xTimerCreate("Timer", pdMS_TO_TICKS(1000), pdTRUE, (void*)0, instance->TimerLink))
               {
                  // Define the background spin time for the sample timer.
                  //
                  StartTimer(TIMER_NO_TIMEOUT, (TickType_t)20000);
#ifdef LED_SCREEN      
                  if (Flag.UseLedScreen) LedScreen.WriteLine3("Network OK");
#endif                  

                  return true;
               }

               // All other returns are errors.
               //
               else
               {
                  Serial.println(" Could not create xTimer");
               }
            }
            else
            {
               Serial.println(" Could not create xNewSampleQueue");
            }
         }
         else
         {
            Serial.println(" Could not create xProcessQueue");
         }
      }
      else
      {
         Serial.println(" Could not create xIncomingQueue");
      }
   }

   Serial.println(" Network was not initialised");

   return false;
}

//-----------------------------------------------------------------------------
// This is the call to have the node boxes initiate their samples.
//
void CNetwork::StartSample(void)
{
   mCurrentNode = 0;
   
   memset((void*)&mSamples, 0, sizeof(SensorSampleType));  // zero the data

   // Start the Task_SampleManager.
   //
   RunSampleManagerTask(TASK_SEND_MASTER);
}

//-----------------------------------------------------------------------------
// Send an xProcessQueue message to the Task_SampleManager. The latter is a state
// machine supervising the movements between states. The message currently
// has no valid contents.
//
void CNetwork::RunSampleManagerTask(TaskType task)
{
   if (xQueueSendToBack(xProcessQueue, (uint8_t*)&task, portMAX_DELAY) != pdTRUE)
   {
      Serial.println("xProcessQueue full");
   }
}

//-----------------------------------------------------------------------------
// Manage all events including establishing and re-establishing the network.
//
// Initial task is TASK_SEND_MASTER.
// A mTxPacket is assembled here with the id MESSAGE_SET_MASTER and send to
// the designated node. There are three possible situations as a result.
//  1. The timeout event happens.
//  2. A reply returns but fails the CRC test.
//  3. The reply Id matches as does the sending mac address.
//
// Case 1: Typically the node does not exist. The <state> entry is flagged 
// as SENSOR_NOT_PRESENT.
//
// Case 2: The node replied, but the packet was corrupted in some way.
// The <state> is flagged as MESSAGE_CRC_ERROR.
//
// Case 3: The received packet has the correct CRC and source mac address. The
// <state> is flagged as SENSOR_PRESENT.
//
// In all cases the mCurrentNode is moved on until all nodes have been probed.
// When that occurs the mCurrentNode is reset and the mCurrentTask set to 
// TASK_DO_SAMPLING.
//
void CNetwork::Task_SampleManager(void* parameter)
{
   MessageIdType dummy;
   TaskType currentTask;
   int count;

   while (true)
   {
      // Wait for an incoming <mCurrentTask> message defining the next step to be done.
      //
      xQueueReceive(Network.GetProcessQueueHandle(), (TaskType*)&currentTask, portMAX_DELAY);
// #ifdef SIMULATING
//       float slugDepth;

//       Serial.println("Asking for the sensor samples.");
//       slugDepth = MotorControl.GetSlugDepth();

      
//       if (xQueueSendToBack(xNewSampleQueue, &mSamples, portMAX_DELAY) != pdTRUE)
//       {
//          Serial.println("xNewSampleQueue full");
//       }

//       // Ready for next.
//       //
//       mCurrentNode = 0;

// #else
      if (mCurrentNode == 0)
      {
         mSamples.actualEntries = 0;
      }
   
      Network.SetCurrentTask(currentTask);
//      sprintf(OledText, "currentTask %d", currentTask);
//      LedScreen.WriteLine1(OledText);
      P_NET(Serial.printf("Task_SampleManager: node %d  ", mCurrentNode));
//      Network.PrintTaskType(currentTask);

      switch (currentTask)
      {
         //----------------------------------------------------------------------------------------
         case TASK_SEND_MASTER:
            // If CHECK_FOR_MISSING_NODES is defined, then on each cycle EVERY node will be probed 
            // with a MESSAGE_SET_MASTER message, regardless of its <sensorState>.
            //
            //  If the node does not reply it will be the timer timeout message TIMER_PROBE_TIMEOUT
            //  event that sets the entry state to SENSOR_NOT_PRESENT for an inactive node and it
            //  will be skipped in the subsequent sample request cycle.
            //
            //  If the node for some reason now does reply, it will be the MESSAGE_MASTER_CONFIRMED
            //  EspNow reply that sets the node state to MESSAGE_ACTIVE meaning it will be probed
            //  for a sample in the subsequent sample request cycle.
            //
            //  The intent here is a node can come back on line after failing and will be 
            //  automatically re-included in the sample cycle.
            //
            // If CHECK_FOR_MISSING_NODES is NOT defined, then if the node sensorState is SENSOR_ACTIVE
            // it is assumed that node is active and working, so the mCurrentNode is stepped thus
            // skipping this node. 
            //
            //        Note this means if a node is powered down there is no way for it
            //        to be brought back on line.
            //
            // Do for all entries in the table.
            //
            if (mCurrentNode < NodeTableEntries)
            {
#ifndef CHECK_FOR_MISSING_NODES               
               // Skip this step if the node is currently active.
               //
               if (NodeAddressTable[mCurrentNode].sensorState == SENSOR_ACTIVE)
               {
                  P_NET(Serial.printf(" node %d is active. TASK_SEND_MASTER skipped\n", mCurrentNode));
                  mCurrentNode++;
                  Network.RunSampleManagerTask(TASK_SEND_MASTER);  // for next node
               }
               else
#endif               
               {
                  // Send our mac address to the mCurrentNode node. Note that
                  // it could fail.
                  //
                  P_NET(Serial.printf(" node %d address %s\n", mCurrentNode, NodeAddressTable[mCurrentNode].address));
                  mTxPacket.id = MESSAGE_SET_MASTER;
                  mTxPacket.Token++;
                  Network.SendThePacket(mCurrentNode);
                  Network.StartTimer(TIMER_PROBE_TIMEOUT, (TickType_t)TIMEOUT_INTERVAL);
               }
            }
            else
            {
               // The network building is complete - move on to sampling.
               //
               P_NET(Serial.printf("  all nodes done.\n\n"));
               mCurrentNode = 0;
               Network.RunSampleManagerTask(TASK_DO_SAMPLING);
            }
            break;

         //------------------------------------------------------------------------------
         case TASK_DO_SAMPLING:
            // For all nodes in the list...
            //
            if (mCurrentNode < NodeTableEntries)
            {
               // Confirm this node is ready and if so, request a sample.
               //
               if (NodeAddressTable[mCurrentNode].sensorState == SENSOR_ACTIVE)
               {
                  // Node is ready - request a sample. Note that it could have
                  // failed since last time, so start the timer.
                  //
                  P_NET(Serial.printf(" sampling: (node %d address %s)\n", mCurrentNode, NodeAddressTable[mCurrentNode].address));
                  mTxPacket.id = MESSAGE_TAKE_SAMPLE;
                  mTxPacket.Token++;
                  Network.SendThePacket(mCurrentNode);
                  Network.StartTimer(TIMER_PROBE_TIMEOUT, (TickType_t)TIMEOUT_INTERVAL);
               }
               else
               {
                  // Node not present - skip.
                  //
                  P_NET(Serial.printf("  node %d marked as not-present\n", mCurrentNode));
                  mCurrentNode++;
                  Network.RunSampleManagerTask(TASK_DO_SAMPLING);
               }
            }
            else
            {
               // All nodes have been sampled. Copy the sensor data set to the xNewSampleQueue.
               //
               mSamples.time = LimaxTime.GetLimaxTime(); // finally add time.
#ifdef SIMULATING
               mSamples.time = WellSimulator.GetSampleNumber();
#endif
               Serial.println("*** All nodes sampled. Sample set being queued for processing");

               // if (Flag.SchedulePlotEvent)
               // {
               //    Flag.SchedulePlotEvent = false;
               //    Flag.DoPlotEvent = true;
               // }
               // if (Flag.ScheduleRecordEvent)
               // {
               //    Flag.ScheduleRecordEvent = false;
               //    Flag.DoRecordEvent = true;
               // }

               if (xQueueSendToBack(xNewSampleQueue, &mSamples, portMAX_DELAY) != pdTRUE)
               {
                  Serial.println("xNewSampleQueue full");
               }

               // Ready for next.
               //
               mCurrentNode = 0;
            }
            break;

         //------------------------------------------------------------------------------
         default:
            Serial.printf(" No handler: CurrentTask: %d\n", currentTask);
            break;
      }
// #endif      
   }

   vTaskDelete(NULL);
}

//-----------------------------------------------------------------------------
//
void CNetwork::TimerDone(xTimerHandle pxTimer)
{
   // A timer period has expired. What happens here depends
   // on what the CurrentTimer has been set to.
   //
   //Serial.printf("Timer %d done\n", mCurrentTimer);
   switch (mCurrentTimer)
   {
      case TIMER_PROBE_TIMEOUT:
         // There was no response to the SET_MASTER packet to the CurrentNode.
         // Here we update the NodeAddressTable with the correct state, step to
         // the next node, and restart the Task_Process.
         //
         P_NET(Serial.printf("  timeout: PROBE_TIMEOUT  node %d not present  ", mCurrentNode));
//         PrintTaskType(mCurrentTask);
         NodeAddressTable[mCurrentNode].sensorState = SENSOR_NOT_PRESENT;
         switch (mCurrentTask)
         {
            case TASK_DO_SAMPLING:
               NodeAddressTable[mCurrentNode].sensorState == SENSOR_NOT_PRESENT;
               mCurrentNode++;
               RunSampleManagerTask(TASK_DO_SAMPLING);
               break;

            default:
            case TASK_SEND_MASTER:
               mCurrentNode++;
               RunSampleManagerTask(TASK_SEND_MASTER);
               break;
         }
         break;

      case TIMER_NO_TIMEOUT:
      default:
         break;
   }

   mCurrentTimer = TIMER_NO_TIMEOUT;
}

//-----------------------------------------------------------------------------
// Received packets can be commands/requests or acknowledgements of packets
// sent. Acks allow the data flow to continue as in a state machine, but some
// acks can be an indication that that received packet was corrupted, and in
// those cases we re-send the saved packet.
//
void CNetwork::Task_ProcessReceivedPacket(void* parameter)
{
   RXQuePacket q;
   uint8_t macStr[CHARS_IN_ADDRESS_STRING];
   int index;
   String myText;
   char t[20];

   while (true)
   {
      // Wait for an incoming message from a node via EspNow.
      //
      xQueueReceive(xIncomingQueue, (RXQuePacket*)&q, portMAX_DELAY);
      P_NET(Serial.println("xIncomingQueue packet arrived"));
      
      // Copy the incoming message to local storage.
      //
      memcpy((uint8_t*)&mRxPacket, (uint8_t*)&q.pkt, sizeof(PacketType));
      memcpy((uint8_t*)&mReplyAddress, (uint8_t*)&q.addr, BYTES_IN_ADDRESS);

      // for(int i=0; i<BYTES_IN_ADDRESS; i++)
      // {
      //    Serial.printf("%02X ", q.addr[i]);
      // }
      // Serial.println();

      // Save a copy of the sender's mac address locally in macStr.
      //
      Network.ByteToMACAddress((uint8_t*)&mReplyAddress, (char*)&macStr);

      P_NET(Serial.printf("Task_ProcessReceivedPacket: "));
//      Network.PrintID(mRxPacket.id);
      P_NET(Serial.printf("  token %d  received from: %s\n", mRxPacket.Token, macStr));

      // Serial.printf("%d: %d %f %d %f %d\n",  mRxPacket.id, 
      //                                     mRxPacket.Token, 
      //                                     mRxPacket.pressure, 
      //                                     mRxPacket.pressureUnits, 
      //                                     mRxPacket.temperature, 
      //                                     mRxPacket.temperatureUnits);

      index = Network.FindMatchingNode(macStr); // returns NodeTableEntries if no match
      P_NET(Serial.printf("Network: Reply MAC address: %d\n", index));

      // This node needs to be in our NodeAddressTable, ie a known entity.
      //
      if (index < NodeTableEntries)
      {
         // Check for transmission errors.
         //
         if (Network.CheckMessageCRC(&mRxPacket))
         { 
            // Message looks ok. Cancel any timeout 
            //
            Network.CancelTimeout();
//            sprintf(OledText, "PacketID %d", RxPacket.id);
//            LedScreen.WriteLine2(OledText);

            switch (mRxPacket.id)
            {
            case MESSAGE_MASTER_CONFIRMED:
               P_NET(Serial.printf("  node: %d", index));
               P_NET(Serial.println(" confirmed reply to SET_MASTER message"));

               // Set this node as ready for sampling.
               //
               NodeAddressTable[index].sensorState = SENSOR_ACTIVE;

               // Move on to next node.
               //
               mCurrentNode++;
               Network.RunSampleManagerTask(TASK_SEND_MASTER);
               break;

            case MESSAGE_SAMPLE:
               // The sensor data for this node has arrived and is now in the mRxPacket.
               // The <index> variable describes from which node. To allow post-analysis 
               // the node mac address is included in each SensorDataSet structure. 
               // Once a node scan completes the data set is placed in the xNewSampleQueue
               // for access by others.
               //
               mSamples.actualEntries++;
               Network.MACToByteAddress((uint8_t*)macStr, (uint8_t*)&mSamples.SensorData[index].mac);
               mSamples.SensorData[index].pressure.value = mRxPacket.pressure;
               mSamples.SensorData[index].pressure.unit = mRxPacket.pressureUnits;
               mSamples.SensorData[index].temperature.value = mRxPacket.temperature;
               mSamples.SensorData[index].temperature.unit = mRxPacket.temperatureUnits;

               P_NET(Serial.printf(" %d  PRESSURE:    %f  %d\n", index, mSamples.SensorData[index].pressure.value, mSamples.SensorData[index].pressure.unit));
//               Serial.printf("  TEMPERATURE: %f  %d\n", mSamples.SensorData[index].temperature.value, mSamples.SensorData[index].temperature.unit);

               sprintf(t, "PRESS %7.4f", mSamples.SensorData[index].pressure.value);
               myText = t;
//               LedScreen.WriteLine2(myText);
//               sprintf(t, "TEMP  %7.4f", mSamples.SensorData[index].temperature.value);
//               myText = t;
//               LedScreen.WriteLine3(myText);
//               sprintf(OledText, "Temp %6.3f", mSamples.SensorData[index].temperature.value);
//               LedScreen.WriteLine3(OledText);

               mCurrentNode++;
               Network.RunSampleManagerTask(TASK_DO_SAMPLING);
               break;

            case MESSAGE_CRC_ERROR:
               Serial.println("Transmitted (from us) CRC error");
               // Our last message to the <mCurrentNode> node had a CRC error on arrival.
               // Response is as for a received CRC error - we re-send the last message.
               //
               Network.CrcHandler();
               break;

            case MESSAGE_SAMPLE_TIMEOUT:
               // Something wrong in the node software. Can't do much from
               // here except signal the fact. For the moment we just move 
               // on to next node.
               Network.MACToByteAddress((uint8_t*)macStr, (uint8_t*)&mSamples.SensorData[index].mac);

               mCurrentNode++;
               Network.RunSampleManagerTask(TASK_DO_SAMPLING);
               break;

            case MESSAGE_NO_MASTER:
               Serial.println("  MESSAGE_NO_MASTER");

               // A node has sent a message saying it has no master address by sending
               // a broadcast message address to everyone. We have already tested it
               // exists in our slave list and its matching entry is in <index>. Here
               // we reset the entry to its initial state.
               //
               NodeAddressTable[index].sensorState = SENSOR_NOT_PRESENT;
               Serial.printf("   Match found in %d. Node reset\n", index);

               // Unfortunately this means this node will not have an entry for 
               // this scan cycle. 
               //
// Remember to add a dummy reading set in the output.  (?????)
               //
               mCurrentNode++;
               break;

            default:
               Serial.printf("  Default!!!  Node %d  Id: %d\n", index, mRxPacket.id);
               break;
            }
         }
         else
         {
            // This received packet failed our crc test. We will assume the sender
            // was the current <mCurrentNode> and this is a corrupted reply to our
            // current request. This is presumably the answer to our previous
            // request, so a valid solution is to send the previously saved request
            // again and increment the <retries> field. If the <retries> is already 
            // at MAX_RETRIES we clear the <sensorState> field which restarts the
            // network setup for this node.
            //
            Serial.println("Received (to us) CRC error");
            Network.CrcHandler();
         }
      }
      else
      {
         // There is no matching node ???
         Serial.println("Network: No matching node!!");
      }
   }

   vTaskDelete(NULL);
}

//-----------------------------------------------------------------------------
//
void CNetwork::StartTimer(TimerIdType type, TickType_t delay)
{
//   P_NET(Serial.printf("Network: StartTimer: %d\n", delay)); 
   mCurrentTimer = type;
   xTimerChangePeriod(xTimer, delay, 0);
}

//-----------------------------------------------------------------------------
//
void CNetwork::TimerLink(xTimerHandle pxTimer)
{
   Network.TimerDone(pxTimer);
}

//-----------------------------------------------------------------------------
//
void CNetwork::SendThePacket(uint32_t node)
{
   MacAsBytesType destination;

   InsertCRC(&mTxPacket);

   // Unpack the colon separated address into bytes.
   //
   MACToByteAddress((uint8_t*)&NodeAddressTable[node].address, (uint8_t*)&destination);

   // Keep a copy in case transmission errors.
   //
   memcpy((uint8_t*)&mPacketSent, (uint8_t*)&mTxPacket, sizeof(PacketType));

   // Register the node and send the packet.
   //
#ifdef SIMULATING
   WellSimulator.RegisterPeer((uint8_t*)&destination);  
   WellSimulator.SendData((uint8_t*)&destination, (uint8_t*)&mTxPacket, sizeof(PacketType));
#else    
   EspNow.RegisterPeer((uint8_t*)&destination);
   EspNow.SendData((uint8_t*)&destination, (uint8_t*)&mTxPacket, sizeof(PacketType));
#endif
//   Serial.printf(" SendThePacket: Token: %d ", mTxPacket.Token);
//   PrintID(mTxPacket.id);

}

//-----------------------------------------------------------------------------
//
void CNetwork::CrcHandler(void)
{
   if (NodeAddressTable[mCurrentNode].retries < MAX_RETRIES)
   {
      // Try this node again.
      //
      Serial.println("Re-trying");
      NodeAddressTable[mCurrentNode].retries++;
      memcpy((uint8_t*)&mTxPacket, (uint8_t*)&mPacketSent, sizeof(PacketType));
      SendThePacket(mCurrentNode);
   }
   else
   {
      // Flag this node as non-existant and see if it can be re-started
      // later on.
      //
      Serial.println("Resetting");
      NodeAddressTable[mCurrentNode].sensorState = SENSOR_NOT_PRESENT;
      mCurrentNode++;
   }
}

//-----------------------------------------------------------------------------
//
boolean CNetwork::CheckMessageCRC(PacketType* packet)
{
   uint16_t CRC;
   uint16_t checkCRC;

   // Copy then zero the message CRC.
   //
   CRC = (uint16_t)packet->CRChi << 8;
   CRC |= (uint16_t)packet->CRClo & 0xff; 
   packet->CRChi = 0;
   packet->CRClo = 0;
   
   // Compute what should be a matching crc.
   //
   checkCRC = ComputeCRC16(packet, sizeof(PacketType));

//   Serial.printf("Sent CRC: %4X  Computed CRC: %4X\n", CRC, checkCRC);

   if (CRC == checkCRC) 
      return true; 
   else 
      return false;
}

//-------------------------------------------------------------------
// Unpack an input byte string such as "24:6f:28:8f:7c:74" into
// an output byte array with 6 entries.
//
void CNetwork::MACToByteAddress(uint8_t* input, uint8_t* output)
{
   int i;
   char b[3];
   byte n;

   b[2] = '\0';
   for (i=0; i<BYTES_IN_ADDRESS; i++)
   {
      b[0] = *input++;
      b[1] = *input++;
      n = strtol(b, 0, 16);
      *output++ = (uint8_t)n;
      input++;    // step over colon
   }
}

//-------------------------------------------------------------------
// Unpack an input byte string such as b[0], b[1], .. , b[5] into
// an output string such as "24:6f:28:8f:7c:74".
//
void CNetwork::ByteToMACAddress(uint8_t* input, char* output)
{
   // Copies the sender mac address to a string.
   //
   snprintf(output, CHARS_IN_ADDRESS_STRING, "%02X:%02X:%02X:%02X:%02X:%02X", 
            *(input+0), *(input+1), *(input+2), 
            *(input+3), *(input+4), *(input+5));
}

//-----------------------------------------------------------------------------
// Check whether a node is present.
//
boolean CNetwork::IsNodePresent(uint16_t node)
{
   if (node < NodeTableEntries)
   {
      if (NodeAddressTable[node].sensorState == SENSOR_ACTIVE)
      {
         return true;
      }
   }

   return false;
}

//-----------------------------------------------------------------------------
//
void CNetwork::InsertCRC(PacketType* packet)
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
uint16_t CNetwork::ComputeCRC16(PacketType* packetPtr, uint32_t count)
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
uint16_t CNetwork::CrcInnerLoop(uint16_t crc)
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

//-----------------------------------------------------------------------------
// Search the NodeAddressTable for a match to the parameter.
//
int CNetwork::FindMatchingNode(uint8_t* address)
{
   for (int i=0; i < NodeTableEntries; i++)
   {
      if (strncmp((const char*)address, (const char*)&NodeAddressTable[i].address, CHARS_IN_ADDRESS_STRING) == 0)
      {
         return i;
      }
   }

   return NodeTableEntries;
}

//-----------------------------------------------------------------------------
//
void CNetwork::CancelTimeout(void)
{
   StartTimer(TIMER_NO_TIMEOUT, (TickType_t)2);    // cancel timeout
}

//-----------------------------------------------------------------------------
//
void CNetwork::PrintID(MessageIdType id)
{
   String type;

   switch (id)
   {
      case MESSAGE_SET_MASTER:
         type = "MESSAGE_SET_MASTER";
         break;
      case MESSAGE_MASTER_CONFIRMED:
         type = "MESSAGE_MASTER_CONFIRMED";
         break;
      case MESSAGE_TAKE_SAMPLE:
         type = "MESSAGE_TAKE_SAMPLE";
         break;
      case MESSAGE_SAMPLE:
         type = "MESSAGE_SAMPLE";
         break;
      case MESSAGE_SAMPLE_TIMEOUT:
         type = "MESSAGE_SAMPLE_TIMEOUT";
         break;
      case MESSAGE_CRC_ERROR:
         type = "MESSAGE_CRC_ERROR";
         break;
      case MESSAGE_NO_MASTER:
         type = "MESSAGE_NO_MASTER";
         break;
      default:
         type = "UNKNOWN MESSAGE";
         break;
   }
   Serial.printf("  MessageId: %s\n", type.c_str());
}

//-----------------------------------------------------------------------------
//
void CNetwork::PrintTaskType(TaskType id)
{
   String type;

   switch (id)
   {
      case TASK_NO_TASK:
         type = "TASK_NO_TASK";
         break;
      case TASK_SEND_MASTER:
         type = "TASK_SEND_MASTER";
         break;
      case TASK_MASTER_REPLY:
         type = "TASK_MASTER_REPLY";
         break;
      case TASK_DO_SAMPLING:
         type = "TASK_DO_SAMPLING";
         break;
      default:
         type = "UNKNOWN TASK";
         break;
   }
   Serial.printf("  TaskId: %s\n", type.c_str());
}

