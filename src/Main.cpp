#include "Main.h"
#include "Buttons.h"
#include "DataInputs.h"
#include "Eprom.h"
#include "SDCard.h"
//#include "Motor.h"
//#include "MotorControl.h"


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
JSONVar readings;
CButtons Buttons;
CDataInputs DataInputs;
//CMotor Motor;
//CMotorControl MotorControl;
CFileSystem FileSystem;
CEprom Eprom;
CSDCard SDCard;

String DataFilename;    // contains the current data file path
char OledText[40];


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
//
String Processor(const String& var)
{
   Serial.printf("Processor: var= ", var);

   return String();
}



float turn;

//-----------------------------------------------------------------------------
//
void GetTurnInCms(void)
{
   DataEntry entry;

   entry = DataInputs.GetEntry(0);
   Serial.printf("%s %s %s %d\n", entry.InputParameter.c_str(), entry.HtmlValue.c_str(), entry.Update);
   entry = DataInputs.GetEntry(100);
   Serial.printf("%s %s %s %d\n", entry.InputParameter.c_str(), entry.HtmlValue, entry.Update);
   turn = PI * DataInputs.GetEntry(DB_DRUM_DIA).HtmlValue.toFloat() / 10.0 * DataInputs.GetEntry(DB_DRIVE_RATIO).HtmlValue.toFloat();
   Serial.printf("OnedrumTurnInCMS %f\n", turn);

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
   Eprom.ListDir("/", 0);

   // Configure the SDCard memory. Verify the directory
   // structure is correct.
   //
   if (SDCard.Begin() != CARD_SUCCESS)
   {
      Serial.println("Card Init failed");
      while (1);
   }
   SDCard.VerifyDataFolder(DATA_FOLDER);
   SDCard.ListDir("/", 1);

   LedScreen.Init();
   LedScreen.WriteLine1("Limax Website");

   // Configure the LittleFS paths for all data inputs and
   // the relevant buttons. Must be done before either are
   // accessed.
   //
//   Buttons.SetButtonPaths();
//   DataInputs.SetInputPaths();

   DataInputs.PrepareEprom();

//   GetTurnInCms();

   
//    if (!InitWiFi())
//    {
//       Serial.println("Cannot access WiFi");
//       while (1);
//    }
// //   ws.onEvent(onEvent);
//    server.addHandler(&ws);

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

