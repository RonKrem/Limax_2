// FileSystem.h
//
#include "Main.h"

#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H


typedef enum
{
    CARD_SUCCESS,
    FAILED_OPEN_FOLDER,
    NOT_A_FOLDER,
    MKDIR_FAILED,
    RMDIR_FAILED,
    CANNOT_OPEN_FILE,
    CANNOT_OPEN_FOR_WRITING,
    WRITE_FAILED,
    CANNOT_OPEN_FOR_APPENDING,
    APPEND_FAILED,
    RENAME_FAILED,
    DELETE_FAILED,
    FILE_DOES_NOT_EXIST,
    CARD_MOUNT_FAILED,
    NO_CARD_EXISTS,
    FILE_OPEN_FAILED,
} FileStateType;


class CFileSystem
{
public:
   CFileSystem(void);

   void ListDir(fs::FS &fs, const char *dirname, uint8_t levels) ;

   FileStateType CreateDir(fs::FS &fs, const char *path);

   FileStateType FileExists(fs::FS &fs, const char *path);

   File OpenFile(fs::FS &fs, const char* path);

   String ReadFile(fs::FS &fs, const char* path);

   String ReadFileUntil(File file, char terminator);

   FileStateType WriteFile(fs::FS &fs, const char* path, const char* message);

   FileStateType AppendFile(fs::FS &fs, const char *path, const char *message);

   FileStateType DeleteFile(fs::FS &fs, const char *path);

private:
};


#endif   // _FILESYSTEM_H
