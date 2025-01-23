// Eprom.h
//
#ifndef  _EPROM_H
#define  _EPROM_H

#include "FileSystem.h"


class CEprom : public CFileSystem
{
public:
   CEprom(void);

   int Begin();

   void ListDir(const char *dirname, uint8_t levels);

   String ReadFile(const String path);

   FileStateType WriteFile(const String path, const String message);

   FileStateType DeleteFile(const String path);

   void PrepareEprom(void);

private:   
};



#endif      // _EPROM_H