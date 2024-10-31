// Buttons.h
//
#ifndef  _BUTTONS_H
#define  _BUTTONS_H

#include "Main.h"


#define  STATE_IS_SAVED       0x01
#define  IS_GPIO_PORT         0x02


//-----------------------------------------------------------------------------
// Button structure definitions.
//
typedef struct 
{
   String id;           // html id
   String checked;      // "1" or "0"
   String onText;       // text shown to turn it on
   String offText;      // text shown to turn it off
   uint8_t states;      // 0x01 - state is saved if true
                        // 0x02 - true if is GPIO port
} ButtonDetails;

// Indexes to the Button array:
enum
{
   BUTTON_INVALID,            //  0 (invalid)
   BUTTON_GO_UP,              //  1 (store - json)
   BUTTON_GO_DOWN,            //  2
   BUTTON_DIRECTION,          //  3
   BUTTON_BRAKE_OVERIDE,     //  4
   BUTTON_DYNAMIC_DEPTH,      //  5
   BUTTON_DATA_VIEW,          //  6
   BUTTON_DATA_DOWNLOAD,      //  7
   BUTTON_DATA_DELETE,        //  8
   BUTTON_SINE2_ENABLE,       //  9
   BUTTON_SINE3_ENABLE,       // 10
   BUTTON_TEST_CONTROL,       // 11
   BUTTON_SINE_QUICKLOOK,     // 12
   BUTTON_SWEEP_CONTROL,      // 13
   BUTTON_SWEEP_QUICKLOOK,    // 14
   BUTTON_LIN_EXP,            // 15
   BUTTON_SHOW_PLOTS,         // 16
};


class CButtons
{
public:
   CButtons(void);
   
   String GetButtonStates(void);

   void RestoreSavedStates(void);

   void ProcessButtons(String id, String checked);

   void ActionButton(uint16_t index, String checked, String state);

   boolean GetButtonState(uint32_t index);
   
   boolean SetButtonState(String state, const uint32_t index);

   String CreateDirectoryName(uint32_t index);

private:

   void ManageMotorUp(uint32_t index, String state);

   void ManageMotorDown(uint32_t index, String state);

};


#endif   // _BUTTONS_H