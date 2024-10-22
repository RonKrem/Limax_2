// Ledscreen.h
//
// Manages a screen full of text, allowing a user to update any line.
//
#ifndef LEDSCREEN_H
#define LEDSCREEN_H
#include "Main.h"

#define CHARACTERS      15
#define SCREENLINES     4

// typedef struct
// {
//     char    t[CHARACTERS + 1];
// } LineType;

// typedef struct
// {
//     LineType    Line[SCREENLINES];
// } PageType;



class CLedScreen
{
public:
    CLedScreen(void);

    void Init(void);

    void DisplayPage(void);

    void WriteLine1(String line);
    void WriteLine2(String line);
    void WriteLine3(String line);
    void WriteLine4(String line);

private:
    void CopyLine(uint16_t line, String text);

private:
    String    mPage[SCREENLINES];
};





















#endif  // LEDSCREEN_H
