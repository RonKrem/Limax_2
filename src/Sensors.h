// Sensors.h
//
#ifndef  _SENSORS_H
#define  _SENSORS_H

#include "Main.h"
#include "SensorData.h"



class CSensors
{
public:
   CSensors(void);

   String GetSensorPressUnits(uint8_t unitType) const;

   String GetSensorTempUnits(uint8_t unitType) const;

private:   
};


#endif   // _SENSORS_H
