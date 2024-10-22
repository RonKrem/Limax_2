// Eprom.cpp
//
#include "Eprom.h"
#include "DataInputs.h"

#define  SPIFFS               LITTLEFS

extern CFileSystem FileSystem;
extern CDataInputs DataInputs;


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
FileStateType CEprom::WriteFile(const char *path, const char *message) 
{
   return FileSystem.WriteFile(LittleFS, path, message);
}
