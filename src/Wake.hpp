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
#include <Settings.hpp>

#define DYNAMIC_MODE 0
#define STATIC_MODE 1

void wake();
void dynamicDive();
void startStaticDive();
void staticDiveWakeUp();
void selectMode();
bool detectSurface();

#endif