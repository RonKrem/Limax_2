// Card.h
//
#ifndef  _CARD_H
#define  _CARD_H

#include "FileSystem.h"


class CSDCard : public CFileSystem
{
public:
   CSDCard(void);

   FileStateType Begin();

   FileStateType VerifyDataFolder(const char *path);

   void ListDir(const char *dirname, uint8_t levels) ;

   FileStateType CreateDir(const char *path);

   FileStateType FileExists(const char *path);

   FileStateType WriteFile(const String path, const String message) ;

   FileStateType AppendFile(const String path, const String message);

   File OpenFile(String path);

   String ReadFile(const String path);

   String ReadFileUntil(File fd, char terminator);

   FileStateType DeleteFile(const String path);

private:   
};



#endif      // _CARD_H