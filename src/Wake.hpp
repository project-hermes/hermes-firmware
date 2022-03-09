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
#include <Network/GoogleCloudIOT.hpp>

#define FIRMWARE_VERSION 2

#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 5        /* Time ESP32 will go to sleep (in seconds) between each static record */

#define minDepth 0         /* Min depth to validate dynamic dive */
#define maxCounter 10      /* Number of No Water to end dynamic dive */
#define maxStaticCounter 2 /* Number of No Water to end static dive */

void wake();
void dynamicDive();
void startStaticDive();

void sleep(bool timer = false);



#endif