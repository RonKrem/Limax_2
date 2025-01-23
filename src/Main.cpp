#include <EspNow.h>
#include "Main.h"
#include "Buttons.h"
#include "Data.h"
#include "Eprom.h"
#include "SDCard.h"
#include "Sensors.h"
#include "Motor.h"
#include "MotorControl.h"
#include "Timers.h"
#include "ManageSamples.h"
#include "Network.h"
#include "WellSimulator.h"
#include "LimaxTime.h"
#include "TempTMP116.h"

using namespace std;


#define  PWR_ENABLE           32    // power enable
#define  SDA_PIN              21
#define  SCL_PIN              22

#define  SOFT_AP
#ifdef SOFT_AP
// Replace with your network credentials for WIFI_AP
const char* ssid = "Kremford";
const char* password = "123456789";
#else
// Replace with your network credentials for WIFI_AP_STA
const char* ssid = "Telstra076B29";
const char* password = "7h8nfrx4s9";
#endif

extern CTimers Timers;

CLedScreen LedScreen;
AsyncWebServer server(80);    // create AsyncWebServer object on port 80
AsyncWebSocket ws("/ws");     // create a WebSocket object
AsyncEventSource events("/events");

SPIClass theSPI = SPIClass(VSPI);

CEspNow EspNow;
CSensors Sensors;
CButtons Buttons;
CData Data;
CMotor Motor;
CMotorControl MotorControl;
CFileSystem FileSystem;
CEprom Eprom;
CSDCard SDCard;
//CControls Controls;
CManageSamples ManageSamples;
CNetwork Network;
CLimaxTime LimaxTime;
CTempTMP116 tmp116;

#ifdef SIMULATING
CWellSimulator WellSimulator;
#endif
xTimerHandle xIntervalTimer;
xTimerHandle xStepTimer;
xTimerHandle xSafetyTimer;

xQueueHandle xDisplayQueue;
xQueueHandle xStepQueue;
xQueueHandle xIncomingQueue;
xQueueHandle xNewSampleQueue;

TaskHandle_t ProcessStepTask;
TaskHandle_t ReceivedPacketTask;
TaskHandle_t ProcessorTask;
TaskHandle_t ManageSamplesTask;

String DataFilename;    // contains the current data file path
char OledText[40];
Flags Flag;
boolean LedState;
// Json Variable to Hold Sensor readings.
JSONVar jSonValues;
JSONVar jSonReadings;
uint32_t Channel;
SensorSampleType SampleSet;
DynamicData Temp;
SemaphoreHandle_t xSDCardAccessMutex;



//extern ButtonDetails Button[]; 





const char *PARAM_INPUT_ID = "id";
const char *PARAM_INPUT_STATE = "checked";

float DummyTime;



//-----------------------------------------------------------------------------
// Initialize WiFi.
//
boolean InitWiFi(void) 
{
//   int channel;

#ifdef SOFT_AP
   LedScreen.WriteLine1("Connecting...");
   WiFi.mode(WIFI_AP_STA);
   WiFi.softAP(ssid, password, WIFI_CHANNEL);
   Serial.println(WiFi.softAPIP());
   sprintf(OledText, "%s", WiFi.softAPIP().toString());
   LedScreen.WriteLine1(OledText);
#else
   WiFi.mode(WIFI_STA);
   WiFi.begin(ssid, password);
   Serial.println("Connecting to WiFi...");
   LedScreen.WriteLine1("Connecting...");

   while (WiFi.status() != WL_CONNECTED) 
   {
      Serial.print('.');
      delay(1000);
   }
   sprintf(OledText, "%s", WiFi.localIP().toString());
   LedScreen.WriteLine1(OledText);
#endif

   // Ensure channel matches request.
   Serial.println();
   Channel = WiFi.channel();
   Serial.println(WiFi.localIP());
   Serial.println(Channel);

   return true;
}

//-----------------------------------------------------------------------------
//
String GetSensorReadings(void)
{
   String jsonString;

   jSonReadings["time"] = String(0);
   jSonReadings["slug"] = String(0);

   jsonString = JSON.stringify(jSonReadings);

   return jsonString;
}

