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

   Serial.printf("Examining directory: %s\n", dirname);
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
         Serial.print("  DIR : ");

         Serial.print(file.path());
         time_t t = file.getLastWrite();
         struct tm *tmstruct = localtime(&t);
         Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", 
                        (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, 
                        tmstruct->tm_mday, tmstruct->tm_hour,
                        tmstruct->tm_min, tmstruct->tm_sec
                     );

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

         Serial.print(file.size());
         time_t t = file.getLastWrite();
         struct tm *tmstruct = localtime(&t);
         Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", 
                        (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, 
                        tmstruct->tm_mday, tmstruct->tm_hour,
                        tmstruct->tm_min, tmstruct->tm_sec
                        );
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
      Serial.println(Serial.println("Dir created"));
   } 
   else 
   {
      Serial.println(Serial.println("mkdir failed"));
      return MKDIR_FAILED;
   }

   return CARD_SUCCESS;
}

//-------------------------------------------------------------------
//
FileStateType CFileSystem::FileExists(fs::FS &fs, const char *path)
{
   File file = fs.open(path);
   if (!file) 
   {
      Serial.println(Serial.printf("Failed to open file %s for reading\n", path));
      return FILE_DOES_NOT_EXIST;
   }

   return CARD_SUCCESS;
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
