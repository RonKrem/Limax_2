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

   SD_MMC.setPins(SDCARD_CLK, SDCARD_CMD, SDCARD_D0, SDCARD_D1, SDCARD_D2, SDCARD_D3);
   if (!SD_MMC.begin("/sdcard", false)) 
   {
      Serial.println(Serial.println("Card Mount Failed"));
      return CARD_MOUNT_FAILED;
   }

   cardType = SD_MMC.cardType();

   if (cardType == CARD_NONE) 
   {
      Serial.println(Serial.println("No SD_MMC card attached"));
      return NO_CARD_EXISTS;
   }

   Serial.println("SD_MMC Card Type: ");
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
   Serial.printf("SD_MMC Card Size: %llu MB\n", cardSize);
   Serial.println("-----------------");

   return CARD_SUCCESS;
}

//-------------------------------------------------------------------
//
FileStateType CSDCard::VerifyDataFolder(const char *path)
{
   // First check the Limax directory exiss.
   //
   if (FileExists(DATA_FOLDER) == CARD_SUCCESS)
   {
      Serial.printf("Folder %s exists.\n", DATA_FOLDER);
      return CARD_SUCCESS;
   }
   else
   {
      Serial.printf("Creating data folder %s\n", DATA_FOLDER);
      if (CreateDir(DATA_FOLDER) != CARD_SUCCESS)
      {
         Serial.println("Cannot create data folder.");
         return FAILED_OPEN_FOLDER;
      }
      else
      {
         Serial.println("Data folder created.");
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
