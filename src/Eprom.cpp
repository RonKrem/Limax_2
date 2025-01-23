// Eprom.cpp
//
#include "Eprom.h"
#include "Data.h"


extern CFileSystem FileSystem;
extern CData Data;


//-------------------------------------------------------------------
//
CEprom::CEprom(void)
{
}

//-----------------------------------------------------------------------------
//
int CEprom::Begin()
{
   return LittleFS.begin();
}

//-----------------------------------------------------------------------------
//
void CEprom::ListDir(const char *dirname, uint8_t levels) 
{
   Serial.printf("Listing LittleFS directory: %s\r\n", dirname);
   FileSystem.ListDir(LittleFS, dirname, levels);
   Serial.println("  ---------------");
}

// //-----------------------------------------------------------------------------
// // Initialise the SPIFFS variable database.
// //
// void CEprom::PrepareEprom(void)
// {
//    int i;
//    String entry;

//    Serial.println("PrepareEprom");

//    // Read the input values.
//    //
//    for (i = 0; i < DB_Entries; i++)
//    {
//       entry = ReadFile(SPIFFS, DB[i].PathName);
// //      Serial.println(entry);
//       if (entry == "NULL")
//       {
//          // If no entry, write the default values.
//          //
//          WriteFile(SPIFFS, DB[i].PathName, DB[i].HtmlValue.c_str());
//       }
//    }

//    // Write the button details.
//    //
//    for (i=1; i<NumButtons; i++)
//    {
//       entry = ReadFile(SPIFFS, Button[i].path);
//       if (entry == "NULL")
//       {
//          WriteFile(SPIFFS, Button[i].path, Button[i].state.c_str());
//       }
//       else
//       {
//          Button[i].state = entry;
//       }
//    }
// }

//-------------------------------------------------------------------
//
FileStateType CEprom::WriteFile(const String path, const String message) 
{
   //Serial.println(message);
   return FileSystem.WriteFile(LittleFS, path.c_str(), message.c_str());
}

//-------------------------------------------------------------------
//
String CEprom::ReadFile(const String path) 
{
//   Serial.printf("Reading EEProm file %s\n", path.c_str());
   return FileSystem.ReadFile(LittleFS, path.c_str());
}

//-------------------------------------------------------------------
//
FileStateType CEprom::DeleteFile(const String path)
{
   return FileSystem.DeleteFile(LittleFS, path.c_str());
}
