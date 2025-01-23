// LedScreen.cpp
//
#include "LedScreen.h"

extern CLedScreen LedScreen;

QwiicTransparentOLED myOLED;


//-------------------------------------------------------------------
//
CLedScreen::CLedScreen(void)
{
   mPage[0] = "";
   mPage[1] = "";
   mPage[2] = "";
   mPage[3] = "";
}

//-------------------------------------------------------------------
//
void CLedScreen::Init(void)
{
#ifdef LED_SCREEN   
   // Start the OLED display.  
   if (!myOLED.begin(Wire, 0x3d))
   {
      Serial.println("OLED - Device begin failed");
      while(1);
   }
   myOLED.rectangleFill(1, 1, myOLED.getWidth()-1, myOLED.getHeight()-1);
   myOLED.display();
   myOLED.erase();
   myOLED.display();
   myOLED.setFont(&QW_FONT_8X16);
   
//   DisplayPage();
#endif
}

//-------------------------------------------------------------------
//
void CLedScreen::DisplayPage(void)
{
#ifdef LED_SCREEN   
    myOLED.erase();
    myOLED.text(0,  0, mPage[0]);
    myOLED.text(0, 16, mPage[1]);
    myOLED.text(0, 32, mPage[2]);
    myOLED.text(0, 48, mPage[3]);
    myOLED.display();
#endif
}

//-------------------------------------------------------------------
//
void CLedScreen::WriteLine1(String line)
{
    CopyLine(0, line);
    DisplayPage();
}

//-------------------------------------------------------------------
//
void CLedScreen::WriteLine2(String line)
{
    CopyLine(1, line);
    DisplayPage();
}

//-------------------------------------------------------------------
//
void CLedScreen::WriteLine3(String line)
{
    CopyLine(2, line);
    DisplayPage();
}

//-------------------------------------------------------------------
//
void CLedScreen::WriteLine4(String line)
{
    CopyLine(3, line);
    DisplayPage();
}

//-------------------------------------------------------------------
//
void CLedScreen::CopyLine(uint16_t line, String text)
{
    if (line < SCREENLINES)
    {
        mPage[line] = text;
    }
}

