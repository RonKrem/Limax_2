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
} FileStateType;


class CFileSystem
{
public:
   CFileSystem(void);

   void ListDir(fs::FS &fs, const char *dirname, uint8_t levels) ;

   FileStateType CreateDir(fs::FS &fs, const char *path);

   FileStateType FileExists(fs::FS &fs, const char *path);

   FileStateType WriteFile(fs::FS &fs, const char *path, const char *message);

private:
};


#endif   // _FILESYSTEM_H
