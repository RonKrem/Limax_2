// Sensors.cpp
//
#include "Sensors.h"


//-------------------------------------------------------------------
//
CSensors::CSensors(void)
{
}

//-------------------------------------------------------------------
// Package the pressure sensor data ready for transfer to the web site.
// 
String CSensors::GetSensorPressUnits(uint8_t unitType)
{
   String unit;

   switch (unitType)
   {
      case 17:
         unit = "PSI";
         break;

      case 19:
         unit = "kPa";
         break;

      case 20:
         unit = "Bar";
         break;

      case 21:
         unit = "mBar";
         break;

      case 22:
         unit = "mmHg";
         break;

      case 23:
         unit = "inHg";
         break;

      case 24:
         unit = "cmH2O";
         break;

      case 25:
         unit = "inH2O";
         break;

      default:
         unit = "none";
         break;
   }

   return unit;
}  

//-------------------------------------------------------------------
// Package the temperature sensor data ready for transfer to the web site.
//
String CSensors::GetSensorTempUnits(uint8_t unitType)
{
   String unit;

   switch (unitType)
   {
      case 1:
         unit = "C";
         break;

      case 2:
         unit = "F";
         break;

      default:
         unit = "?";
         break;
   }

   return unit;
}  