//-----------------------------------------------------------------------------
//
void NotifyClients(String state) 
{
   Serial.println("NotifyClients: ");
   Serial.println(state);
   ws.textAll(state.c_str());
}

//-----------------------------------------------------------------------------
// Unpack the input string into an array of Strings. There will be three strings
// with a commas seperator, but the third string can gave embedded commas, so 
// the first operation is to change these first 2 commas to something else
// and use these to manage the splitting.
// Returns the number of words found or 0 for none.
//
int Unpacker(char *data, vector<String> &word)
{
   char* ptr = (char*)data;
   int words = 0;
   char inlieu[2];

   // Replace the first two commas.
   //
   strcpy(inlieu, "~");
   for (int i=0; i<strlen(data); i++)
   {
      if (*ptr == ',' && words < 2)
      {
         *ptr = inlieu[0];
         words++;
      }
      ptr++;
   }
//   Serial.printf("%s\n", data);

   // Now extract the three words.
   //
   words = 0;
   ptr = strtok((char*)data, inlieu);
   while (ptr != NULL)
   {
      words++;
      word.push_back(ptr);
      ptr = strtok(NULL, inlieu);
   } 

//   Serial.printf("<%s> <%s> <%s>\n", word[0].c_str(), word[1].c_str(), word[2].c_str());
   return words;
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
               Serial.println(" Calling ProcessCheckbox");
               Buttons.ProcessCheckbox(word[1], word[2]);
            }
            else
            {
               Serial.println("ignored");
            }
         }
         else if (word[0] == "isheader")
         {
            // Handle an "isheader" message.
            //
            Serial.printf("word[0]: %s,  word[1]: %s,  word[2]: %s\n", word[0].c_str(), word[1].c_str(), word[2].c_str());
            Data.MatchAndUpdateHeaders(word[1], word[2]);

            // Copy and update to the client.
            //
            NotifyClients(Data.GetCurrentEpromHeaderValues());
         }
         else if ((word[0] == "isnumber") || (word[0] == "istext"))
         {
            // Handle an "isnumber" or "istext" message
            //
            Serial.printf("word[0]: %s,  word[1]: %s,  word[2]: %s\n", word[0].c_str(), word[1].c_str(), word[2].c_str());
            Data.MatchAndUpdateData(word[1], word[2]);

            // Copy and update to the client.
            //
            NotifyClients(Data.GetCurrentEpromDataValues());
         }
         else if (word[0] == "button")
         {
            // Handle a "button" message
            //
            if (word.size() > 1)
            {
               Serial.printf("word[0]: <%s>,  word[1]: <%s>,  word[2]: <%s>\n", word[0].c_str(), word[1].c_str(), word[2].c_str());
               Serial.println(" Calling ProcessButton");
               Buttons.ProcessButton(word[1], word[2]);
            }
            else
            {
               Serial.println("ignored");
            }

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
         HandleWebSocketMessage(arg, data, len);
         break;
      case WS_EVT_PONG:
      case WS_EVT_ERROR:
         break;
  }
}

//-----------------------------------------------------------------------------
//
void initWebSocket(void) 
{
   ws.onEvent(onEvent);
   server.addHandler(&ws);
}

