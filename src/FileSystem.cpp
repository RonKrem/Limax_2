// FileSystem.cpp
//
#include "FileSystem.h"


//-------------------------------------------------------------------
//
CFileSystem::CFileSystem(void)
{

}

//-----------------------------------------------------------------------------
//
void CFileSystem::ListDir(fs::FS &fs, const char *dirname, uint8_t levels) 
{
   File root = fs.open(dirname);

//   Serial.printf("Examining directory: %s\n", dirname);
   if (!root) 
   {
      Serial.println("- failed to open directory");
      return;
   }
   if (!root.isDirectory()) 
   {
      Serial.println(" - not a directory");
      return;
   }

   File file = root.openNextFile();
   if (!file)
   {
      Serial.println("  Directory is empty");
   }
   while (file) 
   {
      if (file.isDirectory()) 
      {
         Serial.print(" DIR : ");
         Serial.println(file.path());
         // time_t t = file.getLastWrite();
         // struct tm *tmstruct = localtime(&t);
         // Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", 
         //                (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, 
         //                tmstruct->tm_mday, tmstruct->tm_hour,
         //                tmstruct->tm_min, tmstruct->tm_sec
         //             );

         if (levels) 
         {
            ListDir(fs, file.path(), levels - 1);
         }
      } 
      else 
      {
         Serial.print("  FILE: ");
         Serial.print(file.name());
         Serial.print("  SIZE: ");
         Serial.println(file.size());
         // time_t t = file.getLastWrite();
         // struct tm *tmstruct = localtime(&t);
         // Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", 
         //                (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, 
         //                tmstruct->tm_mday, tmstruct->tm_hour,
         //                tmstruct->tm_min, tmstruct->tm_sec
         //                );
      }

      file = root.openNextFile();
   }
}

//-------------------------------------------------------------------
//
FileStateType CFileSystem::CreateDir(fs::FS &fs, const char *path) 
{
   Serial.printf("Creating Dir: %s\n", path);
   if (fs.mkdir(path)) 
   {
      Serial.println("Dir created");
   } 
   else 
   {
      Serial.println("mkdir failed");
      return MKDIR_FAILED;
   }

   return CARD_SUCCESS;
}

//-------------------------------------------------------------------
//
FileStateType CFileSystem::FileExists(fs::FS &fs, const char *path)
{
   Serial.printf("Opening file: %s\n", path);
   File file = fs.open(path);
   if (!file) 
   {
      Serial.printf("Failed to open file %s for reading\n", path);
      return FILE_DOES_NOT_EXIST;
   }

   return CARD_SUCCESS;
}

//-----------------------------------------------------------------------------
// Read an entry from SPIFFS
//
String CFileSystem::ReadFile(fs::FS &fs, const char* path)
{
   File file;
   String fileContent;

   file = fs.open(path);
   if (!file || file.isDirectory())
   {
      String error = "NULL";
      Serial.printf("LittleFS failed to open file %s for reading\n", path);
      return error;
   }

   while (file.available())
   {
      fileContent = file.readString();
      break;     
   }

//   Serial.printf("Reading file: %s Content: %s\n", path, fileContent.c_str());

   return fileContent;
}

//-----------------------------------------------------------------------------
// Read an entry from SPIFFS
//
File CFileSystem::OpenFile(fs::FS &fs, const char* path)
{
   File file;

   file = fs.open(path);
   if (!file || file.isDirectory())
   {
      Serial.printf("LittleFS failed to open file %s for reading\n", path);
   }
   return file;
}

//-----------------------------------------------------------------------------
// Read an entry from SPIFFS
//
String CFileSystem::ReadFileUntil(File file, char terminator)
{
   String fileContent;

   if (file.available())
   {
      fileContent = file.readStringUntil(terminator);
   }

//   Serial.printf("Reading file: %s Content: %s\n", fileContent.c_str());

   return fileContent;
}

//-------------------------------------------------------------------
//
FileStateType CFileSystem::WriteFile(fs::FS &fs, const char *path, const char *message) 
{
   Serial.printf("Writing file: %s\n", path);

   File file = fs.open(path, FILE_WRITE);
   if (!file) 
   {
      Serial.println("Failed to open file for writing");
      return CANNOT_OPEN_FOR_WRITING;
   }
   if (file.print(message)) 
   {
      Serial.println("File written");
   } 
   else 
   {
      Serial.println("Write failed");
      return WRITE_FAILED;
   }

   return CARD_SUCCESS;
}

//-------------------------------------------------------------------
//
FileStateType CFileSystem::AppendFile(fs::FS &fs, const char *path, const char *message) 
{
//   Serial.printf("Appending to file: %s\n", path);

   File file = fs.open(path, FILE_APPEND);
   if (!file) 
   {
      Serial.println("Failed to open file for appending");
      return CANNOT_OPEN_FOR_APPENDING;
   }
   if (file.print(message)) 
   {
//      Serial.println("Message appended");
   } 
   else 
   {
      Serial.println("Append failed");
      return APPEND_FAILED;
   }

   return CARD_SUCCESS;
}

//-------------------------------------------------------------------
//
FileStateType CFileSystem::DeleteFile(fs::FS &fs, const char *path) 
{
   Serial.printf("Deleting file: %s\n", path);

   if (fs.remove(path)) 
   {
      Serial.println("File deleted");
   } 
   else 
   {
      Serial.println("Delete failed");
      return DELETE_FAILED;
   }

   return CARD_SUCCESS;
}
