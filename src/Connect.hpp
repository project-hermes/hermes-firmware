#ifndef CONNECT_HPP
#define CONNECT_HPP
#include <Wake.hpp>
#include <Arduino.h>
#include <Wire.h>
#include <TimeLib.h>
#include <AutoConnect.h>
#include <Storage/SecureDigital.hpp>
#include <Navigation/GNSS.hpp>
#include <Utils.hpp>

void startPortal(SecureDigital sd);

void ota();

int uploadDives(SecureDigital sd);

#endif