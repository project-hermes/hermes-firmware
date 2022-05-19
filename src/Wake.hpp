#ifndef WAKE_HPP
#define WAKE_HPP
#include <Arduino.h>
#include <Wire.h>
#include <TimeLib.h>
#include <AutoConnect.h>

#include <Dive.hpp>
#include <hal/TSYS01.hpp>
#include <hal/MS5837.hpp>
#include <hal/remora-hal.h>
#include <Storage/SecureDigital.hpp>
#include <Navigation/GNSS.hpp>
#include <Utils.hpp>
#include <Connect.hpp>
//#include <Network/GoogleCloudIOT.hpp>

#define FIRMWARE_VERSION 1.2

#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP_STATIC 5 /* Time ESP32 will go to sleep (in seconds) between each static record */
#define TIME_DYNAMIC_MODE 1000 /*Time between two records in dynamic mode (ms)*/
#define minDepth    10          /* Min depth to validate dynamic dive */
#define maxCounter 10       /* 10 Number of No Water to end dynamic dive */
#define maxStaticCounter 2     /* 2 Number of No Water to end static dive */
#define WATER_TRIGGER 1500 //tension de détection de l'eau en mv entre 0 et 3300 (pour de l'eau douce mettre une valeur basse)

void wake();
void dynamicDive();
void startStaticDive();
void recordStaticDive();
void endStaticDive();

void sleep(bool timer = false);

#endif