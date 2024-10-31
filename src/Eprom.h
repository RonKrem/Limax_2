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

   void ListDir(const char *dirname, uint8_t levels) ;

   String ReadFile(String path) ;

   FileStateType WriteFile(String path, String message) ;

   void PrepareEprom(void);

private:   
};



#endif      // _EPROM_H