//-----------------------------------------------------------------------------
//
void setup() 
{
   String line;
   File fd;
   char text[100];
   DisplayData test;

   // Serial port for debugging purposes
   Serial.begin(115200);
   Serial.println("\n\n======== Startup");

   // Setup Qwiic pins.
   Wire.setPins(SDA_PIN, SCL_PIN);

   pinMode(PWR_ENABLE, OUTPUT);
   digitalWrite(PWR_ENABLE, HIGH);

   pinMode(LED_PORT, OUTPUT);
   digitalWrite(LED_PORT, HIGH);
   
   pinMode(BRAKE_PORT, OUTPUT);
   digitalWrite(BRAKE_PORT, LOW);

   xSDCardAccessMutex = xSemaphoreCreateMutex();
   if (xSDCardAccessMutex == NULL)
   {
      Serial.println("Could not create xSDCardAccessMutex");
      return;
   }

   //------------------------------------------------------
   // Initialise the SPI interface for the stepper.
   //
   pinMode(STEPPER_CS, OUTPUT);
   digitalWrite(STEPPER_CS, HIGH);
   pinMode(STEPPER_RESET, OUTPUT);
   digitalWrite(STEPPER_RESET, HIGH);
   theSPI.begin(STEPPER_SCK, STEPPER_CIPO, STEPPER_COPI, STEPPER_CS);

   // Start up the stepper controller.
   //
   delay(100);
   MotorControl.Init();
   Motor.Configure();
   Motor.SetParam(ALARM_EN, 0x07);
   delay(100);
   Serial.printf("Motor status: %04X\n", Motor.GetMotorStatus());
   Motor.SetStepMode(4);
   Motor.SetAccelRate(300);
   Motor.SetDecelRate(1000);

#ifdef SIMULATING
   WellSimulator.Init(1);
#endif
   LedScreen.Init();

   Serial.println("Starting WiFi");
   if (!InitWiFi())
   {
      Serial.println("Cannot access WiFi");
      return;
   }

   // Init the TMP116 temperature sensor.
   //
#ifdef LIMAX_CONTROLLER   
   tmp116.begin();
#endif   

   // Configure the SDCard memory. Verify the directory
   // structure is correct.
   //
   Serial.println("Starting SDCard");
   if (SDCard.Begin() != CARD_SUCCESS)
   {
      LedScreen.WriteLine2("SDCard FAIL");
      Serial.println("Card Init failed");
      return;
   }
   if (!SDCard.VerifyDataFolder(DATA_FOLDER) == CARD_SUCCESS)
   {
      LedScreen.WriteLine2("Bad Folders");
      return;
   }
   LedScreen.WriteLine2("SDcard OK");

   // Zero the line count of the dynamic temporary data file.
   //
   // SDCard.VerifyDataFolder(DISPLAY_DATA_FOLDER);
   // SDCard.WriteFile(TEMP_DATA_FILENAME, "1\n"); 
   // test.value[0] = 0;
   // test.value[1] = 0;
   // test.value[2] = 0;
   // test.value[3] = 0;
   // test.value[4] = 0;
   // sprintf(text, "%f.4,%f.4,%f.4,%f.4,", test.value[0], test.value[1], test.value[2], test.value[3], test.value[4]);
   // SDCard.AppendFile(TEMP_DATA_FILENAME, text); 
   // test.value[0] = 12.9;
   // test.value[1] = 2.6;
   // test.value[2] = 3.7;
   // test.value[3] = 4.8;
   // sprintf(text, "%f.4,%f.4,%f.4,%f.4,", test.value[0], test.value[1], test.value[2], test.value[2]);
   // SDCard.AppendFile(TEMP_DATA_FILENAME, text); 
   // // SDCard.AppendFile(TEMP_DATA_FILENAME, "Second line\n"); 
   // fd.close();

   // fd = SDCard.OpenFile(TEMP_DATA_FILENAME);
   // line = SDCard.ReadFileUntil(fd, '\n');
   // Serial.printf("Line entries %d\n", line.toInt());
   // for (int i=0; i<line.toInt(); i++)
   // {
   //    String myline;
   //    for (int j=0; j<4; j++)
   //    {
   //       myline = SDCard.ReadFileUntil(fd, ',');
   //       test.value[j] = myline.toFloat();
   //    }
   //    Serial.printf("\n<%f %f %f %f>\n", test.value[0], test.value[1], test.value[2], test.value[3]);
   // }
   // fd.close();

   SDCard.ListDir("/", 2);

   // Configure the LittleFS eeprom memory.
   //
   Serial.println("Starting EEprom");
   if (!LittleFS.begin())
   {
      LedScreen.WriteLine3("EEprom FAIL");
      Serial.println("LittleFS mount error");
      return;
   }
   LedScreen.WriteLine3("EEprom OK");

//   LedScreen.WriteLine2("EEprom OK");
//   Eprom.ListDir("/", 2);
//   FileSystem.ListDir(LittleFS, "/", 1);

   // Configure the LittleFS paths for all data inputs and the relevant 
   // buttons. Must be done before either are accessed.
   //
   Data.RestoreSavedValues();
   Buttons.RestoreSavedStates();

   // Serial.println("Starting WiFi");
   // if (!InitWiFi())
   // {
   //    Serial.println("Cannot access WiFi");
   //    return;
   // }
   initWebSocket();

   ws.onEvent(onEvent);
   server.addHandler(&ws);

   server.serveStatic("/", LittleFS, "/");

   //--------------------------------------------------------------------------
   // Web Server home URL.
   // Render root / page
   server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
   {
      Serial.println("Sending /home.html");
      Buttons.EnsureNonSavedIOButtonsOff();   // ensure button states
      request->send(LittleFS, "/home.html", "text/html", false, Buttons.HomeProcessor);
   });

   //--------------------------------------------------------------------------
   // Web Server setup URL.
   //
   server.on("/setup", HTTP_GET, [](AsyncWebServerRequest *request)
   {
      Serial.println("Sending /setup.html");
      Buttons.EnsureNonSavedIOButtonsOff();   // ensure button states
      request->send(LittleFS, "/setup.html", "text/html", false);
   });

   //--------------------------------------------------------------------------
   // Web Server setup URL.
   //
   server.on("/basicsine", HTTP_GET, [](AsyncWebServerRequest *request)
   {
      Serial.println("Sending /basicsine.html");
      Buttons.EnsureNonSavedIOButtonsOff();   // ensure button states
      request->send(LittleFS, "/basicsine.html", "text/html", false, Buttons.BasicSineProcessor);
   });

   //--------------------------------------------------------------------------
   // Web Server setup URL.
   //
   server.on("/sweep", HTTP_GET, [](AsyncWebServerRequest *request)
   {
      String jsonString;

      Serial.println("Sending /sweep.html");

      // // Ensure the profile button is off.
      // //
      // jsonString = "0";
      // Buttons.SetButtonState(B_SINE_QUICKLOOK, jsonString);
      // NotifyClients(Buttons.GetButtonStates());

      Buttons.EnsureNonSavedIOButtonsOff();   // ensure button states
      request->send(LittleFS, "/sweep.html", "text/html", false, Buttons.SineSweepProcessor);
   });

   //--------------------------------------------------------------------------
   // Web Server plot URL
   server.on("/sine-quicklook", HTTP_GET, [](AsyncWebServerRequest *request)
   {
      Serial.println("Sending /sine-quicklook.html");
      request->send(LittleFS, "/sine-quicklook.html", "text/html");
   });

   //--------------------------------------------------------------------------
   // Web Server plot URL
   server.on("/sweep-quicklook", HTTP_GET, [](AsyncWebServerRequest *request)
   {
      Serial.println("Sending /sweep-quicklook.html");
       request->send(LittleFS, "/sweep-quicklook.html", "text/html");
   });

   //--------------------------------------------------------------------------
   // Web Server sine3 URL
   server.on("/plot4", HTTP_GET, [](AsyncWebServerRequest *request)
   {
      Serial.println("Sending /plot4.html");
      Flag.RePlot = true;
      // Flag.OnSetup = false;
      // AbortPlot();
      Buttons.EnsureNonSavedIOButtonsOff();   // ensure button states
      // Flag.Plot = false;
      // Flag.DoSine3 = true;
      // Serial.println(" -Plot false");
      request->send(LittleFS, "/plot4.html", "text/html");
   });

   //--------------------------------------------------------------------------
   // Web Server setup URL.
   //
   server.on("/equipment", HTTP_GET, [](AsyncWebServerRequest *request)
   {
      Serial.println("Sending /equipment.html");
      Buttons.EnsureNonSavedIOButtonsOff();   // ensure button states
      request->send(LittleFS, "/equipment.html", "text/html", false, Buttons.EquipmentProcessor);
   });

   //--------------------------------------------------------------------------
   // Web Server setup URL.
   //
   server.on("/datafile", HTTP_GET, [](AsyncWebServerRequest *request)
   {
      Serial.println("Sending /datafile.html");
      Buttons.EnsureNonSavedIOButtonsOff();   // ensure button states
      request->send(LittleFS, "/datafile.html", "text/html");
   });

   //--------------------------------------------------------------------------
   //
   server.on("/download-data", HTTP_GET, [](AsyncWebServerRequest *request)
   {
      Serial.println("Sending /download-data");
      if (xSemaphoreTake(xSDCardAccessMutex, (TickType_t)SDCARD_ACCESS_MUTEX) == pdTRUE)
      {
         request->send(SD_MMC, DataFilename.c_str(), String(), true);

         // return the mutex.
         //
         xSemaphoreGive(xSDCardAccessMutex);
         Serial.println("Sent");
      }
      else
      {
         Serial.println("COULD NOT ACCESS MUTEX IN TIME");
      }
   });

   //--------------------------------------------------------------------------
   //
   server.on("/view-data", HTTP_GET, [](AsyncWebServerRequest *request)
   {
      Serial.println("Sending view-data");
      if (xSemaphoreTake(xSDCardAccessMutex, (TickType_t)SDCARD_ACCESS_MUTEX) == pdTRUE)
      {
         request->send(SD_MMC, DataFilename.c_str(), "text/txt", false);
         
         // return the mutex.
         //
         xSemaphoreGive(xSDCardAccessMutex);
         Serial.println("Sent");
      }
      else
      {
         Serial.println("COULD NOT ACCESS MUTEX IN TIME");
      }
   });

   //--------------------------------------------------------------------------
   //
   server.on("/delete-data", HTTP_GET, [](AsyncWebServerRequest *request)
   {
      if (xSemaphoreTake(xSDCardAccessMutex, (TickType_t)SDCARD_ACCESS_MUTEX) == pdTRUE)
      {
         SDCard.DeleteFile(DataFilename);
         request->send(200, "text/plain", "File was deleted.");
         
         // return the mutex.
         //
         xSemaphoreGive(xSDCardAccessMutex);
         Serial.println("Deleted");
      }
      else
      {
         Serial.println("COULD NOT ACCESS MUTEX IN TIME");
      }
   });

   //--------------------------------------------------------------------------
   //
   server.on("/values", HTTP_GET, [](AsyncWebServerRequest *request)
   {
      JSONVar values;

      Serial.println("/values");

      String json = Data.GetCurrentEpromDataValues();
//      Serial.println(json);
      request->send(200, "application/json", json);
      Flag.RePlot = true;
   });

   //--------------------------------------------------------------------------
   //
   server.on("/headers", HTTP_GET, [](AsyncWebServerRequest *request)
   {
      JSONVar values;

      Serial.println("/headers");

      String json = Data.GetCurrentEpromHeaderValues();
//      Serial.println(json);
      request->send(200, "application/json", json);
   });

   //--------------------------------------------------------------------------
   // Request for the latest sensor readings
   server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request)
   {
      JSONVar jReadings;
      String json;
      char text[20];

      Serial.println("Request for /readings.html");
//      sprintf(text, "%d", Temp.entries);
      jReadings["plotEntries"] = text;
      jReadings["dataset0"] = ManageSamples.GetCurrentPlotValuesForClient(&SampleSet, 0);
      for (int i=1; i<Temp.entries; i++)
      {
//         sprintf(text, "dataset%d", i);
         jReadings[(const char *)&text] = ManageSamples.GetCurrentPlotValuesForClient(&SampleSet, i);
      }
      json = JSON.stringify(jReadings);

      request->send(200, "application/json", json);  // send to client
   });

   //--------------------------------------------------------------------------
   //
   server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) 
   {
      const AsyncWebParameter* p;

      Serial.println("/POST");

      for (int i=0; i<request->params(); i++)     
      {
         p = request->getParam(i);

         if (p->isPost())
         {
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            Data.UpdateDatabase(p);
         }
      }  
   
      Flag.OnSetup = true;
      request->send(LittleFS, "/setup.html", "text/html");
   });
   
   //--------------------------------------------------------------------------
   // GET request to <ESP_IP>/updates?output=<output>&state=<state>
   //
   server.on("/updates", HTTP_GET, [] (AsyncWebServerRequest *request) 
   {
      String id;
      String state;

      Serial.println("/updates");
      for (int i=0; i<request->params(); i++)
      {
         const AsyncWebParameter* p = request->getParam(i);
//         Serial.printf("   %s %s\n", p->name().c_str(), p->value().c_str());
      }

      // GET input value on <ESP_IP>/updates?output=<output>&state=<state>.
      //
      if (request->hasParam(PARAM_INPUT_ID) && request->hasParam(PARAM_INPUT_STATE)) 
      {
         int i;

         id = request->getParam(PARAM_INPUT_ID)->value();
         i = id.toInt() - 1;
         state = request->getParam(PARAM_INPUT_STATE)->value();
//         Serial.printf("index=%d id=%s state=%s\n", i, id.c_str(), state.c_str());

         request->send(200, "text/plain", "OK");
      }
   });

   //--------------------------------------------------------------------------
   events.onConnect([](AsyncEventSourceClient *client)
   {
      String json;

      Serial.println(client->lastId());
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());

      // Send event with message "hello!", id current millis
      // and set reconnect delay to 1 second

      json = Buttons.GetButtonTextAndColors();
      client->send(json.c_str(), NULL, 0, 1000);
   });


   //-------------------------------------------------------------------------
   //
   ElegantOTA.begin(&server);
   server.addHandler(&events);
   server.begin();

   // Must create the xStepQueue before we write to it.
   if (Network.Init(Channel))
   {
      Timers.Init();

      Motor.SetStepMode(0);
   }

   LedScreen.WriteLine4("LIMAX OK");
}

