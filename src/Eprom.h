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

   FileStateType WriteFile(const char *path, const char *message) ;

   void PrepareEprom(void);

private:   
};



#endif      // _EPROM_H