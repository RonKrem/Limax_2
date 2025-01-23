
/***************************************************************************
* 
* Modified for Limax by KREMFORD PTY LTD
* 
* TMP116 - Library for Arduino
* 
* Artekit Labs AK-TMP116N - Digital Temperature Sensor Breakout
* https://www.artekit.eu/products/breakout-boards/ak-tmp116n/
*
* Copyright (c) 2019 Artekit Labs
* https://www.artekit.eu

### CTempTMP116.cpp

#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.

***************************************************************************/

#include "TempTMP116.h"

#define     CONVERSION     0.0078125

//-------------------------------------------------------------------
//
bool CTempTMP116::begin(void)
{
	// Initialize I2C
	Wire.begin();

	// Check for ID
	uint16_t id = ReadRegister(TMP116_REG_DEVICE_ID);
	if (id & 0xFFF != 0x116)
   {
		return false;
   }
   else
   {
	   // Set continuous conversion at 500ms and 8 averages.
      //
	   return WriteRegister(TMP116_REG_CONFIGURATION, 0x01, 0xA0);
   }
}

//-------------------------------------------------------------------
//
float CTempTMP116::ReadTemperature(void)
{
	// Read value
	int16_t temp = ReadRegister(TMP116_REG_TEMPERATURE);

	// Convert and return
	return (float) temp * CONVERSION;
}

//-------------------------------------------------------------------
//
bool CTempTMP116::ReadTemperature(float* temperature)
{
	if (!temperature || !DataReady())
   {
      return false;
   }
   else
   {
      // Read value
      int16_t temp = ReadRegister(TMP116_REG_TEMPERATURE);

      // Convert and return
      *temperature = (float) temp * CONVERSION;

      return true;
   }
}

//-------------------------------------------------------------------
//
bool CTempTMP116::DataReady(void)
{
	uint16_t value = 0;

	if (!ReadRegister(TMP116_REG_CONFIGURATION, &value))
   {
		return false;
   }
   else
   {
   	return ((value >> 13) & 0x01) == 1;
   }
}

//-------------------------------------------------------------------
//
bool CTempTMP116::SetLowLimit(float low)
{
	// Set the low threshold
	int16_t temp = (int16_t)(low / CONVERSION);
	return WriteRegister(TMP116_REG_LOW_LIMIT, temp);
}

//-------------------------------------------------------------------
//
bool CTempTMP116::SetHighLimit(float high)
{
	// Set the low threshold
	int16_t temp = (int16_t)(high / CONVERSION);
	return WriteRegister(TMP116_REG_HIGH_LIMIT, temp);
}

//-------------------------------------------------------------------
//
bool CTempTMP116::SetLowHighLimit(float low, float high)
{
	return (SetLowLimit(low) && SetHighLimit(high));
}

//-------------------------------------------------------------------
//
void CTempTMP116::ClearAlert(void)
{
	ReadRegister(TMP116_REG_CONFIGURATION);
}

//-------------------------------------------------------------------
//
TMP116_Alert CTempTMP116::GetAlertType(void)
{
	uint16_t reg = ReadRegister(TMP116_REG_CONFIGURATION);
	
	// No alerts?
	if ((reg & 0xC000) == 0x00)
		return TMP116_NoAlert;

	// Both alerts?
	if ((reg & 0xC000) == 0xC000)
		return TMP116_AlertLowHigh;

	// High or low alert?
	if ((reg & 0xC000) == 0x8000)
		return TMP116_AlertHigh;
	else
		return TMP116_AlertLow;
}

//-------------------------------------------------------------------
//
void CTempTMP116::end(void)
{
	// Set sensor in shutdown mode
	WriteRegister(TMP116_REG_CONFIGURATION, 0x800);

	delay(25);

	// Disable Wire
	Wire.end();
}

//-------------------------------------------------------------------
//
uint16_t CTempTMP116::ReadRegister(uint8_t address)
{
	uint16_t value = 0;
	ReadRegister(address, &value);
	return value;
}

//-------------------------------------------------------------------
//
bool CTempTMP116::ReadRegister(uint8_t address, uint8_t* valueL, uint8_t* valueH)
{
	if (!valueL || !valueH)
		return false;

	Wire.beginTransmission(dev_address);
	Wire.write(address);
	Wire.endTransmission();
	Wire.requestFrom((int) dev_address, (int) 2);

	while (Wire.available() < 2);

	*valueH = Wire.read();
	*valueL = Wire.read();
	return true;
}

//-------------------------------------------------------------------
//
bool CTempTMP116::ReadRegister(uint8_t address, uint16_t* value)
{
	if (!value)
		return false;

	uint8_t valueL = 0, valueH = 0;
	if (!ReadRegister(address, &valueL, &valueH))
		return false;

	uint8_t* data = (uint8_t*) value;

	data[0] = valueL;
	data[1] = valueH;
	
	/*
	Serial.print("Register ");
	Serial.print(address);
	Serial.print(" = ");
	Serial.print(data[0]);
	Serial.print(" ");
	Serial.println(data[1]);
	*/
	return true;
}

//-------------------------------------------------------------------
//
bool CTempTMP116::WriteRegister(uint8_t address, uint16_t value)
{
	uint8_t* data = (uint8_t*) &value;
	return WriteRegister(address, data[0], data[1]);
}

//-------------------------------------------------------------------
//
bool CTempTMP116::WriteRegister(uint8_t address, uint8_t valueL, uint8_t valueH)
{
	Wire.beginTransmission(dev_address);
	Wire.write(address);
	Wire.write(valueH);
	Wire.write(valueL);
	Wire.endTransmission();
	return true;
}
