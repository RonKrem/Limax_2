// Main.h
//
#ifndef  _MAIN_H
#define  _MAIN_H

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <AsyncEventSource.h>
#include <Arduino_JSON.h>
#include <LittleFS.h>
#include "Ledscreen.h"
#include <SparkFun_Qwiic_OLED.h>
#include <res/qw_fnt_8x16.h>
#include <SD_MMC.h>


#define  BRAKE_PORT              21
#define  DATA_FOLDER             "/LimaxData"
#define  MAX_PATHNAME_LENGTH     40    // used in SetupDatabase


#endif   // _MAIN_H