uint16_t status;

//-----------------------------------------------------------------------------
//
void loop() 
{
   ElegantOTA.loop();

      // // Flash the LED at 1 sec intervals, it is on for 1 sec and off for 1 sec.
      // //
      // if (LedState)
      // {
      //    digitalWrite(LED_PORT, LOW);
      //    LedState = false;
      // }
      // else
      // {
      //    digitalWrite(LED_PORT, HIGH);
      //    LedState = true;
      // }

//    if (xSemaphoreTake(xSDCardAccessMutex, 5) == pdTRUE)
//    {
//       digitalWrite(LED_PORT, HIGH);
//       status = (Motor.GetMotorStatus() & 0x0060) >> 5;
//       xSemaphoreGive(xSDCardAccessMutex);
//       digitalWrite(LED_PORT, LOW);
//    }
//    if (status == 3)
//    {
// //      digitalWrite(LED_PORT, LOW);
//    }
//    else
//    {
// //      digitalWrite(LED_PORT, HIGH);
//    }


   if (Flag.DoSample)
   {
      // Run the network/sensor data transfer. The DoSample flag is set by 
      // the interval timer and the network requests samples from the 
      // connected wells. The CNetwork will post the xNewSampleQueue queue
      // containing the sensor result data on sample completion. At that 
      // point the samples are sent to the Websocket client and also for
      // SDCard storage.
      //
      Flag.DoSample = false;
      
      DummyTime = DummyTime + 1;

      Serial.println("\nStart sample.");
      Network.StartSample();
   }

   if (Flag.UpdateIntervalTimer)
   {
      Flag.UpdateIntervalTimer = false;

      Timers.StartIntervalTimer(DO_SAMPLE, Data.GetDataEntryNumericValue(DB_SAMPLE_INTERVAL) * 1000);
   }

//   float v = tmp116.readTemperature();
//   Serial.println(v);
}

