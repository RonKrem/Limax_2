#include <EspNow.h>
#include "Main.h"
#include "Buttons.h"
#include "DataInputs.h"
#include "Eprom.h"
#include "SDCard.h"
#include "Sensors.h"
#include "Motor.h"
#include "MotorControl.h"

using namespace std;


#define  PWR_ENABLE           32    // power enable
#define  LED_PORT             25
#define  SDA_PIN              21
#define  SCL_PIN              22


#ifdef SOFT_AP
// Replace with your network credentials for WIFI_AP
const char* ssid = "Kremford";
const char* password = "123456789";
#else
// Replace with your network credentials for WIFI_AP_STA
const char* ssid = "Telstra076B29";
const char* password = "7h8nfrx4s9";
#endif


CLedScreen LedScreen;
AsyncWebServer server(80);    // create AsyncWebServer object on port 80
AsyncWebSocket ws("/ws");     // create a WebSocket object
AsyncEventSource events("/events");

SPIClass theSPI = SPIClass(VSPI);

CEspNow EspNow;
CSensors Sensors;
JSONVar readings;
CButtons Buttons;
CDataInputs DataInputs;
CMotor Motor;
CMotorControl MotorControl;
CFileSystem FileSystem;
CEprom Eprom;
CSDCard SDCard;

xTimerHandle xIntervalTimer;
xTimerHandle xStepTimer;
xTimerHandle xSafetyTimer;

xQueueHandle xDisplayQueue;
xQueueHandle xStepQueue;

String DataFilename;    // contains the current data file path
char OledText[40];
Flags Flag;
// Json Variable to Hold Sensor readings.
JSONVar jSonValues;
JSONVar jSonReadings;
IntervalTimerType IntervalTimerState;
StepTimerType StepTimerState;
uint32_t TestElapsedSeconds;


//-----------------------------------------------------------------------------
// Initialize WiFi.
//
boolean InitWiFi(void) 
{
   int channel;

#ifdef SOFT_AP
   WiFi.mode(WIFI_AP_STA);
   WiFi.softAP(ssid, password, WIFI_CHANNEL);
   Serial.println(WiFi.softAPIP());
#else
   WiFi.mode(WIFI_AP_STA);
   WiFi.begin(ssid, password);
   Serial.println("Connecting to WiFi...");
   while (WiFi.status() != WL_CONNECTED) 
   {
      Serial.print('.');
      delay(1000);
   }

   // Ensure channel matches request.
   Serial.println();
   channel = WiFi.channel();
   Serial.println(WiFi.localIP());
   Serial.println(channel);
#endif

   return true;
}

//-----------------------------------------------------------------------------
// Unpack the input string into an array of Strings. Return the number of words
// found or 0 for none.
//
int Unpacker(char *data, vector<String> &word)
{
   char* ptr = (char*)data;
   int words = 0;
   String sep = ",";

   ptr = strtok((char*)data, sep.c_str());
   while (ptr != NULL)
   {
      words++;
      // We are processing a javascript array.
      word.push_back(ptr);
      ptr = strtok(NULL, sep.c_str());
   }

   return words;
}

//-----------------------------------------------------------------------------
//
void NotifyClients(String state) 
{
   Serial.println("NotifyClients");
   ws.textAll(state.c_str());
}

//-----------------------------------------------------------------------------
//
void HandleWebSocketMessage(void *arg, uint8_t *data, size_t len) 
{
   AwsFrameInfo *info = (AwsFrameInfo*)arg;
   vector<String> word;
   int words;
   int i;

   Serial.println("HandleWebSocketMessage");

   if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) 
   {
      data[len] = 0;
      if ((words = Unpacker((char*)data, word)) > 0)
      {
         for (i=0; i<word.size(); i++)
         {
            Serial.printf(" <%s> ", word[i].c_str());
         }
         Serial.printf("\n");
         
         if (word[0] == "states")
         {
            // Handle a "states" message
            //
            Serial.println(" <states> Calling GetOutputStates\n");
            NotifyClients(Buttons.GetButtonStates());
         }
         else if (word[0] == "checkbox")
         {
            // Handle a "checkbox" message
            //
            if (word.size() > 1)
            {
               Serial.println(" <checkbox> Calling ProcessButtons");
//               Control.ProcessButtons(word[1], word[2]);
            }
            else
            {
               Serial.println("ignored");
            }
         }
         else if ((word[0] == "isnumber") || (word[0] == "istext"))
         {
            // Handle an "isnumber" or "istext" message
            //
            Serial.printf("word[0]: %s,  word[1]: %s,  word[2]: %s\n", word[0].c_str(), word[1].c_str(), word[2].c_str());
            DataInputs.MatchAndUpdate(word[1], word[2]);

            // Copy and update to the client.
            //
            NotifyClients(DataInputs.GetCurrentSpiffsValues());
         }
      }
   }
}

