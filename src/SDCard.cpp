// Card.cpp
//
#include "SDCard.h"

// Default pins for Sparkfun ESP32 Datalogger_IOT
//
#define  SDCARD_CLK           14
#define  SDCARD_CMD           15
#define  SDCARD_D0             2
#define  SDCARD_D1             4
#define  SDCARD_D2            12
#define  SDCARD_D3            13


extern CFileSystem FileSystem;


//-------------------------------------------------------------------
//
CSDCard::CSDCard(void)
{
}

//-----------------------------------------------------------------------------
//
FileStateType CSDCard::Begin()
{
   uint8_t cardType;
   uint64_t cardSize;

   Serial.println("Setting SDCard pins");
   SD_MMC.setPins(SDCARD_CLK, SDCARD_CMD, SDCARD_D0, SDCARD_D1, SDCARD_D2, SDCARD_D3);

   Serial.println("Calling SD_MMC.begin()");
   if (!SD_MMC.begin("/sdcard", false)) 
   {
      Serial.println("Card Mount Failed");
      return CARD_MOUNT_FAILED;
   }
   Serial.println("Card mount success");
   
   cardType = SD_MMC.cardType();

   if (cardType == CARD_NONE) 
   {
      Serial.println("No SDCard attached");
      return NO_CARD_EXISTS;
   }

   Serial.printf("SDCard Type: ");
   if (cardType == CARD_MMC) 
   {
      Serial.println("MMC");
   } 
   else 
   if (cardType == CARD_SD) 
   {
      Serial.println("SDSC");
   } 
   else 
   if (cardType == CARD_SDHC) 
   {
      Serial.println("SDHC");
   } 
   else 
   {
      Serial.println("UNKNOWN");
   }

   cardSize = SD_MMC.cardSize() / (1024 * 1024);
   Serial.printf("SDCard Size: %llu MB\n", cardSize);
   Serial.println("-----------------");

   return CARD_SUCCESS;
}

//-------------------------------------------------------------------
//
FileStateType CSDCard::VerifyDataFolder(const char *path)
{
   // First check the Limax directory exiss.
   //
   if (FileExists(path) == CARD_SUCCESS)
   {
      Serial.printf("%s exists.\n", path);
      return CARD_SUCCESS;
   }
   else
   {
      Serial.printf("Creating %s\n", path);
      if (CreateDir(path) != CARD_SUCCESS)
      {
         Serial.printf(" Cannot create %s\n", path);
         return FAILED_OPEN_FOLDER;
      }
      else
      {
         Serial.printf(" %s created\n", path);
      }
   }

   return CARD_SUCCESS;
}

//-----------------------------------------------------------------------------
//
void CSDCard::ListDir(const char *dirname, uint8_t levels) 
{
   Serial.printf("Listing SDCard directory: %s\r\n", dirname);
   FileSystem.ListDir(SD_MMC, dirname, levels);
   Serial.println("  ---------------");
}

//-----------------------------------------------------------------------------
//
FileStateType CSDCard::CreateDir(const char *path) 
{
   Serial.println("CreateDir");
   return FileSystem.CreateDir(SD_MMC, path);
}

//-------------------------------------------------------------------
//
FileStateType CSDCard::FileExists(const char *path)
{
   File file = SD_MMC.open(path);
   if (!file) 
   {
      Serial.printf("Failed to open file %s for reading\n", path);
      return FILE_DOES_NOT_EXIST;
   }

   return CARD_SUCCESS;
}

//-------------------------------------------------------------------
//
FileStateType CSDCard::WriteFile(const String path, const String message) 
{
   return FileSystem.WriteFile(SD_MMC, path.c_str(), message.c_str());
}

//-------------------------------------------------------------------
//
FileStateType CSDCard::AppendFile(const String path, const String message) 
{
   return FileSystem.AppendFile(SD_MMC, path.c_str(), message.c_str());
}

//-------------------------------------------------------------------
//
File CSDCard::OpenFile(String path)
{
   return FileSystem.OpenFile(SD_MMC, path.c_str());

}

//-------------------------------------------------------------------
//
String CSDCard::ReadFile(const String path) 
{
   return FileSystem.ReadFile(SD_MMC, path.c_str());
}

//-------------------------------------------------------------------
//
String CSDCard::ReadFileUntil(File fd, char terminator) 
{
   return FileSystem.ReadFileUntil(fd, terminator);
}

//-------------------------------------------------------------------
//
FileStateType CSDCard::DeleteFile(const String path) 
{
   return FileSystem.DeleteFile(SD_MMC, path.c_str());
}
