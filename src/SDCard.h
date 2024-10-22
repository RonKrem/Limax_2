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

   FileStateType WriteFile(const char *path, const char *message) ;

private:   
};



#endif      // _CARD_H