//-----------------------------------------------------------------------------
//
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) 
{
   Serial.printf("onEvent: ");

   switch (type) 
   {
      case WS_EVT_CONNECT:
         Serial.printf("OnEvent: WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
         break;
      case WS_EVT_DISCONNECT:
         Serial.printf(" client #%u disconnected\n", client->id());
         break;
      case WS_EVT_DATA:
//         HandleWebSocketMessage(arg, data, len);
         break;
      case WS_EVT_PONG:
      case WS_EVT_ERROR:
         break;
  }
}

//-----------------------------------------------------------------------------
// This will change the inter-sample delay.
//
void StartIntervalTimer(IntervalTimerType type, uint32_t delaymSecs)
{
   Serial.printf("StartIntervalTimer %d  %d\n", type, delaymSecs);
   IntervalTimerState = type;
   xTimerChangePeriod(xIntervalTimer, delaymSecs / portTICK_PERIOD_MS, 10);
}

//-----------------------------------------------------------------------------
// The sample interval time has expired. Here we need to recompute the delay
// time as the user may have made a change.
//
void IntervalTimerDone(xTimerHandle pxTimer)
{
   uint32_t interval;

   // A timer period has expired. What happens here depends
   // on what the <IntervalTimerState> has been set to.
   //
//   PRNTLN2(Serial.printf("IntervalTimer done %d\n", IntervalTimerState));
   switch (IntervalTimerState)
   {
   case NO_INTERVAL_TIMER:
      break;
   
   case DO_SAMPLE:
      // This interval timer runs continuously at the period set by a
      // call to StartIntervalTimer.
      //
//      Serial.println("Start timer");
      Flag.DoSample = true;                     // start this sample
      break;

   default:
      break;
   }
}

//-----------------------------------------------------------------------------
//
StepTimerType GetStepTimerState(void)
{
   return StepTimerState;
}

//-----------------------------------------------------------------------------
//
void StartStepTimer(StepTimerType type, uint32_t delayMSecs)
{
//   Serial.printf("StartStepTimer %d\n", type);
   StepTimerState = type;
   xTimerChangePeriod(xStepTimer, (delayMSecs / portTICK_PERIOD_MS), 10);
}

//-----------------------------------------------------------------------------
// A step timer interval has expired. The timer has automatically restarted
// its 200 Msec interval. Here we count up to one second and signal the 
// Hyperdrive to step and provide 200 mSec step signal as required.
//
void StepTimerDone(xTimerHandle pxTimer)
{
   static uint8_t littleTick = 0;
   uint32_t message;

   // A timer period has expired. What happens here depends
   // on what the CurrentTimer has been set to.
   //
   switch (StepTimerState)
   {
   case NO_STEP_TIMER:
      break;
   
   case DO_STEP:

      // This timer ticks forever at a 1 second rate, providing a standard
      // clock for the system and in particular to trigger the motor speed 
      // updates for the motor when it is running. The motor update is 
      // passed to the motor code via the xStepQueue so the motor
      // software runs on a different thread.
      //
      TestElapsedSeconds++;

      // Message the motor software Task_ProcessStepTimer with the one 
      // second tick. If the BUTTON_TEST_CONTROL button is pressed the
      // motor will run at the currently computed speed. The message
      // itself has no function.
      //
      if (xQueueSendToBack(xStepQueue, &message, portMAX_DELAY) != pdTRUE)
      {
         Serial.println("xStepQueue is full");
      }
      break;

   default:
      break;
   }
}







//-----------------------------------------------------------------------------
//
void setup() 
{
   // Serial port for debugging purposes
   Serial.begin(115200);
   Serial.println("\n\n======== Startup");

   // Setup Qwiic pins.
   Wire.setPins(SDA_PIN, SCL_PIN);

   pinMode(PWR_ENABLE, OUTPUT);
   digitalWrite(PWR_ENABLE, HIGH);
   pinMode(LED_PORT, OUTPUT);
   digitalWrite(LED_PORT, HIGH);

   // Start up the stepper controller.
   //
   delay(100);
//   MotorControl.Init();
//   Motor.Configure();
//   Motor.SetParam(ALARM_EN, 0x07);
   delay(100);
//   Serial.printf("Motor status: %04X\n", Motor.GetMotorStatus());
//   Motor.SetStepMode(4);
//   Motor.SetAccelRate(1000);
//   Motor.SetDecelRate(1000);

   // Configure the LittleFS eeprom memory.
   //
   if (!Eprom.Begin())
   {
      Serial.println("LittleFS mount error");
      return;
   }
//   Eprom.ListDir("/", 0);

   // Configure the SDCard memory. Verify the directory
   // structure is correct.
   //
   if (SDCard.Begin() != CARD_SUCCESS)
   {
      Serial.println("Card Init failed");
      while (1);
   }
   SDCard.VerifyDataFolder(DATA_FOLDER);
//   SDCard.ListDir("/", 1);

   LedScreen.Init();
   LedScreen.WriteLine1("Limax Website");

   // Configure the LittleFS paths for all data inputs and
   // the relevant buttons. Must be done before either are
   // accessed.
   //
   DataInputs.RestoreSavedValues();
   Buttons.RestoreSavedStates();

//   GetTurnInCms();

   
   if (!InitWiFi())
   {
      Serial.println("Cannot access WiFi");
      while (1);
   }
//   ws.onEvent(onEvent);
   server.addHandler(&ws);

//    // Render root / page
//    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
//    {
//       Serial.println("Sending /home.html");
//       request->send(LittleFS, "/home.html", String(), false, Processor);
//    });

//    // Render style page
//    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
//    {
//       Serial.println("Sending /style.css");
//       request->send(LittleFS, "/style.css", "text/css");
//    });

//    server.on("/upbutton", HTTP_GET, [](AsyncWebServerRequest *request)
//    {
//       Serial.println("upbutton");
//       digitalWrite(LED_PORT, LOW);
//       request->send(LittleFS, "/home.html", String(), false, Processor);
//    });

//    server.on("/offbutton", HTTP_GET, [](AsyncWebServerRequest *request)
//    {
//       digitalWrite(LED_PORT, HIGH);
//       request->send(LittleFS, "/home.html", String(), false, Processor);
//    });

//    server.addHandler(&events);
//    server.begin();
}

//-----------------------------------------------------------------------------
//
void loop() 
{
  // put your main code here, to run repeatedly:
}

