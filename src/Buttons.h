// Buttons.h
//
#ifndef  _BUTTONS_H
#define  _BUTTONS_H

#include "Main.h"


#define  STATE_IS_SAVED       0x01
#define  IS_GPIO_PORT         0x02
#define  IS_PUSHON_PUSHOFF    0x04


//-----------------------------------------------------------------------------
// Button structure definitions.
//
typedef struct 
{
   String id;           // html id
   boolean checked;     // true or false (indicates on "ON" state)
   String onText;       // text shown to turn it on
   String offText;      // text shown to turn it off
   String onColor;
   String offColor;
   uint8_t states;      // 0x01 - state is saved if true
                        // 0x02 - true if is GPIO port
                        // 0x04 = true if push on/push off
} ButtonDetails;

typedef struct
{
   uint8_t        id;
   String         name;
   String         text;
   String         color;
} PushOnPushOff;

// Indexes to the Button array:
enum
{
   B_INVALID,            //  0 (invalid)
   B_GO_UP,              //  1 (store - json)
   B_STOP,               //  2
   B_GO_DOWN,            //  3
   B_DIRECTION,          //  4
   B_BRAKE_OVERIDE,      //  5
   B_DYNAMIC_DEPTH,      //  6
   B_DATA_VIEW,          //  7
   B_DATA_DOWNLOAD,      //  8
   B_DATA_DELETE,        //  9
   B_SINE2_ENABLE,       // 10
   B_SINE3_ENABLE,       // 11
   B_TEST_CONTROL,       // 12
   B_SINE_QUICKLOOK,     // 13
   B_SWEEP_CONTROL,      // 14
   B_SWEEP_QUICKLOOK,    // 15
   B_LIN_EXP,            // 16
   B_SHOW_PLOTS,         // 17
};

// Button states
//
enum ButtonStates
{
   MOTOR_IS_RUNNING_UP,    // 0
   MOTOR_IS_RUNNING_DOWN,
};

class CButtons
{
public:
   CButtons(void);

   static String HomeProcessor(const String& var);
   static String BasicSineProcessor(const String& var);
   static String SineSweepProcessor(const String& var);
   static String EquipmentProcessor(const String& var);

   String GetButtonStates(void);

   void RestoreSavedStates(void);

   void ProcessCheckbox(const String id, const String checked);

   void ProcessButton(const String identifier, const String index);

   void ActionButton(const uint16_t index, const String identifier);

   boolean IsMotorRunningUP() const;

   void SetButtonState(const uint32_t index, const String state);

   boolean GetBooleanState(const uint32_t index);

   void SetBooleanState(const uint32_t index, boolean state);

   String GetButtonStateAsString(const uint32_t index) const;

   String CreateFileName(const uint32_t index) const;

   void EnsureNonSavedIOButtonsOff(void);

   String GetButtonTextAndColors(void);

private:

   void StartTheTest(void);

   void StopTheTest(void);

   String GetState(uint32_t index);

   String AssembleStates(uint32_t i, uint32_t index);

   void HandleWebButton(const String index, const String state);

   void ButtonOn(const uint16_t i);
   void ButtonOff(const uint16_t i);

   void ManageMotorState(const uint32_t index);


private:

};


#endif   // _BUTTONS_H